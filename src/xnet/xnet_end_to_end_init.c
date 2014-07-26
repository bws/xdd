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
 * This file contains the subroutines necessary to perform initialization
 * for the end-to-end option.
 */
#include "xint.h"
#include "xni.h"


// forward declarations
static int init_xni(target_data_t*);
static int init_src_worker(worker_data_t*);
static int init_dest_worker(worker_data_t*);


/*----------------------------------------------------------------------*/
/* xint_e2e_target_init() - init target structure for E2E
 * This routine is called during target initialization to initialize the
 * target data structure for end-to-end.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xint_e2e_target_init(target_data_t *tdp) {
	xint_restart_t	*rp;	// pointer to a restart structure
	int status;

	// Perform XNI initialization if required
	status = init_xni(tdp);
	if (status == -1) {
		fprintf(xgp->errout,"%s: xint_e2e_target_init: could not initialize XNI for e2e target\n",xgp->progname);
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

static int
init_xni(target_data_t *tdp)
{
	int rc = 0;
	/* Create the XNI control block */
	size_t num_threads = tdp->td_planp->number_of_iothreads;
	if (xni_protocol_tcp == tdp->xni_pcl)
		rc = xni_allocate_tcp_control_block(num_threads,
											tdp->xni_tcp_congestion,
											tdp->td_planp->e2e_TCP_Win,
											&tdp->xni_cb);
#if HAVE_ENABLE_IB
	else if (xni_protocol_ib == tdp->xni_pcl)
		rc = xni_allocate_ib_control_block(tdp->xni_ibdevice,
										   num_threads,
										   &tdp->xni_cb);
#endif  /* HAVE_ENABLE_IB */
	else
		return -1;
	assert(0 == rc);
	
	/* Create the XNI context */
	rc = xni_context_create(tdp->xni_pcl, tdp->xni_cb, &tdp->xni_ctx);
	assert(0 == rc);

	struct xint_e2e * const e2ep = tdp->td_e2ep;
	const int conncnt = (int)e2ep->e2e_address_table_host_count;

	// Initialize mutexes that protect connection establishment
	e2ep->xni_td_connection_mutexes = calloc(conncnt,
											 sizeof(*e2ep->xni_td_connection_mutexes));
	for (int i = 0; i < conncnt; i++) {
		rc = pthread_mutex_init(e2ep->xni_td_connection_mutexes+i, NULL);
		assert(0 == rc);
	}

	// Allocate XNI connections, one per e2e host
	e2ep->xni_td_connections = calloc(conncnt,
									  sizeof(*e2ep->xni_td_connections));
	e2ep->xni_td_connections_count = conncnt;

	return(0);
}

/*----------------------------------------------------------------------*/
/* xint_e2e_worker_init() - init source and destination sides
 * This routine is called during Worker Thread initialization to initialize
 * a source or destination Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xint_e2e_worker_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	int status;
	in_addr_t addr;

	tdp = wdp->wd_tdp;
	wdp->wd_e2ep->e2e_sr_time = 0;

	if(xint_e2e_worker_dest_hostname(wdp) == NULL) {
		fprintf(xgp->errout,"%s: xint_e2e_worker_init: Target %d Worker Thread %d: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		fprintf(xgp->errout,"%s: xint_e2e_worker_init: Target %d Worker Thread %d: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		return(-1);
	}

	// Get the IP address of the destination host
	status = xint_lookup_addr(xint_e2e_worker_dest_hostname(wdp), 0, &addr);
	if (status) {
		fprintf(xgp->errout, "%s: xint_e2e_worker_init: unable to identify host '%s'\n",
				xgp->progname, xint_e2e_worker_dest_hostname(wdp));
		return(-1);
	}

	// Convert to host byte order
	wdp->wd_e2ep->e2e_dest_addr = ntohl(addr);

	if (tdp->td_target_options & TO_E2E_DESTINATION) { // This is the Destination side of an End-to-End
		status = init_dest_worker(wdp);
	} else if (tdp->td_target_options & TO_E2E_SOURCE) { // This is the Source side of an End-to-End
		status = init_src_worker(wdp);
	} else { // Should never reach this point
		status = -1;
	}

	return status;
} // xint_e2e_worker_init()

static int
init_src_worker(worker_data_t *wdp)
{
	target_data_t	*tdp;
	xint_e2e_t		*e2ep;		// Pointer to the E2E data struct


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	
	// Check to make sure that the source target is actually *reading* the data from a file or device
	if (tdp->td_rwratio < 1.0) { // Something is wrong - the source file/device is not 100% read
		fprintf(xgp->errout,"%s: xint_e2e_src_init: Target %d Worker Thread %d: Error - E2E source file '%s' is not being *read*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			tdp->td_target_full_pathname,
			tdp->td_rwratio);
		return(-1);
	}

	// Init the relevant variables 
	e2ep->e2e_msg_sequence_number = 0;

	int status = xint_e2e_src_connect(wdp);
	if (0 != status) {
		fprintf(xgp->errout, "Failure during XNI connection.\n");
		return -1;
	}

	// Request an I/O buffer from XNI
	xni_request_target_buffer(*xint_e2e_worker_connection(wdp),
							  &wdp->wd_e2ep->xni_wd_buf);
	wdp->wd_task.task_datap = xni_target_buffer_data(wdp->wd_e2ep->xni_wd_buf);

	return(0);
}

static int
init_dest_worker(worker_data_t *wdp)
{
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

	int status = xint_e2e_dest_connect(wdp);
	if (0 != status) {
		fprintf(xgp->errout, "Failure during XNI connection.\n");
		return -1;
	}

	return(0);
}

/*----------------------------------------------------------------------------*/
/* xint_get_e2ep() - allocate a new E2E Data Structure 
 */
xint_e2e_t *
xint_get_e2ep(void) {
	xint_e2e_t	*e2ep;
	
	e2ep = malloc(sizeof(xint_e2e_t));
	if (e2ep == NULL) {
		fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for E2E data structure \n",
		xgp->progname, (int)sizeof(xint_e2e_t));
		return(NULL);
	}
	memset(e2ep, 0, sizeof(xint_e2e_t));

	return(e2ep);
} /* End of xint_get_e2ep() */

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
