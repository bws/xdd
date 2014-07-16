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
 * This file contains the subroutines necessary to perform initialization
 * for the end-to-end option.
 */
#include "xint.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_target_init() - init socket library
 * This routine is called during target initialization to initialize the
 * socket library.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_target_init(target_data_t *tdp) {
	xint_restart_t	*rp;	// pointer to a restart structure
	int status;

	// Perform XNI initialization if required
	status = xint_e2e_xni_init(tdp);
	if (status == -1) {
		fprintf(xgp->errout,"%s: xdd_e2e_target_init: could not initialize XNI for e2e target\n",xgp->progname);
		return(-1);
	}
	
	// Restart processing if necessary
	if ((tdp->td_target_options & TO_RESTART_ENABLE) && (tdp->td_restartp)) { // Check to see if restart was requested
		// Set the last_committed_byte_offset to 0
		rp = tdp->td_restartp;
		rp->last_committed_byte_offset = rp->byte_offset;
		rp->last_committed_length = 0;
	}

	return(0);
}

/*----------------------------------------------------------------------*/
/* xdd_e2e_worker_init() - init source and destination sides
 * This routine is called during Worker Thread initialization to initialize
 * a source or destination Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_worker_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	int status;
	in_addr_t addr;

	tdp = wdp->wd_tdp;
	wdp->wd_e2ep->e2e_sr_time = 0;

	if(wdp->wd_e2ep->e2e_dest_hostname == NULL) {
		fprintf(xgp->errout,"%s: xdd_e2e_worker_init: Target %d Worker Thread %d: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		fprintf(xgp->errout,"%s: xdd_e2e_worker_init: Target %d Worker Thread %d: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		return(-1);
	}

	// Get the IP address of the destination host
	status = xint_lookup_addr(wdp->wd_e2ep->e2e_dest_hostname, 0, &addr);
	if (status) {
		fprintf(xgp->errout, "%s: xdd_e2e_worker_init: unable to identify host '%s'\n",
				xgp->progname, wdp->wd_e2ep->e2e_dest_hostname);
		return(-1);
	}

	// Convert to host byte order
	wdp->wd_e2ep->e2e_dest_addr = ntohl(addr);

	if (tdp->td_target_options & TO_E2E_DESTINATION) { // This is the Destination side of an End-to-End
		status = xdd_e2e_dest_init(wdp);
	} else if (tdp->td_target_options & TO_E2E_SOURCE) { // This is the Source side of an End-to-End
		status = xdd_e2e_src_init(wdp);
	} else { // Should never reach this point
		status = -1;
	}

	return status;
} // xdd_e2e_worker_init()

/*----------------------------------------------------------------------*/
/* xdd_e2e_src_init() - init the source side 
 * This routine is called from the xdd io thread before all the action 
 * starts. When calling this routine, it is because this thread is on the
 * "source" side of an End-to-End test. Hence, this routine needs
 * to set up a socket on the appropriate port 
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_src_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	xint_e2e_t		*e2ep;		// Pointer to the E2E data struct


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	
	// Check to make sure that the source target is actually *reading* the data from a file or device
	if (tdp->td_rwratio < 1.0) { // Something is wrong - the source file/device is not 100% read
		fprintf(xgp->errout,"%s: xdd_e2e_src_init: Target %d Worker Thread %d: Error - E2E source file '%s' is not being *read*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			tdp->td_target_full_pathname,
			tdp->td_rwratio);
		return(-1);
	}

	// Init the relevant variables 
	e2ep->e2e_msg_sequence_number = 0;

	return(0);

} /* end of xdd_e2e_src_init() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_init() - init the destination side 
 * This routine is called by a Worker Thread on the "destination" side of an
 * end_to_end operation and is passed a pointer to the Data Struct of the 
 * requesting Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_dest_init(worker_data_t *wdp) {
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	// Check to make sure that the destination target is actually *writing* the data it receives to a file or device
	if (tdp->td_rwratio > 0.0) { // Something is wrong - the destination file/device is not 100% write
		fprintf(xgp->errout,"%s: xdd_e2e_dest_init: Target %d Worker Thread %d: Error - E2E destination file/device '%s' is not being *written*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			tdp->td_target_full_pathname,
			tdp->td_rwratio);
		return(-1);
	}
		
	// Initialize the message counter and sequencer to 0
	wdp->wd_e2ep->e2e_msg_sequence_number = 0;

	// Clear the end-of-file flag
	wdp->wd_e2ep->received_eof = FALSE;

	return(0);

} /* end of xdd_e2e_dest_init() */

/*----------------------------------------------------------------------------*/
/* xdd_get_e2ep() - return a pointer to the xdd_e2e Data Structure 
 */
xint_e2e_t *
xdd_get_e2ep(void) {
	xint_e2e_t	*e2ep;
	
	e2ep = malloc(sizeof(xint_e2e_t));
	if (e2ep == NULL) {
		fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for E2E data structure \n",
		xgp->progname, (int)sizeof(xint_e2e_t));
		return(NULL);
	}
	memset(e2ep, 0, sizeof(xint_e2e_t));

	return(e2ep);
} /* End of xdd_get_e2ep() */

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
