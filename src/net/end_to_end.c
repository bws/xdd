/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines necessary the end-to-end
 * Send and Receive functions. 
 */
#include "xint.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_src_send() - send the data from source to destination 
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
xdd_e2e_src_send(worker_data_t *wdp) {
	target_data_t		*tdp;
	xint_e2e_t			*e2ep;		// Pointer to the E2E data struct
	xdd_e2e_header_t	*e2ehp;		// Pointer to the E2E Header
	int 				bytes_sent;		// Cumulative number of bytes sent 
	int					send_size; 	// Number of bytes to send for each call to sendto()
	int					sento_calls;	// Number of times sendto() has been called
	int					max_xfer;
	unsigned char 		*bufp;
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: ENTER: e2ep=%p: e2ehp=%p: e2e_datap=%p\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep, e2ehp, e2ep->e2e_datap);

	memcpy(e2ehp->e2eh_cookie, tdp->td_magic_cookie, sizeof(e2ehp->e2eh_cookie));

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
	while (bytes_sent < e2ep->e2e_xfer_size) {
		send_size = e2ep->e2e_xfer_size - bytes_sent;
		if (send_size > max_xfer) 
			send_size = max_xfer;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: Actually sending <send_size> %d bytes: e2ep=%p: e2ehp=%p: e2e_datap=%p: bytes_sent=%d: e2ehp+bytes_sent=%p: first 8 bytes=0x%016llx\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, send_size,e2ep,e2ehp,e2ep->e2e_datap,(int)bytes_sent,bufp, *((unsigned long long int *)bufp));
		e2ep->e2e_send_status = sendto(e2ep->e2e_sd,
									   bufp,
									   send_size, 
									   0, 
									   (struct sockaddr *)&e2ep->e2e_sname, 
									   sizeof(struct sockaddr_in));
		if (e2ep->e2e_send_status <= 0) {
			xdd_e2e_err(wdp,"xdd_e2e_src_send","ERROR: error sending HEADER+DATA to destination\n");
			return(-1);
		}
		bytes_sent += e2ep->e2e_send_status;
		bufp += e2ep->e2e_send_status;
		sento_calls++;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: Sent %d of %d bytes - %d bytes sent so far: bufp=%p\n",(long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_send_status,e2ep->e2e_xfer_size,bytes_sent,bufp);
	}
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
/* xdd_e2e_dest_connection() - Wait for an incoming connection and 
 * return when it arrives.
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_dest_connection(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker

	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_connection: Target %d Worker: %d: ENTER: e2e_nd=%d: e2e_sd=%d: FD_SETSIZE=%d\n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number,e2ep->e2e_nd,e2ep->e2e_sd,FD_SETSIZE);

	int rc = select(e2ep->e2e_nd, &e2ep->e2e_readset, NULL, NULL, NULL);
	assert(rc != -1);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
	 * client sockets. We first check to see if the sd is in the readset.
	 * If so, this means that a client is trying to make a new connection
	 * in which case we need to issue an accept to establish the connection
	 * and obtain a new Client Socket Descriptor (csd).
	 */
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_connection: Target %d Worker: %d: Inside SELECT \n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number);
	if (FD_ISSET(e2ep->e2e_sd, &e2ep->e2e_readset)) { /* Process an incoming connection */
		e2ep->e2e_current_csd = e2ep->e2e_next_csd;

		e2ep->e2e_csd[e2ep->e2e_current_csd] = accept(e2ep->e2e_sd, (struct sockaddr *)&e2ep->e2e_rname,&e2ep->e2e_rnamelen);
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_connection: Target %d Worker: %d: connection accepted: sd=%d\n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_sd);

		FD_SET(e2ep->e2e_csd[e2ep->e2e_current_csd], &e2ep->e2e_active); /* Mark this fd as active */
		FD_SET(e2ep->e2e_csd[e2ep->e2e_current_csd], &e2ep->e2e_readset); /* Put in readset so that it gets processed */

		/* Find the next available csd close to the beginning of the CSD array */
		e2ep->e2e_next_csd = 0;
		while (e2ep->e2e_csd[e2ep->e2e_next_csd] != 0) {
			e2ep->e2e_next_csd++;
			if (e2ep->e2e_next_csd == FD_SETSIZE) {
				e2ep->e2e_next_csd = 0;
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_connection: Target %d Worker: %d: ERROR: no csd entries left\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_worker_number);
				return(-1);
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	return(0);
} // End of xdd_e2e_dest_connection()

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_receive_header() - Receive E2E Header from the source side
 * This subroutine will block until the entire header is received or until 
 * the connection is broken in which case an error is returned.
 *
 * Return values: Upon successfully reading the header and validating
 * it, the size of the header in bytes is returned to the caller.
 * Otherwise, in the event of an error, the status of the recvfrom() 
 * call is returned to the caller.
 *
 */
int32_t
xdd_e2e_dest_receive_header(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int					status;			// function call status
	int 				bytes_received;	// Cumulative number of bytes received if multiple recvfrom() invocations are necessary
	int					receive_size; 	// The number of bytes to receive for this invocation of recvfrom()
	int					max_xfer;	// Maximum TCP transmission size
	unsigned char 		*bufp;
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;

	e2ep->e2e_header_size = sizeof(xdd_e2e_header_t);
	e2ep->e2e_xfer_size = e2ep->e2e_header_size;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_header: Target %d Worker: %d: ENTER: Waiting to receive %d bytes of header: op# %lld: e2ep=%p: e2ehp=%p: e2e_datap=%p\n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_header_size, (long long int)wdp->wd_task.task_op_number, e2ep, e2ehp, e2ep->e2e_datap );

	max_xfer = MAXMIT_TCP;

	/* This section will check to see which of the Client Socket Descriptors
	 * are in the readset. For those csd's that are ready, a recv is issued to 
	 * receive the incoming data. 
	 */
	status = -1; // Default status
	bytes_received = 0;
	for (e2ep->e2e_current_csd = 0; e2ep->e2e_current_csd < FD_SETSIZE; e2ep->e2e_current_csd++) { // Process all CSDs that are ready
		if (FD_ISSET(e2ep->e2e_csd[e2ep->e2e_current_csd], &e2ep->e2e_readset)) { /* Process this csd */
			if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
				ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
				ttep->tte_net_processor_start = xdd_get_processor();
			}

			nclk_now(&wdp->wd_counters.tc_current_net_start_time);

			// Read in the E2E Header 
			bytes_received = 0;
			bufp = (unsigned char *)e2ehp;
			while (bytes_received < e2ep->e2e_header_size) {
				// Calculate the max number of bytes we can receive per call to recvfrom()
				receive_size = e2ep->e2e_header_size - bytes_received;
				if (receive_size > max_xfer)
					receive_size = max_xfer;
				
				// Issue recvfrom()
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_header: Target: %d: Worker: %d: HEADER: Calling recvfrom bytes_received=%d: receive_size=%d: e2ehp=%p: new_bp=%p\n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, bytes_received, receive_size, e2ehp, bufp);
				status = recvfrom(e2ep->e2e_csd[e2ep->e2e_current_csd], 
								  bufp,
								  receive_size, 
								  0, 
								  NULL,
								  NULL);
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_header: Target: %d: Worker: %d: HEADER: Received %d of %d bytes\n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, status, e2ep->e2e_header_size);
if (xgp->global_options & GO_DEBUG_E2E) xdd_show_e2e_header(e2ehp);

				// Check for errors
				if (status <= 0) {
					e2ep->e2e_recv_status = status;
					fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_header: Target %d Worker: %d: ERROR RECEIVING HEADER: recv_status=%d, errno=%d\n",
						xgp->progname,
						tdp->td_target_number,
						wdp->wd_worker_number,
						e2ep->e2e_recv_status,
						errno);
					return(status);
				}
				// Otherwise, figure out how much of the header we read
				bytes_received += status;
				bufp += status;
			} // End of WHILE loop that received incoming data from the source machine
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_header: Target: %d: Worker: %d: HEADER: Got the header... now check to see if the status <%d> is > 0 \n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, status);
		} // End of IF stmnt that processes a CSD 
	} // End of FOR loop that processes all CSDs that were ready

	if (bytes_received != e2ep->e2e_header_size) {
		// This is an internal error that should not occur...
		fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_header: Target %d Worker: %d: INTERNAL ERROR: The number of bytes received <%d> is not equal to the size of the E2E Header <%d>\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			bytes_received,
			e2ep->e2e_header_size);
		return(-1);
	}

	// ensure the cookies are equal
	char expected_cookie[sizeof(tdp->td_magic_cookie)];
	memcpy(expected_cookie, tdp->td_magic_cookie, sizeof(expected_cookie));
	if (memcmp(e2ehp->e2eh_cookie, expected_cookie, sizeof(e2ehp->e2eh_cookie))) {
		// Invalid E2E Header - bad magic cookie
		//TODO: it would be helpful to print out the magic cookie
		// received vs. what was expected, but this will require
		// converting the binary cookie to a textual representation
		fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_header: Target %d Worker: %d: ERROR: Bad magic cookie on recv %d\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			e2ep->e2e_msg_recv);
		return(-1);
	}

	if ((e2ehp->e2eh_magic != XDD_E2E_DATA_READY) && (e2ehp->e2eh_magic != XDD_E2E_EOF)) {
		// Invalid E2E Header - bad magic number
		fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_header: Target %d Worker: %d: ERROR: Bad magic number 0x%08x on recv %d - should be either 0x%08x or 0x%08x\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			e2ehp->e2eh_magic, 
			e2ep->e2e_msg_recv,
			XDD_E2E_DATA_READY, 
			XDD_E2E_EOF);
		return(-1);
	}

	return(bytes_received);

} // End of xdd_e2e_dest_receive_header()

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_receive_data() - Receive E2E Data from the source side
 * This subroutine will block until the entire data portion is received or 
 * until the connection is broken in which case an error is returned.
 *
 * Return values: Upon successfully reading the data and validating
 * it, the number of data bytes is returned to the caller.
 * Otherwise, in the event of an error, the status of the recvfrom() 
 * call is returned to the caller.
 *
 */
int32_t
xdd_e2e_dest_receive_data(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int					status;			// function call status
	int 				bytes_received;	// Cumulative number of bytes received if multiple recvfrom() invocations are necessary
	int					receive_size; 	// The number of bytes to receive for this invocation of recvfrom()
	int					max_xfer;	// Maximum TCP transmission size
	unsigned char 		*bufp;
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;

	e2ep->e2e_data_size = e2ehp->e2eh_data_length;
	e2ep->e2e_xfer_size = e2ep->e2e_data_size;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_data: Target %d Worker: %d: ENTER: Waiting to receive %d bytes of DATA: op# %lld: e2ep=%p: e2ehp=%p: e2e_datap=%p\n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_data_size, (long long int)wdp->wd_task.task_op_number, e2ep, e2ehp, e2ep->e2e_datap );
	// The following uses strictly TCP

	max_xfer = MAXMIT_TCP;

	/* This section will check to see which of the Client Socket Descriptors
	 * are in the readset. For those csd's that are ready, a recv is issued to 
	 * receive the incoming data. 
	 */
	status = -1; // Default status
	bytes_received = 0;
	for (e2ep->e2e_current_csd = 0; e2ep->e2e_current_csd < FD_SETSIZE; e2ep->e2e_current_csd++) { // Process all CSDs that are ready
		if (FD_ISSET(e2ep->e2e_csd[e2ep->e2e_current_csd], &e2ep->e2e_readset)) { /* Process this csd */
			if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
				ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
				ttep->tte_net_processor_start = xdd_get_processor();
			}

			// Receive DATA if this was a "DATA" message
			e2ep->e2e_data_size = e2ehp->e2eh_data_length;
			bytes_received = 0;
			bufp = (unsigned char *)e2ep->e2e_datap;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_data: Target: %d: Worker: %d: OK - IT IS A HEADER SO LETS READ DATA: bytes_received=%d:e2e_data_size=%d \n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, bytes_received,e2ep->e2e_data_size);
			while (bytes_received < e2ep->e2e_data_size) {
				receive_size = e2ep->e2e_data_size - bytes_received;
				if (receive_size > max_xfer)
					receive_size = max_xfer;
				// Issue recvfrom()
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_data: Target %d Worker: %d: BEFORE RECVFROM DATA: Preparing to receive <receive_size> %d <e2e_data_size=%d> bytes of data, op# %lld, buffer address %p, wd_bufp=%p, wd_task.task_datap=%p, e2e_datap=%p\n", (unsigned long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, receive_size, e2ep->e2e_data_size, (long long int)wdp->wd_task.task_op_number, ((unsigned char *)e2ehp) + bytes_received, wdp->wd_bufp, wdp->wd_task.task_datap, e2ep->e2e_datap);
				status = recvfrom(e2ep->e2e_csd[e2ep->e2e_current_csd], 
								bufp, 
								receive_size, 
								0, 
								NULL,
								NULL);
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_dest_receive_data: Target: %d: Worker: %d: AFTER RECVFROM DATA: Received %d of %d bytes\n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, status, e2ep->e2e_data_size);

				// Check for errors
				if (status <= 0) {
					e2ep->e2e_recv_status = status;
					fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_data: Target %d Worker: %d: ERROR RECEIVING HEADER: recv_status=%d, errno=%d\n",
						xgp->progname,
						tdp->td_target_number,
						wdp->wd_worker_number,
						e2ep->e2e_recv_status,
						errno);
					return(status);
				}

				// Otherwise, figure out how much data we got and go back for more if necessary
				bytes_received += status;
				bufp += status;
			} // End of WHILE loop that reads the E2E Header
		} 

	}

	return(bytes_received);

} // End of xdd_e2e_dest_receive_data()

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_receive_error() - Process an error event during recvfrom()
 * Return values: None.
 */
int
xdd_e2e_dest_receive_error(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	int					errno_save;		// A copy of the errno


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;

	if (e2ep->e2e_recv_status == 0) { // A status of 0 means that the source side shut down unexpectedly - essentially and Enf-Of-File
		fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_error: Target %d Worker: %d: ERROR: Connection closed prematurely by Source, op number %lld, location %lld\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			(long long int)wdp->wd_task.task_op_number,
			(long long int)wdp->wd_task.task_byte_offset);
	} else if (e2ep->e2e_recv_status < 0) { // A status less than 0 indicates some kind of error.
		errno_save = errno;
		fprintf(xgp->errout,"\n%s: xdd_e2e_dest_receive_error: Target %d Worker: %d: ERROR: recvfrom returned -1, errno %d, op number %lld, location %lld\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			errno,
			(long long int)wdp->wd_task.task_op_number,
			(long long int)wdp->wd_task.task_byte_offset);

		// Restore the errno and display the reason for the error
		errno = errno_save;
		perror("Reason");
	} 
 	// At this point we need to clear out this csd and "Deactivate" the socket. 
	FD_CLR(e2ep->e2e_csd[e2ep->e2e_current_csd], &e2ep->e2e_active);
 	(void) closesocket(e2ep->e2e_csd[e2ep->e2e_current_csd]);
 	e2ep->e2e_csd[e2ep->e2e_current_csd] = 0;
	return(errno);

} // End of xdd_e2e_dest_receive_error()

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_receive() - Receive data from the source side
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
int32_t
xdd_e2e_dest_receive(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int32_t				status;			// Status of the call to xdd_e2e_dst_connection()
	int 				header_bytes_received;	// Number of header bytes received 
	int 				data_bytes_received;	// Number of data bytes received 
	nclk_t 				e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


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
	if (tdp->td_counters.tc_pass_start_time == NCLK_MAX)  { // This is an indication that this is the fist recvfrom() that has completed
		tdp->td_counters.tc_pass_start_time = wdp->wd_counters.tc_current_net_end_time;
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

} /* end of xdd_e2e_dest_receive() */
/*----------------------------------------------------------------------*/
/* xdd_e2e_eof_source_side() - End-Of-File processing for Source 
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_e2e_eof_source_side(worker_data_t *wdp) {
	target_data_t		*tdp;
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int 				bytes_sent;		// Cumulative number of bytes sent if multiple sendto() invocations are necessary
	int 				sendto_calls;	// Cumulative number of calls      if multiple sendto() invocations are necessary
	int					send_size; 		// The number of bytes to send for this invocation of sendto()
	int					max_xfer;		// Maximum TCP transmission size
	unsigned char 		*bufp;
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;

	e2ep->e2e_header_size = sizeof(xdd_e2e_header_t);
	e2ep->e2e_xfer_size = e2ep->e2e_header_size;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_eof_source_side: Target %d Worker: %d: ENTER: \n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number);

    /* If this is XNI, just short circuit */
    if (PLAN_ENABLE_XNI & tdp->td_planp->plan_options) {
		e2ep->e2e_send_status = 0;
		e2ep->e2e_sr_time = 0;
		return 0;
	}
	
	// The following uses strictly TCP
	max_xfer = MAXMIT_TCP;

	nclk_now(&wdp->wd_counters.tc_current_net_start_time);
	e2ehp->e2eh_worker_thread_number = wdp->wd_worker_number;
	e2ehp->e2eh_sequence_number = (wdp->wd_task.task_op_number + wdp->wd_worker_number); // This is an EOF packet header
	e2ehp->e2eh_byte_offset = -1; // NA
	e2ehp->e2eh_data_length = 0;	// NA - no data being sent other than the header
	e2ehp->e2eh_magic = XDD_E2E_EOF;
	memcpy(e2ehp->e2eh_cookie, tdp->td_magic_cookie, sizeof(e2ehp->e2eh_cookie));

	if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_processor_start = xdd_get_processor();
	}
	// This will send the E2E Header to the Destination
	bytes_sent = 0;
	sendto_calls = 0;
	bufp = (unsigned char *)e2ehp;
	while (bytes_sent < e2ep->e2e_header_size) {
		send_size = e2ep->e2e_header_size - bytes_sent;
		if (send_size > max_xfer) 
			send_size = max_xfer;

		bufp += bytes_sent;
		e2ep->e2e_send_status = sendto(e2ep->e2e_sd,
									   bufp,
									   send_size, 
									   0, 
									   (struct sockaddr *)&e2ep->e2e_sname, 
									   sizeof(struct sockaddr_in));

		if (e2ep->e2e_send_status <= 0) {
			xdd_e2e_err(wdp,"xdd_e2e_eof_source_side","ERROR: error sending EOF to destination\n");
			return(-1);
		}
		bytes_sent += e2ep->e2e_send_status;
		sendto_calls++;
if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_eof_source_side: Target: %d: Worker: %d: Sent %d of %d bytes of the HEADER - %d bytes sent so far\n", (long long int)pclk_now(),  tdp->td_target_number, wdp->wd_worker_number, send_size, e2ep->e2e_header_size,bytes_sent);
	}
	nclk_now(&wdp->wd_counters.tc_current_net_end_time);
	
	// Calculate the Send/Receive time by the time it took the last sendto() to run
	e2ep->e2e_sr_time = (wdp->wd_counters.tc_current_net_end_time - wdp->wd_counters.tc_current_net_start_time);
	// If time stamping is on then we need to reset these values
   	if ((tdp->td_ts_table.ts_options & (TS_ON|TS_TRIGGERED))) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_start = wdp->wd_counters.tc_current_net_start_time;
		ttep->tte_net_end = wdp->wd_counters.tc_current_net_end_time;
		ttep->tte_net_processor_end = xdd_get_processor();
		ttep->tte_net_xfer_size = bytes_sent;
		ttep->tte_net_xfer_calls = sendto_calls;
		ttep->tte_byte_offset = -1;
		ttep->tte_disk_xfer_size = 0;
		ttep->tte_op_number =e2ehp->e2eh_sequence_number;
		ttep->tte_op_type = TASK_OP_TYPE_EOF;
	}


	if (bytes_sent != e2ep->e2e_header_size) {
		xdd_e2e_err(wdp,"xdd_e2e_eof_source_side","ERROR: could not send EOF to destination\n");
		return(-1);
	}

	return(0);
} /* end of xdd_e2e_eof_source_side() */

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
