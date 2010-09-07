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
 * This file contains the subroutines used by target_pass() that are specific
 * to an End-to-End (E2E) operation.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_loop() - This subroutine will manage assigning tasks to
 * QThreads during an E2E operation but only on the destination side of an
 * E2E operation. 
 * Called from xdd_targetpass()
 * 
 */
void
xdd_targetpass_e2e_loop(ptds_t *p) {
	ptds_t	*qp;

	// The idea is to keep all the QThreads busy reading whatever is sent to them
	// from their respective QThreads on the Source Side.
	// The "normal" targetpass task assignment loop counts down the number
	// of bytes it has assigned to be transferred until the number of bytes remaining
	// becomes zero.
	// This task assignment loop is based on the availability of QThreads to perform
	// I/O tasks - namely a recvfrom/write task which is specific to the 
	// Destination Side of an E2E operation. Each QThread will continue to perform
	// these recvfrom/write tasks until it receives an End-of-File (EOF) packet from 
	// the Source Side. At this point that QThread remains "unavailable" but also 
	// turns on its "EOF" flag to indicate that it has received an EOF. It will also
	// enter the targetpass_qthread_passcomplete barrier so that by the time this 
	// subroutine returns, all the QThreads will be at the targetpass_qthread_passcomplete
	// barrier.
	//
	// This routine will keep requesting QThreads from the QThread Locator until it
	// returns "zero" which indicates that all the QThreads have received an EOF.
	// At that point this routine will return.

	// Get the first QThread pointer for this Target
	qp = xdd_get_next_available_qthread(p);

	//////////////////////////// BEGIN I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////
	while (qp) { 


		// Check to see if we've been canceled - if so, we need to leave this loop
		if (xgp->canceled) 
			break;
	
		// Make sure the QThread does not think the pass is complete
		qp->task_request = TASK_REQ_IO;
		qp->my_current_op_type = OP_TYPE_WRITE;
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
			p->ttp->tte[qp->ts_current_entry].op_type = OP_TYPE_WRITE;
			p->ttp->tte[qp->ts_current_entry].op_number = -1; 		// to be filled in after data received
			p->ttp->tte[qp->ts_current_entry].byte_location = -1; 	// to be filled in after data received
			p->ttp->tte[qp->ts_current_entry].bytes_xferred = 0; 	// to be filled in after data received
		}

		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_barrier,&p->occupant,0);
	
		// Update the pointers/counters in the Target PTDS to get ready for the next I/O operation
		//p->last_qthread_assigned = qp;

		p->my_current_op_number++;
		// Get another QThread and lets keepit roling...
		qp = xdd_get_next_available_qthread(p);
	
	} // End of WHILE loop
	//////////////////////////// END OF I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////

} // End of xdd_targetpass_e2e_loop()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_eof_source_side() - This subroutine will manage End-Of-File
 * processing for an End-to-End operation on the source side only.
 * This subroutine will cycle through all the QThreads for a specific Target Thread.
 * Upon completion of this routine all the QThreads on the SOURCE side will have
 * been given a task to send an EOF packet to their corresponding QThread on the
 * Destination side.
 *
 * We need to issue an End-of-File task for this QThread so that it sends an EOF
 * packet to the corresponding QThread on the Destination Side. We do not have to wait for
 * that operation to complete. Just cycle through all the QThreads and later wait at the 
 * targetpass_qthread_passcomplete barrier. 
 *
 */
void
xdd_targetpass_eof_source_side(ptds_t *p) {
	ptds_t	*qp;
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
	
		qp->task_request = TASK_REQ_EOF;

		// Make sure the QThread does not think the pass is complete
		qp->pass_complete = 0;
	
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
		p->ttp->tte[qp->ts_current_entry].op_type = OP_TYPE_EOF;
		p->ttp->tte[qp->ts_current_entry].op_number = -1*qp->my_qthread_number;
		p->ttp->tte[qp->ts_current_entry].byte_location = -1;
		}
	
		// Set up the correct QThread to wait for if Strict or Loose ordering is in effect
		if ((p->target_options & TO_STRICT_ORDERING) || 
			(p->target_options & TO_LOOSE_ORDERING)) { 
			qp->qthread_to_wait_for = p->last_qthread_assigned;
		} else { // No ordering restrictions 
			qp->qthread_to_wait_for = NULL;
		}
	
		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_barrier,&p->occupant,0);
	
		// Move on to the next QThread
		qp = qp->next_qp; 		// Address of the next QThread on this Target Thread

	} // End of WHILE loop that transfers data for a single pass

	// At this point each of the QThreads on the Source Side should have sent an
	// EOF packet to each of the corresponding QThreads on the Destination Side and
	// each of the Destination Side QThreads will have received the EOF packet.

} // End of xdd_targetpass_eof_source_side()

