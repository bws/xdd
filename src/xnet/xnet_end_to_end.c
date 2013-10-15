/* Copyright (C) 1992-2010 I/O Performance, Inc. and the
 * United States Departments of Energy (DoE) and Defense (DoD)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named 'Copying'; if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
 */
/* Principal Author:
 *      Tom Ruwart (tmruwart@ioperformance.com)
 * Contributing Authors:
 *       Steve Hodson, DoE/ORNL
 *       Steve Poole, DoE/ORNL
 *       Bradly Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines necessary the end-to-end
 * Send and Receive functions. 
 */
#include "xint.h"

/*----------------------------------------------------------------------*/
/* xint_e2e_xni_send() - send the data from source to destination 
 * This subroutine will take the message header from the Worker Data Struct
 * and update it with:
 *     - senders Worker Thread number
 *     - time stamp when this packet is being sent normalized to global time
 * The message header is placed after the end of the user data in the
 * message buffer (thus making it a trailer rather than a header).
 * 
 * Return values: 0 is good, -1 is bad
 *
 * The size of the buffer depends on whether it is being used for network
 * I/O as in an End-to-end operation. For End-to-End operations, the size
 * of the buffer is 1 page larger than for non-End-to-End operations.
 *
 * For normal (non-E2E operations) the buffer pointers are as follows:
 *                   |<----------- wd_buf_size = N Pages ----------------->|
 *	                 +-----------------------------------------------------+
 *	                 |  data buffer                                        |
 *	                 |  transfer size (td_xfer_size) rounded up to N pages |
 *	                 |<-wd_bufp                                            |
 *	                 |<-task_datap                                         |
 *	                 +-----------------------------------------------------+
 *
 * For End-to-End operations, the buffer pointers are as follows:
 *  |<------------------- wd_buf_size = N+1 Pages ------------------------>|
 *	+----------------+-----------------------------------------------------+
 *	|<----1 page---->|  transfer size (td_xfer_size) rounded up to N pages |
 *	|<-wd_bufp       |<-task_datap                                         |
 *	|     |   E2E    |      E2E                                            |
 *	|     |<-Header->|   data buffer                                       |
 *	+-----*----------*-----------------------------------------------------+
 *	      ^          ^
 *	      ^          +-e2e_datap     
 *	      +-e2e_hdrp 
 */
int32_t
xint_e2e_xni_send(worker_data_t *wdp) {
	target_data_t		*tdp;
	xint_e2e_t			*e2ep;		// Pointer to the E2E data struct
	xdd_e2e_header_t	*e2ehp;		// Pointer to the E2E Header
	int 				bytes_sent;		// Cumulative number of bytes sent 
	int					sento_calls;	// Number of times sendto() has been called
	int					max_xfer;
	unsigned char 		*bufp;
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry

	/* Release the current target buffer */
	xni_release_target_buffer(&wdp->wd_e2ep->xni_buf);
	
	/* Request a target buffer */
	xni_request_target_buffer(wdp->wd_e2ep->xni_conn, &wdp->wd_e2ep->xni_buf);
	uintptr_t xtb_bufp = (uintptr_t)xni_target_buffer_data(wdp->wd_e2ep->xni_buf);
	wdp->wd_task.task_datap = (unsigned char*)(xtb_bufp + (2*getpagesize()));
	wdp->wd_e2ep->e2e_hdrp = (xdd_e2e_header_t *)(xtb_bufp + (2*getpagesize() - sizeof(xdd_e2e_header_t)));
	wdp->wd_e2ep->e2e_datap = wdp->wd_task.task_datap;

	/* Local aliases */
	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: ENTER: e2ep=%p: e2ehp=%p: e2e_datap=%p\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep, e2ehp, e2ep->e2e_datap);

	// The "task" data structure contains variables relevant to the file-read operation 
	e2ehp->e2eh_worker_thread_number = wdp->wd_worker_number;
	e2ehp->e2eh_sequence_number = wdp->wd_task.task_op_number;
	e2ehp->e2eh_byte_offset = wdp->wd_task.task_byte_offset;
	e2ehp->e2eh_data_length = wdp->wd_task.task_xfer_size;

	// The message header for this data packet precedes the data portion
	if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_processor_start = xdd_get_processor();
	}

	// Note: the e2ep->e2e_xfer_size is the size of the data field plus the size of the header
	max_xfer = MAXMIT_TCP;
	bytes_sent = 0;
	bufp = (unsigned char *)e2ehp;
	sento_calls = 0;
	// The transfer size is the size of the header buffer (not the header struct)
	// plus the amount of data in the data portion of the IO buffer.
	// For EOF operations the amount of data in the data portion should be zero.
	e2ep->e2e_xfer_size = sizeof(xdd_e2e_header_t) + e2ehp->e2eh_data_length;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: Preparing to send %d bytes: e2ep=%p: e2ehp=%p: e2e_datap=%p: e2e_xfer_size=%d: e2eh_data_length=%lld\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_xfer_size,e2ep,e2ehp,e2ep->e2e_datap,e2ep->e2e_xfer_size,(long long int)e2ehp->e2eh_data_length);
if (xgp->global_options & GO_DEBUG_E2E) xdd_show_e2e_header((xdd_e2e_header_t *)bufp);

	nclk_now(&wdp->wd_counters.tc_current_net_start_time);
	e2ep->e2e_send_status = xni_send_target_buffer(e2ep->xni_conn,
												   &e2ep->xni_buf);
	nclk_now(&wdp->wd_counters.tc_current_net_end_time);
	// Time stamp if requested
	if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_xfer_size = e2ep->e2e_xfer_size;
		ttep->tte_net_start = wdp->wd_counters.tc_current_net_start_time;
		ttep->tte_net_end = wdp->wd_counters.tc_current_net_end_time;
		ttep->tte_net_processor_end = xdd_get_processor();
		ttep->tte_net_xfer_calls = sento_calls;
	}
	
	// Calculate the Send/Receive time by the time it took the last sendto() to run
	e2ep->e2e_sr_time = (wdp->wd_counters.tc_current_net_end_time - wdp->wd_counters.tc_current_net_start_time);

	if (bytes_sent != e2ep->e2e_xfer_size) {
		xdd_e2e_err(wdp,"xdd_e2e_src_send","ERROR: could not send header+data from e2e source\n");
		return(-1);
	}

	if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: EXIT...\n",(long long int)pclk_now(),tdp->td_target_number, wdp->wd_worker_number);


    return(0);

} /* end of xdd_e2e_src_send() */

/*----------------------------------------------------------------------*/
/* xint_e2e_xni_recv() - Receive data from the source side
 * This subroutine will block until data is received or until the
 * connection is broken in which case an error is returned.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
// The receive is done in two parts.
//    1) The first part of the receive reads in the header portion of the packet
//       which is normally 1 page. The header contains the number of bytes to
//       read in for the data portion of the packet.
//    2) The second part of the receive reads in the data portion of the packet.
//       If this is an EOF operation then there is no further data to read.
int32_t xint_e2e_xni_recv(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int32_t				status;			// Status of the call to xdd_e2e_dst_connection()
	int 				header_bytes_received;	// Number of header bytes received 
	int 				data_bytes_received;	// Number of data bytes received 
	nclk_t 				e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


	/* Release the current target buffer */
	xni_release_target_buffer(&wdp->wd_e2ep->xni_buf);
	
	/* Request a target buffer and assemble it into the wdp */
	xni_request_target_buffer(wdp->wd_e2ep->xni_conn, &wdp->wd_e2ep->xni_buf);
	uintptr_t bufp = (uintptr_t)xni_target_buffer_data(wdp->wd_e2ep->xni_buf);
	wdp->wd_task.task_datap = (unsigned char*)(bufp + (2*getpagesize()));
	wdp->wd_e2ep->e2e_hdrp = (xdd_e2e_header_t *)(bufp + (2*getpagesize() - sizeof(xdd_e2e_header_t)));
	wdp->wd_e2ep->e2e_datap = wdp->wd_task.task_datap;

	/* Local aliases */
	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;

	nclk_now(&e2e_wait_1st_msg_start_time);
	wdp->wd_counters.tc_current_net_start_time = e2e_wait_1st_msg_start_time;

	// Make the connection to the source
	status = xdd_e2e_dest_connection(wdp);
	if (status) 
		return(-1);

	// Read in the E2E Header, this will tell us the length of the data portion of this E2E Message
	header_bytes_received = xdd_e2e_dest_receive_header(wdp);
	if (header_bytes_received <= 0)  {
		xdd_e2e_dest_receive_error(wdp);
		return(-1);
	}

	data_bytes_received = 0;
	if (e2ehp->e2eh_magic == XDD_E2E_DATA_READY) {
		// Read in the data portion of this E2E Message
		data_bytes_received = xdd_e2e_dest_receive_data(wdp);
		if (data_bytes_received <= 0)  {
			xdd_e2e_dest_receive_error(wdp);
			return(-1);
		}
	} else {
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive: Target %d Worker: %d: Received and EOF\n", (unsigned long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number);
	}
	
	nclk_now(&wdp->wd_counters.tc_current_net_end_time);

	// Keep track of the total number of bytes received so far...
	e2ep->e2e_recv_status = header_bytes_received + data_bytes_received;
	
	// This will record the amount of time that we waited from the time we started until we got the first packet
	if (!e2ep->e2e_wait_1st_msg) 
		e2ep->e2e_wait_1st_msg = wdp->wd_counters.tc_current_net_end_time - e2e_wait_1st_msg_start_time;

	// If this is the first packet received by this Worker Thread then record the *end* of this operation as the
	// *start* of this pass. The reason is that the initial recvfrom() may have been issued long before the
	// Source side started sending data and we need to ignore that startup delay. 
	if (wdp->wd_counters.tc_pass_start_time == NCLK_MAX)  { // This is an indication that this is the fist recvfrom() that has completed
		wdp->wd_counters.tc_pass_start_time = wdp->wd_counters.tc_current_net_end_time;
		e2ep->e2e_sr_time = 0; // The first Send/Receive time is zero.
	} else { // Calculate the Send/Receive time by the time it took the last recvfrom() to run
		e2ep->e2e_sr_time = (wdp->wd_counters.tc_current_net_end_time - wdp->wd_counters.tc_current_net_start_time);
	}

	e2ehp->e2eh_recv_time = wdp->wd_counters.tc_current_net_end_time; // This needs to be the net_end_time from this side of the operation

	// If time stamping is on then we need to reset these values
	if ((tdp->td_ts_table.ts_options & (TS_ON|TS_TRIGGERED))) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_start = wdp->wd_counters.tc_current_net_start_time;
		ttep->tte_net_end = wdp->wd_counters.tc_current_net_end_time;
		ttep->tte_net_processor_end = xdd_get_processor();
		ttep->tte_net_xfer_size = e2ep->e2e_recv_status;
		ttep->tte_byte_offset = e2ehp->e2eh_byte_offset;
		ttep->tte_disk_xfer_size = e2ehp->e2eh_data_length;
		ttep->tte_op_number = e2ehp->e2eh_sequence_number;
		if (e2ehp->e2eh_magic == XDD_E2E_EOF)
			ttep->tte_op_type = SO_OP_EOF;
		else ttep->tte_op_type = SO_OP_WRITE;
	}

	e2ep->e2e_readset = e2ep->e2e_active;  /* Prepare for the next select */

	return(0);

} /* end of xint_e2e_xni_recv() */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
