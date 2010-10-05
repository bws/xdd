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
 *       Brad Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines that support the Target threads.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_targetpass() - This subroutine will perform a single pass
 */
int32_t
xdd_targetpass(ptds_t *p) {

	/* Before we get started, check to see if we need to reset the 
	 * run_status in case we are using the start trigger.
	 */
	if (p->target_options & TO_WAITFORSTART) 
		p->run_status = 0;

	// This barrier is to ensure that all TARGETS start at same time 

	xdd_barrier(&xgp->results_targets_startpass_barrier,&p->occupant,0);

	/* Check to see if any of the other threads have aborted */
	if (xgp->abort) {
		fprintf(xgp->errout,"%s: ERROR: xdd_targetpass: Target number %d name '%s' Aborting due to failure with another target\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		return(-1);
	}

 	// This will wait for the interactive command processor to say go if we are in interactive mode
	if (xgp->global_options & GO_INTERACTIVE) 
		xdd_barrier(&xgp->interactive_barrier,&p->occupant,0);
	if ( xgp->abort == 1) 
		return(0);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN)
		return(0);

	// If this run has completed or we've been canceled, then exit this loop
	if (xgp->run_complete)
		return(0);

	// Things to do before this pass is started
	xdd_target_ttd_before_pass(p);

	// Get the next available qthread and give it a task to perform
	// We stay in the following loop for a single PASS
	p->my_current_byte_location = 0;
	p->my_current_op_number = 0;
	p->bytes_issued = 0;
	p->bytes_completed = 0;
	p->bytes_remaining = p->target_bytes_to_xfer_per_pass;

/////////////////////////////// Loop Starts Here ////////////////////////////////////
	// The pass loops are handled by one of two subroutines depending on whether this is 
	// the Destination Side of an E2E operation or not. 
	p->my_current_state &= ~CURRENT_STATE_PASS_COMPLETE;
	if (p->target_options & TO_ENDTOEND) { // E2E operations are *different*
		if (p->target_options & TO_E2E_SOURCE)
			 xdd_targetpass_e2e_loop_src(p);
		else xdd_targetpass_e2e_loop_dst(p);
	} else { // Normal operations (other than E2E)
		xdd_targetpass_loop(p);
	}
	p->my_current_state |= CURRENT_STATE_PASS_COMPLETE;
/////////////////////////////// Loop  Ends  Here ////////////////////////////////////
	// If this is an E2E operation and we had gotten canceled - just return
	if ((p->target_options & TO_ENDTOEND) && (xgp->canceled))
		return(-1); 

	// Things that the Target Thread needs to do after a pass has completed
	xdd_target_ttd_after_pass(p);

	// Release the results_manager() to process/display the results for this pass
	xdd_barrier(&xgp->results_targets_endpass_barrier,&p->occupant,0);

	// At this point all the Target Threads have completed their pass and have
	// passed thru the previous barrier releasing the results_manager() 
	// to process/display the results for this pass. 

	// Wait at this barrier for the results_manager() to process/display the results for this last pass
	xdd_barrier(&xgp->results_targets_display_barrier,&p->occupant,0);

	// This pass is complete - return to the Target Thread
	return(0);
} // End of xdd_targetpass()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_loop() - This subroutine will assign tasks to QThreads until
 * all bytes have been processed. 
 * 
 * This subroutine is called by xdd_targetpass().
 */
void
xdd_targetpass_loop(ptds_t *p) {
	ptds_t	*qp;
	int		q;


	while (p->bytes_remaining) {
		// Things to do before an I/O is issued
		xdd_target_ttd_before_io_op(p);
		// Get pointer to next QThread to issue a task to
		qp = xdd_get_any_available_qthread(p);

		// Check to see if we've been canceled - if so, we need to leave this loop
		if (xgp->canceled) 
			break;

		// Set up the task for the QThread
		xdd_targetpass_task_setup(qp);

		// Update the pointers/counters in the Target PTDS to get ready for the next I/O operation
		p->my_current_byte_location += qp->my_current_io_size;
		p->my_current_op_number++;
		p->bytes_issued += qp->my_current_io_size;
		p->bytes_remaining -= qp->my_current_io_size;

		// E2E Source Side needs to be monitored...
		if (p->target_options & TO_E2E_SOURCE_MONITOR)
			xdd_targetpass_e2e_monitor(p);

		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_for_task_barrier,&p->occupant,0);

	} // End of WHILE loop that transfers data for a single pass

	// Check to see if we've been canceled - if so, we need to leave 
	if (xgp->canceled) {
		fprintf(xgp->errout,"\n%s: xdd_targetpass_loop: Target %d: ERROR: Canceled!\n",
			xgp->progname,
			p->my_target_number);
		return;
	}
	// Wait for all QThreads to complete their most recent task
	// The easiest way to do this is to get the QThread pointer for each
	// QThread specifically and then reset it's "busy" bit to 0.
	for (q = 0; q < p->queue_depth; q++) {
		qp = xdd_get_specific_qthread(p,q);
		pthread_mutex_lock(&qp->qthread_target_sync_mutex);
		qp->qthread_target_sync &= ~QTSYNC_BUSY; // Mark this QThread NOT Busy
		pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
	}
	return;
} // End of xdd_targetpass_loop()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_task_setup() - This subroutine will set up the task info for an I/O
 */
void
xdd_targetpass_task_setup(ptds_t *qp) {
	ptds_t	*p;

	p = qp->target_ptds;
	// Assign an IO task to this qthread
	qp->task_request = TASK_REQ_IO;

	// Set the Operation Type
	if (p->seekhdr.seeks[p->my_current_op_number].operation == SO_OP_WRITE) // Write Operation
		qp->my_current_op_type = OP_TYPE_WRITE;
	else if (p->seekhdr.seeks[p->my_current_op_number].operation == SO_OP_READ) // READ Operation
		qp->my_current_op_type = OP_TYPE_READ;
	else qp->my_current_op_type = OP_TYPE_NOOP;

	// Figure out the transfer size to use for this I/O
	if (p->bytes_remaining < p->iosize)
		qp->my_current_io_size = p->bytes_remaining;
	else qp->my_current_io_size = p->iosize;

	// Set the location to seek to 
	qp->my_current_byte_location = p->my_current_byte_location;

	// Remember the operation number for this target
	qp->target_op_number = p->my_current_op_number;
	if (p->my_current_op_number == 0) 
		pclk_now(&p->my_first_op_start_time);

   	// If time stamping is on then assign a time stamp entry to this QThread
   	if ((p->ts_options & (TS_ON|TS_TRIGGERED))) {
		qp->ts_current_entry = p->ts_current_entry;	
		p->ts_current_entry++;
		if (p->ts_options & TS_ONESHOT) { // Check to see if we are at the end of the ts buffer
			if (p->ts_current_entry == p->ts_size)
				p->ts_options &= ~TS_ON; // Turn off Time Stamping now that we are at the end of the time stamp buffer
		} else if (p->ts_options & TS_WRAP) {
			p->ts_current_entry = 0; // Wrap to the beginning of the time stamp buffer
		}
		p->ttp->tte[qp->ts_current_entry].pass_number = p->my_current_pass_number;
		p->ttp->tte[qp->ts_current_entry].qthread_number = qp->my_qthread_number;
		p->ttp->tte[qp->ts_current_entry].op_type = qp->my_current_op_type;
		p->ttp->tte[qp->ts_current_entry].op_number = qp->target_op_number;
		p->ttp->tte[qp->ts_current_entry].byte_location = qp->my_current_byte_location;
	}

} // End of xdd_targetpass_task_setup()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
