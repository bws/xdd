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
 * in qthread_io_for_os.c as part of this distribution.
 */
void
xdd_qthread_io(ptds_t *qp) {
	int32_t		status;			// Status of various subroutine calls


	// Do the things that need to get done before the I/O is started
	// If this is the Destination Side of an End-to-End (E2E) operation, the xdd_qthread_ttd_before_io_op()
	// subroutine will perform the "recvfrom()" operation to get the data from the Source Side
	// of the E2E operation.
	status = xdd_qthread_ttd_before_io_op(qp);
	if (status) { // Must be a problem is status is anything but zero
		fprintf(xgp->errout,"\n%s: xdd_qthread_io: Target %d QThread %d: ERROR: Canceling run due to previous error\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number);
		xgp->canceled = 1; // Need to terminate early
	}


	// If this is the Destination Side of an E2E operation and this is an End-of-File Packet 
	// there is no data to write to the storage so just set the "EOF_RECEIVED" bit in the
	// qthread_target_sync flags so that the Target Thread knows that this QThread is
	// done receiving data from the Source Side.
	// If there is Loose or Serial Ordering in effect then we need to release the Next QThread
	// before returning.
	if ((qp->target_options & TO_E2E_DESTINATION) && (qp->e2e_header.magic == PTDS_E2E_MAGIQ)) { 
		// Indicate that this QThread has received its EOF Packet
		pthread_mutex_lock(&qp->qthread_target_sync_mutex);
		qp->qthread_target_sync |= QTSYNC_EOF_RECEIVED;
		pthread_mutex_unlock(&qp->qthread_target_sync_mutex);

		// Release the next QThread I/O 
		if ((qp->target_options & TO_SERIAL_ORDERING) || (qp->target_options & TO_LOOSE_ORDERING)) 
			 xdd_qthread_release_next_io(qp);

		return;
	}
		
	// Wait for the Previous I/O to release this QThread if Serial or Loose Ordering is in effect.
	// It is important to note that for Serial Ordering, when we get released by the QThread that performed
	// the previous I/O operation and we are gauranteed that the previous I/O operation has completed.
	// However, for Loose Ordering, we get released just *before* the previous QThread actually performs its I/O 
	// operation. Therefore, in order to ensure that the previous QThread's I/O operation actually completes [properly], 
	// we need to wait again *after* we have completed our I/O operation for the previous QThread to 
	// release us *after* it completes its operation. 
	// Man I hope this works...
	if ((qp->target_options & TO_SERIAL_ORDERING) || (qp->target_options & TO_LOOSE_ORDERING)) {
		xdd_qthread_wait_for_previous_io(qp);
	}
	
	// Check to see if we have been canceled - if so, then we need to release the next QThread
	// if there is any ordering involved.
	// Subsequent QThreads will do the same...
	if (xgp->canceled) {
		// Release the Next QThread I/O if requested
		if ((qp->target_options & TO_SERIAL_ORDERING) || (qp->target_options & TO_LOOSE_ORDERING)) 
			 xdd_qthread_release_next_io(qp);
		return;
	}

	// If Loose Ordering is in effect then release the Next QThread so that it can start
	if (qp->target_options & TO_LOOSE_ORDERING) 
		xdd_qthread_release_next_io(qp);

	// Call the OS-appropriate IO routine to perform the I/O
	qp->my_current_state |= CURRENT_STATE_IO;
	xdd_io_for_os(qp);
	qp->my_current_state &= ~CURRENT_STATE_IO;

	// Update counters and status in this QThread's PTDS
	xdd_qthread_update_local_counters(qp);

	// Update the Target's PTDS counters and timers and the TOT
	xdd_qthread_update_target_counters(qp);

	// If Loose or Serial Ordering is in effect then we need to release the Next QThread.
	// For Loose Ordering, the Next QThread has issued its I/O operation and it may have completed
	// in which case the Next QThread is waiting for us to release it so that it can continue.
	// For Serial Ordering, the Next QThread has not issued its I/O operation yet because it is 
	// waiting for us to release it.
	if ((qp->target_options & TO_LOOSE_ORDERING) || (qp->target_options & TO_SERIAL_ORDERING))
		xdd_qthread_release_next_io(qp);

	// Check I/O operation completion
	// If this is the Source Side of an End-to-End (E2E) operation, the xdd_qthread_ttd_after_io_op()
	// subroutine will perform the "sendto()" operation to send the data that was just read from storage
	// over to the Destination Side of the E2E operation. 
	xdd_qthread_ttd_after_io_op(qp);

} // End of xdd_qthread_io()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_wait_for_previous_io() - This subroutine will wait for
 * the previous QThread I/O to complete.
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_qthread_wait_for_previous_io(ptds_t *qp) {
	int32_t 	status;
	ptds_t		*p;				// Pointer to the Target PTDS for this QThread
	int32_t		tot_offset;		// Offset into the TOT
	tot_entry_t	*tep;			// Pointer to the TOT entry to use


	p = qp->target_ptds;
	// Wait for the I/O operation ahead of this one to complete (if necessary)
	tot_offset = ((qp->my_current_byte_location/p->iosize) % p->totp->tot_entries) - 1;
	if (tot_offset < 0) 
		tot_offset = p->totp->tot_entries - 1; // The last TOT_ENTRY
	
	if (qp->target_options & TO_E2E_DESTINATION) {
		if (qp->my_current_op_number == 0)
		return(0);	// Dont need to wait for op minus 1 ;)
	} else {
		if (qp->target_op_number == 0)
			return(0);	// Dont need to wait for op minus 1 ;)
	}


	tep = &p->totp->tot_entry[tot_offset];
	qp->my_current_state |= CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_TS;
	pthread_mutex_lock(&tep->tot_mutex);
	qp->my_current_state &= ~CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_TS;
	pclk_now(&tep->tot_wait_ts);
	tep->tot_wait_qthread_number = qp->my_qthread_number;
	pthread_mutex_unlock(&tep->tot_mutex);
	qp->my_current_state |= CURRENT_STATE_QT_WAITING_FOR_PREVIOUS_IO;
	status = sem_wait(&tep->tot_sem);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_wait_for_previous_io: Target %d QThread %d: ERROR: Bad status from sem_wait: status=%d, errno=%d, target_op_number=%lld, tot_offset=%d\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			status,
			errno,
			(long long int)qp->target_op_number,
			tot_offset);
		return(-1);	
	}
	qp->my_current_state &= ~CURRENT_STATE_QT_WAITING_FOR_PREVIOUS_IO;

	return(0);
} // End of xdd_qthread_wait_for_previous_io()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_release_next_io() - This subroutine will check to
 * see if we need to release the next QThread that might be waiting for
 * this QThread to complete. 
 * 
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_qthread_release_next_io(ptds_t *qp) {
	int32_t 	status;
	ptds_t		*p;				// Pointer to the Target PTDS for this QThread
	int32_t		tot_offset;		// Offset into the TOT
	tot_entry_t	*tep;			// Pointer to the TOT entry to use


	p = qp->target_ptds;
	tot_offset = ((qp->my_current_byte_location/p->iosize) % p->totp->tot_entries);

	// Wait for the I/O operation ahead of this one to complete (if necessary)

	tep = &p->totp->tot_entry[tot_offset];
	qp->my_current_state |= CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_RELEASE;
	pthread_mutex_lock(&tep->tot_mutex);
	qp->my_current_state &= ~CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_RELEASE;
	tep->tot_post_qthread_number = qp->my_qthread_number;
	pclk_now(&tep->tot_post_ts);
	
	pthread_mutex_unlock(&tep->tot_mutex); //TMR
	// Increment the specified semaphore to let the next QThread run 
	status = sem_post(&tep->tot_sem);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_release_next_io: Target %d QThread %d: ERROR: Bad status from sem_post: status=%d, errno=%d, target_op_number=%lld, tot_offset=%d\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			status,
			errno,
			(long long int)qp->target_op_number,
			tot_offset);
//TMR		pthread_mutex_unlock(&tep->tot_mutex);
		return(-1);
	}
//TMR	pthread_mutex_unlock(&tep->tot_mutex);
	return(0);
} // End of xdd_qthread_release_next_io()

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
	qp->my_current_error_count = 0;
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
	} else {// ((qp->my_current_io_status < 0) || (qp->my_current_io_status != qp->my_current_io_size)) 
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
	int32_t		tot_offset;		// Offset into the TOT
	tot_entry_t	*tep;			// Pointer to the TOT entry to use


	// Get the pointer to the Target's PTDS
	p = qp->target_ptds;

	///////////////////////////////////////////////////////////////////////////////////	
	// Update the TOT entry for this last I/O
	// Since the TOT is a resource owned by the Target Thread and shared by the QThreads
	// it will be updated here.
	// Calculate the TOT Offset
	tot_offset = ((qp->my_current_byte_location/p->iosize) % p->totp->tot_entries);
	// Get a pointer to the correct TOT Entry
	tep = &p->totp->tot_entry[tot_offset];
	// LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK
	// Lock this entry 
	qp->my_current_state |= CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_UPDATE;
	pthread_mutex_lock(&tep->tot_mutex);
	qp->my_current_state &= ~CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_UPDATE;
	// Record the update time, byte_location, and io_size for this I/O
	if ((p->target_options & (TO_SERIAL_ORDERING | TO_LOOSE_ORDERING)) &&  
		(tep->tot_byte_location >= qp->my_current_byte_location)) { // This means there is a real collision
		fprintf(xgp->errout, "%s: qthread_io: Target %d QThread %d: INTERNAL ERROR: TOT Collision at entry %d byte location is %lld [block %lld] my byte location is %lld [block %lld] last updated by qthread %d\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			tot_offset,
			(long long int)tep->tot_byte_location,
			(long long int)(tep->tot_byte_location / (long long int)(p->iosize)),
			(long long)qp->my_current_byte_location,
			(long long int)(qp->my_current_byte_location / (long long int)(p->iosize)),
			tep->tot_update_qthread_number);
	}  else { // Only update if there was no collision or if there is no ordering in which case collisons do not matter
		pclk_now(&tep->tot_update_ts);
		tep->tot_update_qthread_number = qp->my_qthread_number;
		tep->tot_byte_location = qp->my_current_byte_location;
		tep->tot_io_size = qp->my_current_io_size;
	}
	// Unlock this entry 
	pthread_mutex_unlock(&tep->tot_mutex);
	// UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED

	///////////////////////////////////////////////////////////////////////////////////	
	// The following section gets the "counter_mutex" of the Target PTDS 
	// in order to update the various counters and timers.
	//
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
		if ( (qp->target_options & TO_ENDTOEND) && (qp->target_options & TO_E2E_DESTINATION)) {
			if ( qp->first_pass_start_time < p->first_pass_start_time)  // Record the proper *First* pass start time
				p->first_pass_start_time = qp->first_pass_start_time;
			if ( qp->my_pass_start_time < p->my_pass_start_time)  // Record the proper PASS start time
				p->my_pass_start_time = qp->my_pass_start_time;
		}
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

	pthread_mutex_unlock(&p->counter_mutex);
	// UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED

} // End of xdd_qthread_update_target_counters()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
