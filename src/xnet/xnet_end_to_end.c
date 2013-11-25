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

/* Generate a useful debug macro */
#define DEBUG 1
#if DEBUG
#define dprintf(flag, ...) if (xgp->global_options & (flag)) fprintf(stderr, __VA_ARGS__)
#define de2eprintf(...) dprintf(GO_DEBUG_E2E, __VA_ARGS__)
#else
#define dprintf(...)
#define de2eprintf(...)
#endif

int32_t xint_e2e_src_connect(target_data_t *tdp) {

	int rc = 0;
	int e2e_idx = 0;
	
	/* Loop through the available addresses, and connect */
	while (0 == tdp->td_e2ep->e2e_address_table[e2e_idx].port_count)
		e2e_idx++;
	
	/* Resolve name to an IP */
	rc = xint_lookup_addr(tdp->td_e2ep->e2e_address_table[e2e_idx].hostname, 0,
						  &tdp->td_e2ep->e2e_dest_addr);
	struct in_addr addr = { .s_addr = tdp->td_e2ep->e2e_dest_addr };
	char* ip_string = inet_ntoa(addr);
	fprintf(xgp->errout, "Dest host: %s Connect IP: %s Port: %d\n", tdp->td_e2ep->e2e_address_table[e2e_idx].hostname, ip_string, tdp->td_e2ep->e2e_address_table[e2e_idx].base_port);
	
	/* Create an XNI endpoint from the e2e spec */
	xni_endpoint_t xep = {.host = ip_string,
						  .port = tdp->td_e2ep->e2e_address_table[e2e_idx].base_port};
	rc = xni_connect(tdp->xni_ctx, &xep, &tdp->td_e2ep->xni_td_conn);
	return rc;
}

int32_t xint_e2e_src_disconnect(target_data_t *tdp) {

	/* Perform XNI disconnect */
	int rc = xni_close_connection(&tdp->td_e2ep->xni_td_conn);
	return rc;
}

int32_t xint_e2e_dest_connect(target_data_t *tdp) {

	int rc = 0;
	int e2e_idx = 0;

	/* Loop through the available addresses, and connect */
	while (0 == tdp->td_e2ep->e2e_address_table[e2e_idx].port_count)
		e2e_idx++;
	
	/* Resolve name to an IP */
	rc = xint_lookup_addr(tdp->td_e2ep->e2e_address_table[e2e_idx].hostname, 0,
						  &tdp->td_e2ep->e2e_dest_addr);
	struct in_addr addr = { .s_addr = tdp->td_e2ep->e2e_dest_addr };
	char* ip_string = inet_ntoa(addr);
	
	/* Create an XNI endpoint from the e2e spec */
	xni_endpoint_t xep = {.host = ip_string,
						  .port = tdp->td_e2ep->e2e_address_table[e2e_idx].base_port};
	rc = xni_accept_connection(tdp->xni_ctx, &xep, &tdp->td_e2ep->xni_td_conn);
	return rc;
}

int32_t xint_e2e_dest_disconnect(target_data_t *tdp) {

	/* Perform XNI disconnect */
	int rc = xni_close_connection(&tdp->td_e2ep->xni_td_conn);
	return rc;
}

/*
 * xint_e2e_xni_send() - send the data from source to destination 
 *  Using XNI makes sending a bit weird.  The worker thread read a piece
 *  of data from disk, but it will send whatever XNI has queued.  Hopefully
 *  that will be the read data, but XNI just performs a linear search for a
 *  filled target buffer, so  there is no guarantee.
 *
 *  Release the filled target buffer to XNI
 *  Request a target buffer from XNI
 *  Stitch the new target buffer into wdp over the old one (using the e2ehp)
 *  Send the data
 *
 *  Returns 0 on success, -1 on failure
 */
int32_t xint_e2e_xni_send(worker_data_t *wdp) {
	target_data_t		*tdp;
	xint_e2e_t			*e2ep;		// Pointer to the E2E data struct
	xdd_e2e_header_t	*e2ehp;		// Pointer to the E2E Header
	//int 				bytes_sent;		// Cumulative number of bytes sent 
	//int					sento_calls; // Number of times sendto() has been called
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry

	/* Local aliases */
	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;
	
	de2eprintf("DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: ENTER: e2ep=%p: e2ehp=%p: e2e_datap=%p\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep, e2ehp, e2ep->e2e_datap);

    /* Some timestamp code */
	if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_processor_start = xdd_get_processor();
	}

	/* Construct the e2e header */
	e2ehp->e2eh_worker_thread_number = wdp->wd_worker_number;
	e2ehp->e2eh_sequence_number = wdp->wd_task.task_op_number;
	e2ehp->e2eh_byte_offset = wdp->wd_task.task_byte_offset;
	e2ehp->e2eh_data_length = wdp->wd_task.task_xfer_size;
	e2ep->e2e_xfer_size = sizeof(xdd_e2e_header_t) + e2ehp->e2eh_data_length;
	e2ep->e2e_xfer_size = getpagesize() + e2ehp->e2eh_data_length;

	de2eprintf("DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: Preparing to send %d bytes: e2ep=%p: e2ehp=%p: e2e_datap=%p: e2e_xfer_size=%d: e2eh_data_length=%lld\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_xfer_size,e2ep,e2ehp,e2ep->e2e_datap,e2ep->e2e_xfer_size,(long long int)e2ehp->e2eh_data_length);
	if (xgp->global_options & GO_DEBUG_E2E) xdd_show_e2e_header((xdd_e2e_header_t *)xni_target_buffer_data(wdp->wd_e2ep->xni_wd_buf));

	nclk_now(&wdp->wd_counters.tc_current_net_start_time);
	/* Don't send magic eof across wire */
	if (XDD_E2E_EOF != e2ehp->e2eh_magic) {
		/* Set XNI parameters and send */
		xni_target_buffer_set_data_length(e2ep->e2e_xfer_size,
										  e2ep->xni_wd_buf);
		xni_target_buffer_set_target_offset(e2ehp->e2eh_byte_offset,
											e2ep->xni_wd_buf);
	    e2ep->e2e_send_status = xni_send_target_buffer(tdp->td_e2ep->xni_td_conn,
													   &e2ep->xni_wd_buf);
		/* Request a fresh buffer from XNI */
		xni_request_target_buffer(tdp->xni_ctx, &wdp->wd_e2ep->xni_wd_buf);
			 
		/* The first page is XNI, the second page is E2E header */
		uintptr_t bufp =
			(uintptr_t) xni_target_buffer_data(wdp->wd_e2ep->xni_wd_buf);
		wdp->wd_task.task_datap = (unsigned char*)(bufp + getpagesize());
		wdp->wd_e2ep->e2e_hdrp = (xdd_e2e_header_t *)(bufp + (getpagesize() - sizeof(xdd_e2e_header_t)));
		wdp->wd_e2ep->e2e_datap = wdp->wd_task.task_datap;
	} else {
		fprintf(stderr, "Triggered EOF\n");
		e2ep->e2e_send_status = e2ep->e2e_xfer_size;
	}
	nclk_now(&wdp->wd_counters.tc_current_net_end_time);
	
	// Time stamp if requested
	if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
		ttep = &tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		ttep->tte_net_xfer_size = e2ep->e2e_xfer_size;
		ttep->tte_net_start = wdp->wd_counters.tc_current_net_start_time;
		ttep->tte_net_end = wdp->wd_counters.tc_current_net_end_time;
		ttep->tte_net_processor_end = xdd_get_processor();
		ttep->tte_net_xfer_calls = 1;
	}
	
	// Calculate the Send/Receive time by the time it took the last sendto() to run
	e2ep->e2e_sr_time = (wdp->wd_counters.tc_current_net_end_time - wdp->wd_counters.tc_current_net_start_time);

	//if (bytes_sent != e2ep->e2e_xfer_size) {
	//	xdd_e2e_err(wdp,"xdd_e2e_src_send","ERROR: could not send header+data from e2e source\n");
	//	return(-1);
	//}

	de2eprintf("DEBUG_E2E: %lld: xdd_e2e_src_send: Target: %d: Worker: %d: EXIT...\n",(long long int)pclk_now(),tdp->td_target_number, wdp->wd_worker_number);

    return(0);

} /* end of xdd_e2e_src_send() */

/*
 * xint_e2e_xni_recv() - recv the data from source at destination 
 *
 *  Release the empty target buffer to XNI
 *  Request a target buffer from XNI
 *  Stitch the new target buffer into wdp over the old one (using the e2ehp)
 *  Receive the data
 *
 * Return values: 0 is good, -1 is bad
 */
int32_t xint_e2e_xni_recv(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to the Target Data
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker
	xdd_e2e_header_t	*e2ehp;			// Pointer to the E2E Header 
	int32_t				status;			// Status of the call to xdd_e2e_dst_connection()
	nclk_t 				e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived
	xdd_ts_tte_t		*ttep;		// Pointer to a time stamp table entry
	
	/* Release the current target buffer to XNI */
	xni_release_target_buffer(&wdp->wd_e2ep->xni_wd_buf);

	/* Collect the begin time */
	nclk_now(&e2e_wait_1st_msg_start_time);
	wdp->wd_counters.tc_current_net_start_time = e2e_wait_1st_msg_start_time;

	/* Receive a target buffer and assemble it into the wdp */
	tdp = wdp->wd_tdp;
	status = xni_receive_target_buffer(tdp->td_e2ep->xni_td_conn,
									   &wdp->wd_e2ep->xni_wd_buf);

	if (XNI_OK == status) {
		/* Assemble pointers into the worker's target buffer */
		uintptr_t bufp =
			(uintptr_t)xni_target_buffer_data(wdp->wd_e2ep->xni_wd_buf);
		//for (int i = 0; i < 256; i++) {
		//	printf("8 bytes %ld: %d\n", bufp + i, *((int*)bufp + i));
		//}
		wdp->wd_task.task_datap = (unsigned char*)(bufp + (1*getpagesize()));
		wdp->wd_e2ep->e2e_hdrp = (xdd_e2e_header_t *)(bufp + (1*getpagesize() - sizeof(xdd_e2e_header_t)));
		wdp->wd_e2ep->e2e_datap = wdp->wd_task.task_datap;
	}
	else if (XNI_EOF == status) {
		/* No buffer set on EOF, so just create a static one */
		xdd_e2e_header_t *eof_header = malloc(sizeof(*eof_header));
		wdp->wd_e2ep->e2e_hdrp = eof_header;
		wdp->wd_task.task_datap = 0;
		wdp->wd_e2ep->e2e_datap = 0;

		/* Perform EOF Assembly */
		wdp->wd_e2ep->e2e_hdrp->e2eh_magic = XDD_E2E_EOF;
	} else {
		fprintf(xgp->errout, "Error receiving data via XNI.");
		return -1;
	}

	//de2eprintf("DEBUG_E2E: %lld: xdd_e2e_dest_recv: Target: %d: Worker: %d: Preparing to send %d bytes: e2ep=%p: e2ehp=%p: e2e_datap=%p: e2e_xfer_size=%d: e2eh_data_length=%lld\n",(long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number, e2ep->e2e_xfer_size,e2ep,e2ehp,e2ep->e2e_datap,e2ep->e2e_xfer_size,(long long int)e2ehp->e2eh_data_length);
	//xgp->global_options |= GO_DEBUG_E2E;
	//if (xgp->global_options & GO_DEBUG_E2E) xdd_show_e2e_header(wdp->wd_e2ep->e2e_hdrp);

	/* Local aliases */
	e2ep = wdp->wd_e2ep;
	e2ehp = e2ep->e2e_hdrp;

	/* Collect the end time */
	nclk_now(&wdp->wd_counters.tc_current_net_end_time);

	/* Store timing data */
	e2ep->e2e_sr_time = (wdp->wd_counters.tc_current_net_end_time - wdp->wd_counters.tc_current_net_start_time);
	//e2ehp->e2eh_recv_time = wdp->wd_counters.tc_current_net_end_time; // This needs to be the net_end_time from this side of the operation

	/* Tabulate timestamp data */
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
