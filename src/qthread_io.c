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


//******************************************************************************
// I/O Operation
//******************************************************************************

/*----------------------------------------------------------------------------*/
/* xdd_qthread_io() - This subroutine is called by qthread() when it is
 * given an I/O Task Request. This subroutine will perform the requested I/O
 * operation and update the local and Target PTDS counters and timers.
 * It will also perform all the post-IO checks.
 * NOTE: The xdd_io_for_os() subroutine is OS-specific and the source lives
 * in xdd_io_for_os.c as part of this distribution.
 */
void
xdd_qthread_io(ptds_t *qp) {
	int32_t		status;			// Status of various subroutine calls

	// Do the things that need to get done before the I/O is started
	status = xdd_qthread_ttd_before_io_op(qp);
	if (status) { // Must be a problem is status is anything but zero
		fprintf(xgp->errout,"\n%s: xdd_qthread_io: Target %d QThread %d: ERROR: Canceling run due to previous error\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number);
		xgp->canceled = 1; // Need to terminate early
	}

	// Wait for the previous QThread I/O operation to complete if requested
	xdd_qthread_wait_for_previous_qthread(qp);
	
	// Check to see if we have been canceled - if so, then we need to 
	// release the next I/O and just return without performing this I/O.
	// Subsequent QThreads will do the same...
	if (xgp->canceled) {
		// Release the next QThread I/O if requested
		xdd_qthread_release_next_qthread(qp);
		return;
	}
	// Call the OS-appropriate IO routine to perform the I/O
	qp->my_current_state |= CURRENT_STATE_IO;
	xdd_io_for_os(qp);
	qp->my_current_state &= ~CURRENT_STATE_IO;

	// Release the next QThread I/O if requested
	xdd_qthread_release_next_qthread(qp);

	// Update counters and status in this QThread's PTDS
	xdd_qthread_update_local_counters(qp);

// FLUSHWRITE ??????????????????? TMR-TTD
	// Check status of the last operation 
	xdd_qthread_check_io_status(qp);

	// Check I/O operation completion
	xdd_qthread_ttd_after_io_op(qp);

	// Update the Target's PTDS counters and timers
	xdd_qthread_update_target_counters(qp);

	qp->qthread_to_wait_for = NULL; // Clear this out

} // End of xdd_qthread_io()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_wait_for_previous_qthread() - This subroutine will check to
 * see if we need to wait for the previous QThread I/O to complete and 
 * enter the appropriate semaphore if necessary. Otherwise, this routine 
 * will simple return.
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_qthread_wait_for_previous_qthread(ptds_t *qp) {
	int32_t 	status;


	// Wait for the QThread ahead of this one to complete (if necessary)
	if (!(qp->target_options & TO_NO_POC_SEMAPHORE)) {
		if (qp->qthread_to_wait_for) {
			qp->my_current_state |= CURRENT_STATE_WAITING_FOR_PREVIOUS_QTHREAD;
			status = sem_wait(&qp->qthread_to_wait_for->qthread_task_complete);
			if (status) {
				fprintf(xgp->errout,"%s: xdd_qthread_wait_for_previous_qthread: Target %d QThread %d: ERROR: Bad status from sem_wait: status=%d, errno=%d\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					status,
					errno);
				return(-1);	
			}
			qp->my_current_state &= ~CURRENT_STATE_WAITING_FOR_PREVIOUS_QTHREAD;
		}
	}
	return(0);
} // End of xdd_qthread_wait_for_previous_qthread()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_release_next_qthread() - This subroutine will check to
 * see if we need to release the next QThread that might be waiting for
 * this QThread to complete. 
 * 
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_qthread_release_next_qthread(ptds_t *qp) {
	int32_t 	status;


	// Increment the "qthread_task_complete" semaphore to let the next QThread run if necessary
	if (!(qp->target_options & TO_NO_POC_SEMAPHORE)) {
		status = sem_post(&qp->qthread_task_complete);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_qthread_release_next_qthread: Target %d QThread %d: ERROR: Bad status from sem_post: status=%d, errno=%d\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number,
				status,
				errno);
		return(-1);
		}
	}
	return(0);
} // End of xdd_qthread_release_next_qthread()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_update_local_counters() - This subroutine will update the local
 * PTDS counters and timers in this QThread's PTDS
 * 
 */
void
xdd_qthread_update_local_counters(ptds_t *qp) {

	qp->my_current_op_elapsed_time = (qp->my_current_op_end_time - qp->my_current_op_start_time);
	qp->my_accumulated_op_time += qp->my_current_op_elapsed_time;
	qp->my_current_io_errno = errno;
	if (qp->my_current_io_status == qp->my_current_io_size) {
		qp->my_current_bytes_xfered_this_op = qp->my_current_io_size;
		qp->my_current_bytes_xfered += qp->my_current_io_size;
		qp->my_current_op_count++;
		// Operation-specific counters
		switch (qp->my_current_op_type) { 
			case OP_TYPE_READ: 
				qp->my_accumulated_read_op_time += qp->my_current_op_elapsed_time;
				qp->my_current_bytes_read += qp->my_current_io_size;
				qp->my_current_read_op_count++;
				break;
			case OP_TYPE_WRITE: 
				qp->my_accumulated_write_op_time += qp->my_current_op_elapsed_time;
				qp->my_current_bytes_written += qp->my_current_io_size;
				qp->my_current_write_op_count++;
				break;
			case OP_TYPE_NOOP: 
				qp->my_accumulated_noop_op_time += qp->my_current_op_elapsed_time;
				qp->my_current_bytes_noop += qp->my_current_io_size;
				qp->my_current_noop_op_count++;
				break;
		} // End of SWITCH
	}
} // End of xdd_qthread_update_local_counters()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_update_target_counters() - This subroutine will update the Target
 * PTDS counters and timers 
 * NOTE: This routine locks the Target PTDS so that no other QThreads can be
 * making simultaneous updates. The lock is only held in this routine.
 */
void
xdd_qthread_update_target_counters(ptds_t *qp) {
	ptds_t	*p;			// Pointer to the Tartget's PTDS


	// Get the pointer to the Target's PTDS
	p = qp->target_ptds;

	// The following section gets the "counter_mutex" of the Target PTDS 
	// in order to update the various counters and timers.
	// It will also check to see if this was the last I/O operation for this
	// Target in which case it will set the "pass_complete" flag in the local 
	// QThread PTDS and the Target PTDS.
	// LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK
	pthread_mutex_lock(&p->counter_mutex);
	// Update counters and status in the QThread PTDS
	p->my_accumulated_op_time += qp->my_current_op_elapsed_time;
	if (qp->my_current_io_status == qp->my_current_io_size) {
		p->my_current_bytes_xfered_this_op = qp->my_current_bytes_xfered_this_op;
		p->bytes_completed += qp->my_current_bytes_xfered_this_op;
		p->my_current_bytes_xfered += qp->my_current_bytes_xfered_this_op;
		p->my_current_op_count++;
		p->e2e_sr_time += qp->e2e_sr_time; // E2E Send/Receive Time
		// Operation-specific counters
		switch (qp->my_current_op_type) { 
			case OP_TYPE_READ: 
				p->my_accumulated_read_op_time += qp->my_current_op_elapsed_time;
				p->my_current_bytes_read += qp->my_current_io_size;
				p->my_current_read_op_count++;
				break;
			case OP_TYPE_WRITE: 
				p->my_accumulated_write_op_time += qp->my_current_op_elapsed_time;
				p->my_current_bytes_written += qp->my_current_io_size;
				p->my_current_write_op_count++;
				break;
			case OP_TYPE_NOOP: 
				p->my_accumulated_noop_op_time += qp->my_current_op_elapsed_time;
				p->my_current_bytes_noop += qp->my_current_io_size;
				p->my_current_noop_op_count++;
				break;
			default:
				break;
		} // End of SWITCH
	}
	p->my_current_error_count += qp->my_current_error_count;
	if (p->bytes_completed >= p->target_bytes_to_xfer_per_pass) {  // This means that this was the last I/O operation and we are DONE
		p->pass_complete = 1;
		qp->pass_complete = 1;
	}
	pthread_mutex_unlock(&p->counter_mutex);
	// UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED
//fprintf(stderr,"bytes_xfered_this_op=%lld, bytes_completed=%lld, current_bytes_xfered=%lld, current_op_count=%lld, pass_complete=%d\n",(long long int)p->my_current_bytes_xfered_this_op, (long long int)p->bytes_completed, (long long int)p->my_current_bytes_xfered, (long long int)p->my_current_op_count,p->pass_complete);

} // End of xdd_qthread_update_target_counters()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_check_io_status() - This subroutine will check the status of 
 * the most recent I/O operation for the given QThread PTDS 
 */
void
xdd_qthread_check_io_status(ptds_t *qp) {

	qp->my_current_error_count = 0;
	if ((qp->my_current_io_status < 0) || (qp->my_current_io_status != qp->my_current_io_size)) {
		fprintf(xgp->errout, "%s: I/O ERROR on Target %d QThread %d: ERROR: Status %d, I/O Transfer Size %d, %s Operation Number %lld\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->my_current_io_status,
			qp->my_current_io_size,
			qp->my_current_op_str,
			(long long)qp->target_op_number);
		if (!(qp->target_options & TO_SGIO)) {
			if ((qp->my_current_io_status == 0) && (errno == 0)) { // Indicate this is an end-of-file condition
				fprintf(xgp->errout, "%s: Target %d QThread %d: WARNING: END-OF-FILE Reached during %s Operation Number %lld\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					qp->my_current_op_str,
					(long long)qp->target_op_number);
			} else {
				perror("reason"); // Only print the reason (aka errno text) if this is not an SGIO request
			}
		}
		qp->my_current_error_count = 1;
	} // Done checking status
} // End of xdd_qthread_check_io_status()
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
