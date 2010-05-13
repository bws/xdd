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
/* xdd_get_next_available_qthread() - This subroutine will scan the list of
 * QThreads and return the next one available for a task.
 * This subroutine will block until a QThread becomes available as indicated
 * by the qthread_available semaphore in a QThread PTDS.
 * The input parameter is a pointer to the Target Thread PTDS that owns 
 * the chain of QThreads to be scanned.
 */
ptds_t *
xdd_get_next_available_qthread(ptds_t *p) {
	ptds_t		*qp;			// Pointer to a QThread PTDS
	int			status;			// Status of the sem_wait system calls

	if (p->target_options & TO_NO_POC_SEMAPHORE) {
		// Wait for any QThread to become available
		p->my_current_state |= CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
		status = sem_wait(&p->any_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_get_next_available_qthread: Target %d: WARNING: Bad status from sem_post on any_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
		qp = p->next_qp;
		while (qp) {
			if (qp->my_current_state & CURRENT_STATE_THIS_QTHREAD_IS_AVAILABLE)
				break;
			qp = qp->next_qp;
			if (qp == NULL) {
				fprintf(stderr, "xdd_get_next_available_qthread: Target %d: no available QThreads found but there should be one here... waiting on any_qthread_available semaphore\n",p->my_target_number);
				p->my_current_state |= CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
				status = sem_wait(&p->any_qthread_available);
				p->my_current_state &= ~CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
				qp = p->next_qp;
			}
		}
	} else { // Use Strict Ordering
		qp = p->next_qthread_to_use;
		if (qp->next_qp) // Is there another QThread on this chain after the current one?
			p->next_qthread_to_use = qp->next_qp; // point to the next QThread in the chain
		else p->next_qthread_to_use = p->next_qp; // else point to the first QThread in the chain
		// Wait for this specific QThread to become available
		p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		status = sem_wait(&qp->this_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_get_next_available_qthread: Target %d: WARNING: Bad status from sem_post on this_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
	} // End of Strict Ordering QThread locator

	// Mark this QThread NOT Available
	qp->my_current_state &= ~CURRENT_STATE_THIS_QTHREAD_IS_AVAILABLE; 

	return(qp);
} // End of xdd_get_next_available_qthread()

/*----------------------------------------------------------------------------*/
/* xdd_target_pass() - This subroutine will perform a single pass
 */
int32_t
xdd_target_pass(ptds_t *p) {
	ptds_t		*qp;			// Pointer to a QThread's PTDS
	int			status;			// Status of the sem_wait system calls

	/* Before we get started, check to see if we need to reset the 
	 * run_status in case we are using the start trigger.
	 */
	if (p->target_options & TO_WAITFORSTART) 
		p->run_status = 0;

	// This barrier is to ensure that all TARGETS start at same time 

	xdd_barrier(&xgp->results_targets_startpass_barrier,&p->occupant,0);

	/* Check to see if any of the other threads have aborted */
	if (xgp->abort) {
		fprintf(xgp->errout,"%s: ERROR: xdd_target_pass: Target number %d name '%s' Aborting due to failure with another target\n",
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

//////////////////////////// BEGIN I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////
	while (p->bytes_remaining) {
		// Things to do before an I/O is issued
		xdd_target_ttd_before_io_op(p);
		// Get pointer to next QThread to issue a task to
		qp = xdd_get_next_available_qthread(p);

		// Check to see if we've been canceled - if so, we need to leave this loop
		if (xgp->canceled) {
			// Make this qthread available so that it will get through End-of-Pass checking
			sem_post(&qp->this_qthread_available);
			break;
		}

		// Set up the next task
		xdd_target_pass_task_setup(qp);

		// Update the pointers/counters in the Target PTDS to get ready for the next I/O operation
		p->my_current_byte_location += qp->my_current_io_size;
		p->my_current_op_number++;
		p->bytes_issued += qp->my_current_io_size;
		p->bytes_remaining -= qp->my_current_io_size;
		p->last_qthread_assigned = qp;

		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_barrier,&p->occupant,0);

	} // End of WHILE loop that transfers data for a single pass
///////////////////////////// END I/O LOOP FOR ENTIRE PASS ////////////////////////////////////////////

	// At this point the pass is complete -or- we got CANCELED and ended the pass prematurely
	// In either case, there may be QThreads that have been assigned tasks and are still in the
	// process of completing them. Therefore, we need to wait here until all the outstanding
	// QThreads have finished their respective tasks.
	// Cycle through all the QThreads and assign an "EndOfPass" task to each and enter their respective
	// qthread_target_pass_wait barrier to release them to perform that special task. This loop will
	// ensure that all QThreads have completed and the pass will thus be complete.
	qp = p->next_qp;
	while (qp) {
		// Wait for this thread to become available - meaning it has finished its last assigned task
		p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		status = sem_wait(&qp->this_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_target_pass: Target %d: WARNING: Bad status from sem_post on this_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;

		// Make this thread available 
		qp->my_current_state |= CURRENT_STATE_THIS_QTHREAD_IS_AVAILABLE;
		status = sem_post(&qp->this_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_target_pass: Target %d QThread %d: WARNING: Bad status from sem_post on qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number,
				status,
				errno);
		}
		qp = qp->next_qp;
	}

	// At this point all the QThreads have completed and the pass is officially complete.
	// All the QThreads should be waiting on their respective qthread_targetpass_wait_barrier 

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
} // end of xdd_target_pass()

/*----------------------------------------------------------------------------*/
/* xdd_target_pass_task_setup() - This subroutine will set up the task info for an I/O
 */
void
xdd_target_pass_task_setup(ptds_t *qp) {
	ptds_t	*p;

	p = qp->target_ptds;
	// Assign an IO task to this qthread
	qp->task_request = TASK_REQ_IO;

	// Make sure the QThread does not think the pass is complete
	qp->pass_complete = 0;

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

	if (p->target_options & TO_ENDTOEND) {
		qp->e2e_msg_sequence_number = p->e2e_msg_sequence_number;
		p->e2e_msg_sequence_number++;
	}

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
	}
	// Set up the correct QThread to wait for if required
	if (p->target_options & TO_NO_POC_SEMAPHORE) {
		qp->qthread_to_wait_for = NULL;
	} else {
		qp->qthread_to_wait_for = p->last_qthread_assigned;
	}

} // End of xdd_target_pass_task_setup()

