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
	p->pass_complete = 0;
	p->last_qthread_assigned = NULL;

/////////////////////////////// Loop Starts Here ////////////////////////////////////
	// The pass loops are handled by one of two subroutines depending on whether this is 
	// the Destination Side of an E2E operation or not. 
	if (p->target_options & TO_E2E_DESTINATION) { // E2E Destination Side gets handled separately
		xdd_targetpass_e2e_loop(p);
	} else { // Normal operations (other than E2E Dest Side)
		xdd_targetpass_loop(p);
	}
/////////////////////////////// Loop  Ends  Here ////////////////////////////////////

	// At this point the Target Thread has assigned all I/O tasks required for this loop.
	// If this is the Source Side of an E2E Operation then we need to send End-of-File (EOF)
	// packets to the Destination. 
	// Otherwise, for normal I/O operations, we need to call xdd_targetpass_end_of_pass()
	// to gracefully funnel all the QThreads into the targetpass_qthread_passcomplete barrier
	// so that they are quiesced when the Results Manager collects their performance data. 
	if (p->target_options & TO_ENDTOEND) {
		if (p->target_options & TO_E2E_SOURCE) { // E2E Source Side Needs to send EOF packets at the end
			xdd_targetpass_eof_source_side(p);
		} else { // E2E Destination Side does not need to do anything
		}
	} else { // Non-E2E - normal I/O
		// This is intended to confirm that all QThreads have completed and funnels them into the 
		// targetpass_qthread_passcomplete barrier
		xdd_targetpass_end_of_pass(p);
	}

	p->pass_complete = 1;
	// Enter the EndOfPass barrier and wait for the last QThread to get here
	xdd_barrier(&p->targetpass_qthread_passcomplete_barrier,&p->occupant,1);


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
	ptds_t	*tmpqp;
	int qmax, qmin;
	int64_t opmax, opmin;
	int qavail;

		while (p->bytes_remaining) {
			// Things to do before an I/O is issued
			xdd_target_ttd_before_io_op(p);
			// Get pointer to next QThread to issue a task to
			qp = xdd_get_next_available_qthread(p);
	
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
			p->last_qthread_assigned = qp;

			if ((p->my_current_op_number > 0) && ((p->my_current_op_number % p->queue_depth) == 0)) {
					qmin = 0;
					qmax = 0;
					opmin = p->target_ops;
					opmax = -1;
					qavail = 0;
					tmpqp = p->next_qp; // first QThread on the chain
					while (tmpqp) { // Scan the QThreads to determine the one furthest ahead and the one furthest behind
						if (tmpqp->this_qthread_is_available) {
							qavail++;
						} else {
							if (tmpqp->target_op_number < opmin) {
								opmin = tmpqp->target_op_number;
								qmin = tmpqp->my_qthread_number;
							}
							if (tmpqp->target_op_number > opmax) {
								opmax = tmpqp->target_op_number;
								qmax = tmpqp->my_qthread_number;
							}
						}
						tmpqp = tmpqp->next_qp;
					}
			fprintf(stderr,"\n\nopmin %4lld, qmin %4d, opmax %4lld, qmax %4d, separation is %4lld, %4d qthreads busy, %lld percent complete\n\n",
				(long long int)opmin, qmin, (long long int)opmax, qmax, (long long int)(opmax-opmin+1), p->queue_depth-qavail, (long long int)((opmax*100)/p->target_ops));
		
}
	
			// Release the QThread to let it start working on this task
			xdd_barrier(&qp->qthread_targetpass_wait_barrier,&p->occupant,0);

		} // End of WHILE loop that transfers data for a single pass

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

	// Make sure the QThread does not think the pass is complete
	qp->pass_complete = 0;

	if (qp->target_options & TO_ENDTOEND) { // For End-to-End Operations
		qp->e2e_msg_sequence_number = p->e2e_msg_sequence_number;
		p->e2e_msg_sequence_number++;
	} 
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

	// Set up the correct QThread to wait for if Strict or Loose ordering is in effect
	if ((p->target_options & TO_STRICT_ORDERING) || 
		(p->target_options & TO_LOOSE_ORDERING)) { 
		qp->qthread_to_wait_for = p->last_qthread_assigned;
	} else { // No ordering restrictions 
		qp->qthread_to_wait_for = NULL;
	}

} // End of xdd_targetpass_task_setup()

/*----------------------------------------------------------------------*/
/* xdd_targetpass_end_of_pass() - Force all the QThreads to set their pass_complete
 * indicator which causes them to enter the targetpass_qthread_passcomplete
 * barrier.
 */
void
xdd_targetpass_end_of_pass(ptds_t *p) {
	ptds_t		*qp;			// QThread pointer 
	int			status;			// Status of the sem_wait system calls

	// Get pointer to FIRST QThread on the chain
	qp = p->next_qp;
	while (qp) {
		// Wait for this specific QThread to become available
		p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		status = sem_wait(&qp->this_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_targetpass_eof_processing: Target %d: WARNING: Bad status from sem_wait on this_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
	
		qp->task_request = TASK_REQ_END_OF_PASS;

		// This operation will occur in parallel with no dependence on the ordering
		qp->qthread_to_wait_for = NULL;
	
		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_barrier,&p->occupant,0);
	
		// Move on to the next QThread
		qp = qp->next_qp; 		// Address of the next QThread on this Target Thread

	} // End of WHILE loop that transfers data for a single pass

	// At this point each of the QThreads have been told to enter the targetpass_qthread_passcomplete barrier...

	return;
} /* end of xdd_targetpass_end_of_pass() */


