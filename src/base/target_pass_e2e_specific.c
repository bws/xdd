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
 * This file contains the subroutines used by target_pass() or targetpass_loop() 
 * that are specific to an End-to-End (E2E) operation.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_loop_dst() - This subroutine will manage assigning tasks to
 * QThreads during an E2E operation but only on the destination side of an
 * E2E operation. 
 * Called from xdd_targetpass()
 * 
 */
void
xdd_targetpass_e2e_loop_dst(ptds_t *p) {
	ptds_t	*qp;
	int		q;

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
	qp = xdd_get_any_available_qthread(p);

	//////////////////////////// BEGIN I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////
	while (qp) { 

		// Check to see if we've been canceled - if so, we need to leave this loop
		if ((xgp->canceled) || (xgp->abort) || (p->abort)) {
			// When we got this QThread the QTSYNC_BUSY flag was set by get_any_available_qthread()
			// We need to reset it so that the subsequent loop will find it with get_specific_qthread()
			// Normally we would get the mutex lock to do this update but at this point it is not necessary.
			qp->qthread_target_sync &= ~QTSYNC_BUSY;
			break;
		}

	
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
			p->ttp->tte[qp->ts_current_entry].thread_id     = qp->my_thread_id;
			p->ttp->tte[qp->ts_current_entry].op_type = OP_TYPE_WRITE;
			p->ttp->tte[qp->ts_current_entry].op_number = -1; 		// to be filled in after data received
			p->ttp->tte[qp->ts_current_entry].byte_location = -1; 	// to be filled in after data received
			p->ttp->tte[qp->ts_current_entry].disk_xfer_size = 0; 	// to be filled in after data received
			p->ttp->tte[qp->ts_current_entry].net_xfer_size = 0; 	// to be filled in after data received
		}

		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_for_task_barrier,&p->occupant,0);
	
		p->my_current_op_number++;
		// Get another QThread and lets keepit roling...
		qp = xdd_get_any_available_qthread(p);
	
	} // End of WHILE loop
	//////////////////////////// END OF I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////

	// Check to see if we've been canceled - if so, we need to leave 
	if (xgp->canceled) {
		fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
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
		// Check to see if we've been canceled - if so, we need to leave 
		if (xgp->canceled) {
			fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
				xgp->progname,
				p->my_target_number);
			break;
		}
	}

	return;

} // End of xdd_targetpass_e2e_loop_dst()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_loop_src() - This subroutine will assign tasks to QThreads until
 * all bytes have been processed. It will then issue an End-of-Data Task to 
 * all QThreads one at a time. The End-of-Data Task will send an End-of-Data
 * packet to the Destination Side so that those QThreads know that there
 * is no more data to receive.
 * 
 * This subroutine is called by xdd_targetpass().
 */
void
xdd_targetpass_e2e_loop_src(ptds_t *p) {
	ptds_t	*qp;
	int		q;


	while (p->bytes_remaining) {
		// Things to do before an I/O is issued
		xdd_target_ttd_before_io_op(p);
		// Check to see if either the pass or run time limit has expired - if so, we need to leave this loop
		if ((p->my_time_limit_expired) || (xgp->run_time_expired)) 
			break;

		// Get pointer to next QThread to issue a task to
		qp = xdd_get_any_available_qthread(p);

		// Check to see if we've been canceled - if so, we need to leave this loop
		if ((xgp->canceled) || (xgp->abort) || (p->abort)) {
			// When we got this QThread the QTSYNC_BUSY flag was set by get_any_available_qthread()
			// We need to reset it so that the subsequent loop will find it with get_specific_qthread()
			// Normally we would get the mutex lock to do this update but at this point it is not necessary.
			qp->qthread_target_sync &= ~QTSYNC_BUSY;
			break;
		}

		// Set up the task for the QThread
		xdd_targetpass_e2e_task_setup_src(qp);

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
		fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
			xgp->progname,
			p->my_target_number);
		return;
	}

	// Assign each of the QThreads an End-of-Data Task
	xdd_targetpass_e2e_eof_src(p);

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

} // End of xdd_targetpass_e2e_loop_src()
/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_task_setup_src() - This subroutine will set up the task info for an I/O
 */
void
xdd_targetpass_e2e_task_setup_src(ptds_t *qp) {
	ptds_t	*p;

	p = qp->target_ptds;
	// Assign an IO task to this qthread
	qp->task_request = TASK_REQ_IO;

	qp->e2e_msg_sequence_number = p->e2e_msg_sequence_number;
	p->e2e_msg_sequence_number++;

	if (p->seekhdr.seeks[p->my_current_op_number].operation == SO_OP_READ) // READ Operation
		qp->my_current_op_type = OP_TYPE_READ;
	else qp->my_current_op_type = OP_TYPE_NOOP;

	// Figure out the transfer size to use for this I/O
	// It will be either the normal I/O size (p->iosize) or if this is the
	// end of this file then the last transfer could be less than the
	// normal I/O size. 
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
		p->ttp->tte[qp->ts_current_entry].thread_id     = qp->my_thread_id;
		p->ttp->tte[qp->ts_current_entry].op_type = qp->my_current_op_type;
		p->ttp->tte[qp->ts_current_entry].op_number = qp->target_op_number;
		p->ttp->tte[qp->ts_current_entry].byte_location = qp->my_current_byte_location;
	}

} // End of xdd_targetpass_e2e_task_setup_src()

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
xdd_targetpass_e2e_eof_src(ptds_t *p) {
	ptds_t	*qp;
	int		q;

	for (q = 0; q < p->queue_depth; q++) {
		qp = xdd_get_specific_qthread(p,q);
		qp->task_request = TASK_REQ_EOF;

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
		p->ttp->tte[qp->ts_current_entry].thread_id     = qp->my_thread_id;
		p->ttp->tte[qp->ts_current_entry].op_type = OP_TYPE_EOF;
		p->ttp->tte[qp->ts_current_entry].op_number = -1*qp->my_qthread_number;
		p->ttp->tte[qp->ts_current_entry].byte_location = -1;
		}
	
		// Release the QThread to let it start working on this task
		xdd_barrier(&qp->qthread_targetpass_wait_for_task_barrier,&p->occupant,0);
	
	}
} // End of xdd_targetpass_eof_source_side()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_monitor() - This subroutine will monitor and display
 * information about the QThreads that are running on the Source Side of an
 * E2E operation.
 * 
 * This subroutine is called by xdd_targetpass_loop().
 */
void
xdd_targetpass_e2e_monitor(ptds_t *p) {
	ptds_t	*tmpqp;
	int qmax, qmin;
	int64_t opmax, opmin;
	int qavail;


	if ((p->my_current_op_number > 0) && ((p->my_current_op_number % p->queue_depth) == 0)) {
		qmin = 0;
		qmax = 0;
		opmin = p->target_ops;
		opmax = -1;
		qavail = 0;
		tmpqp = p->next_qp; // first QThread on the chain
		while (tmpqp) { // Scan the QThreads to determine the one furthest ahead and the one furthest behind
			if (tmpqp->qthread_target_sync & QTSYNC_BUSY) {
				if (tmpqp->target_op_number < opmin) {
					opmin = tmpqp->target_op_number;
					qmin = tmpqp->my_qthread_number;
				}
				if (tmpqp->target_op_number > opmax) {
					opmax = tmpqp->target_op_number;
					qmax = tmpqp->my_qthread_number;
				}
			} else {
				qavail++;
			}
			tmpqp = tmpqp->next_qp;
		}
		fprintf(stderr,"\n\nopmin %4lld, qmin %4d, opmax %4lld, qmax %4d, separation is %4lld, %4d qthreads busy, %lld percent complete\n\n",
			(long long int)opmin, qmin, (long long int)opmax, qmax, (long long int)(opmax-opmin+1), p->queue_depth-qavail, (long long int)((opmax*100)/p->target_ops));
	}
} // End of xdd_targetpass_e2e_monitor();

