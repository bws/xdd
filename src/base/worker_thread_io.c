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
#include <inttypes.h>
#include "xint.h"


//******************************************************************************
// I/O Operation
//******************************************************************************

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_io() - This subroutine is called by worker_thread() when it is
 * given an I/O Task Request. This subroutine will perform the requested I/O
 * operation and update the local and Target Data counters and timers.
 * It will also perform all the post-IO checks.
 * NOTE: The xdd_io_for_os() subroutine is OS-specific and the source lives
 * in worker_thread_io_for_os.c as part of this distribution.
 */
void
xdd_worker_thread_io(worker_data_t *wdp) {
	int32_t		status;			// Status of various subroutine calls
	target_data_t		*tdp;				// Pointer to the Target Data for this Worker Thread


	// Get the pointer to the Target's Data
	tdp = wdp->wd_tdp;

	// Do the things that need to get done before the I/O is started
	// If this is the Destination Side of an End-to-End (E2E) operation, the xdd_worker_thread_ttd_before_io_op()
	// subroutine will perform the "recvfrom()" operation to get the data from the Source Side
	// of the E2E operation.
	status = xdd_worker_thread_ttd_before_io_op(wdp);
	if (status) { // Must be a problem is status is anything but zero
		fprintf(xgp->errout,"\n%s: xdd_worker_thread_io: Target %d Worker Thread %d: ERROR: Canceling run due to previous error\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number);
		xgp->canceled = 1; // Need to terminate early
	}


	// If this is the Destination Side of an E2E operation and this is an End-of-File Packet 
	// there is no data to write to the storage so just set the "EOF_RECEIVED" bit in the
	// wd_worker_thread_target_sync flags so that the Target Thread knows that this Worker Thread is
	// done receiving data from the Source Side.
	// If there is Loose or Serial Ordering in effect then we need to release the Next Worker Thread
	// before returning.
	if ((tdp->td_target_options & TO_E2E_DESTINATION) && (wdp->wd_e2ep->e2e_header.magic == XDD_E2E_MAGIQ)) { 
		// Indicate that this Worker Thread has received its EOF Packet
		pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
		wdp->wd_worker_thread_target_sync |= WTSYNC_EOF_RECEIVED;
		pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);

		// Release the next Worker Thread I/O 
		if (tdp->td_target_options & (TO_ORDERING_STORAGE_SERIAL | TO_ORDERING_STORAGE_LOOSE)) 
			 xdd_worker_thread_release_next_io(wdp);

		return;
	}
		
	// Wait for the Previous I/O to release this Worker Thread if Serial or Loose Ordering is in effect.
	// It is important to note that for Serial Ordering, when we get released by the Worker Thread that performed
	// the previous I/O operation and we are gauranteed that the previous I/O operation has completed.
	// However, for Loose Ordering, we get released just *before* the previous Worker Thread actually performs its I/O 
	// operation. Therefore, in order to ensure that the previous Worker Thread's I/O operation actually completes [properly], 
	// we need to wait again *after* we have completed our I/O operation for the previous Worker Thread to 
	// release us *after* it completes its operation. 
	// Man I hope this works...
	if (tdp->td_target_options & (TO_ORDERING_STORAGE_SERIAL | TO_ORDERING_STORAGE_LOOSE)) {
		xdd_worker_thread_wait_for_previous_io(wdp);
	}
	
	// Check to see if we have been canceled - if so, then we need to release the next Worker Thread
	// if there is any ordering involved.
	// Subsequent Worker Threads will do the same...
	if ((xgp->canceled)  || (xgp->abort) || (tdp->td_abort)) {
		// Release the Next Worker Thread I/O if requested
		if (tdp->td_target_options & (TO_ORDERING_STORAGE_SERIAL | TO_ORDERING_STORAGE_LOOSE)) 
			 xdd_worker_thread_release_next_io(wdp);
		return;
	}

	// If Loose Ordering is in effect then release the Next Worker Thread so that it can start
	if (tdp->td_target_options & TO_ORDERING_STORAGE_LOOSE) 
		xdd_worker_thread_release_next_io(wdp);

	// Call the OS-appropriate IO routine to perform the I/O
	tdp->td_current_state |= CURRENT_STATE_IO;
	xdd_io_for_os(wdp);
	tdp->td_current_state &= ~CURRENT_STATE_IO;

	// Update counters and status in this Worker Thread's Data
	xdd_worker_thread_update_local_counters(wdp);

	// Update the Target's Data counters and timers and the TOT
	xdd_worker_thread_update_target_counters(wdp);

	// If Loose or Serial Ordering is in effect then we need to release the Next Worker Thread.
	// For Loose Ordering, the Next Worker Thread has issued its I/O operation and it may have completed
	// in which case the Next Worker Thread is waiting for us to release it so that it can continue.
	// For Serial Ordering, the Next Worker Thread has not issued its I/O operation yet because it is 
	// waiting for us to release it.
	if (tdp->td_target_options & (TO_ORDERING_STORAGE_SERIAL | TO_ORDERING_STORAGE_LOOSE)) 
		xdd_worker_thread_release_next_io(wdp);

	// Check I/O operation completion
	// If this is the Source Side of an End-to-End (E2E) operation, the xdd_worker_thread_ttd_after_io_op()
	// subroutine will perform the "sendto()" operation to send the data that was just read from storage
	// over to the Destination Side of the E2E operation. 
	xdd_worker_thread_ttd_after_io_op(wdp);

} // End of xdd_worker_thread_io()

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_wait_for_previous_io() - This subroutine will wait for
 * the previous Worker Thread I/O to complete.
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_worker_thread_wait_for_previous_io(worker_data_t *wdp) {
	target_data_t		*tdp;				// Pointer to the Target Data for this Worker Thread
	int32_t		tot_offset;		// Offset into the TOT
	tot_entry_t	*tep;			// Pointer to the TOT entry to use


	tdp = wdp->wd_tdp;
	// Wait for the I/O operation ahead of this one to complete (if necessary)
	tot_offset = (tdp->td_current_op_number % tdp->td_totp->tot_entries) - 1;
	if (tot_offset < 0) 
		tot_offset = tdp->td_totp->tot_entries - 1; // The last TOT_ENTRY
	
//	if (tdp->td_target_options & TO_E2E_DESTINATION) {
		if (tdp->td_current_op_number == 0)
		return(0);	// Dont need to wait for op minus 1 ;)
//	} else {
//		if (tdp->td_current_op_number == 0)
//			return(0);	// Dont need to wait for op minus 1 ;)
//	}


	tep = &tdp->td_totp->tot_entry[tot_offset];
	tdp->td_current_state |= CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS;
	pthread_mutex_lock(&tep->tot_mutex);
	tdp->td_current_state &= ~CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS;
	nclk_now(&tep->tot_wait_ts);
	tep->tot_wait_worker_thread_number = wdp->wd_thread_number;

	
	tdp->td_current_state |= CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO;
	while (1 != tep->is_released) {
	    pthread_cond_wait(&tep->tot_condition, &tep->tot_mutex);
	}
	tdp->td_current_state &= ~CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO;
	tep->is_released = 0;
	pthread_mutex_unlock(&tep->tot_mutex);

	return(0);
} // End of xdd_worker_thread_wait_for_previous_io()

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_release_next_io() - This subroutine will check to
 * see if we need to release the next Worker Thread that might be waiting for
 * this Worker Thread to complete. 
 * 
 * Return value of 0 is good, -1 indicates there was an error
 */
int32_t
xdd_worker_thread_release_next_io(worker_data_t *wdp) {
	int32_t 	status;
	target_data_t		*tdp;				// Pointer to the Target Data for this Worker Thread
	int32_t		tot_offset;		// Offset into the TOT
	tot_entry_t	*tep;			// Pointer to the TOT entry to use


	tdp = wdp->wd_tdp;
	tot_offset = (tdp->td_current_op_number % tdp->td_totp->tot_entries);

	// Wait for the I/O operation ahead of this one to complete (if necessary)

	tep = &tdp->td_totp->tot_entry[tot_offset];
	tdp->td_current_state |= CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE;
	pthread_mutex_lock(&tep->tot_mutex);
	tdp->td_current_state &= ~CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE;
	tep->tot_post_worker_thread_number = wdp->wd_thread_number;
	nclk_now(&tep->tot_post_ts);

	// Increment the specified semaphore to let the next Worker Thread run 
	tep->is_released = 1;
	pthread_mutex_unlock(&tep->tot_mutex); //TMR
	status = pthread_cond_signal(&tep->tot_condition);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_worker_thread_release_next_io: Target %d Worker Thread %d: ERROR: Bad status from sem_post: status=%d, errno=%d, target_op_number=%lld, tot_offset=%d\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			status,
			errno,
			(long long int)tdp->td_current_op_number,
			tot_offset);
		return(-1);
	}
	return(0);
} // End of xdd_worker_thread_release_next_io()

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_update_local_counters() - This subroutine will update the local
 * Data counters and timers in this Worker Thread's Data
 *
 * The worker_thread_io_for_os() subroutine issues an I/O operation and sets the
 * "td_counters.tc_current_io_status" 
 * member of this Worker Thread's Worker Data equal to whatever the
 * read/write system call returned. Under normal conditions the return value
 * from the read/write system call (or equivalent) should be the number of
 * bytes transferred. If there was an error during the transfer then the
 * return value should be -1 and "errno" for this Worker Thread will be set to
 * indicate roughly what happened (i.e. EIO). If the return value is zero 
 * then the read/write hit an end-of-file condition. A return value of 
 * anything between 1 and the number of bytes that were supposed to be 
 * transferred (minus 1) is considered an error. 
 * 
 * If the operation completed normally then the various counters in this
 * Worker Thread's Worker Data are updated appropriately. 
 * Otherwise, an error message is displayed with relevant information 
 * as to the time and location of the error. The my_current_error_count is
 * set to 1 to indicate an error condition to the Worker Thread and subsequently
 * the Target Thread so that they can take appropriate action. 
 */
void
xdd_worker_thread_update_local_counters(worker_data_t *wdp) {
	target_data_t	*tdp;			// Pointer to the Tartget's Data

	// Get the pointer to the Target's Data
	tdp = wdp->wd_tdp;

	wdp->wd_counters.tc_current_op_elapsed_time = (wdp->wd_counters.tc_current_op_end_time - wdp->wd_counters.tc_current_op_start_time);
	wdp->wd_counters.tc_accumulated_op_time += wdp->wd_counters.tc_current_op_elapsed_time;
	wdp->wd_counters.tc_current_io_errno = errno;
	wdp->wd_counters.tc_current_error_count = 0;
	if (wdp->wd_task.task_io_status == wdp->wd_task.task_xfer_size) { // Status is GOOD - update counters
//		wdp->wd_current_bytes_xfered_this_op = wdp->wd_task.task_xfer_size;
//		wdp->wd_current_bytes_xfered += wdp->wd_task.task_xfer_size;
//		wdp->wd_current_op_count++;
		// Operation-specific counters
//		switch (wdp->wd_current_op_type) { 
//			case TASK_OP_TYPE_READ: 
//				tdp->td_counters.tc_accumulated_read_op_time += tdp->td_current_op_elapsed_time;
//				tdp->td_current_bytes_read += wdp->wd_task.task_xfer_size;
//				tdp->td_current_read_op_count++;
//				break;
//			case TASK_OP_TYPE_WRITE: 
//				tdp->td_counters.tc_accumulated_write_op_time += tdp->td_current_op_elapsed_time;
//				tdp->td_current_bytes_written += wdp->wd_task.task_xfer_size;
//				tdp->td_current_write_op_count++;
//				break;
//			case TASK_OP_TYPE_NOOP: 
//				tdp->td_counters.tc_accumulated_noop_op_time += tdp->td_current_op_elapsed_time;
//				tdp->td_current_bytes_noop += wdp->wd_task.task_xfer_size;
//				tdp->td_current_noop_op_count++;
//				break;
//		} // End of SWITCH
	} else {// Something went wrong - issue error message
		if (xgp->global_options & GO_STOP_ON_ERROR) {
			tdp->td_abort = 1; // This tells all the other Worker Threads and the Target Thread to abort
		}
		fprintf(xgp->errout, "%s: I/O ERROR on Target %d Worker Thread %d: ERROR: Status %d, I/O Transfer Size [expected status] %d, %s Operation Number %lld\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			(int)wdp->wd_task.task_io_status,
			(int)wdp->wd_task.task_xfer_size,
			wdp->wd_task.task_op_string,
			(long long)wdp->wd_task.task_op_number);
		if (!(tdp->td_target_options & TO_SGIO)) {
			if ((wdp->wd_counters.tc_current_io_status == 0) && (errno == 0)) { // Indicate this is an end-of-file condition
				fprintf(xgp->errout, "%s: Target %d Worker Thread %d: WARNING: END-OF-FILE Reached during %s Operation Number %lld\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_thread_number,
					wdp->wd_task.task_op_string,
					(long long)wdp->wd_task.task_op_number);
			} else {
				perror("reason"); // Only print the reason (aka errno text) if this is not an SGIO request
			}
		}
		wdp->wd_counters.tc_current_error_count = 1;
	} // Done checking status
#ifdef ndef
	tdp->td_counters.tc_current_op_elapsed_time = (wdp->wd_counters.tc_current_op_end_time - wdp->wd_counters.tc_current_op_start_time);
	tdp->td_counters.tc_accumulated_op_time += tdp->td_counters.tc_current_op_elapsed_time;
	tdp->td_current_io_errno = errno;
	tdp->td_current_error_count = 0;
	if (tdp->td_counters.tc_current_io_status == tdp->td_xfer_size) { // Status is GOOD - update counters
		tdp->td_current_bytes_xfered_this_op = wdp->wd_task.task_xfer_size;
		tdp->td_current_bytes_xfered += wdp->wd_task.task_xfer_size;
		tdp->td_current_op_count++;
		// Operation-specific counters
		switch (tdp->td_current_op_type) { 
			case TASK_OP_TYPE_READ: 
				tdp->td_counters.tc_accumulated_read_op_time += tdp->td_current_op_elapsed_time;
				tdp->td_current_bytes_read += wdp->wd_task.task_xfer_size;
				tdp->td_current_read_op_count++;
				break;
			case TASK_OP_TYPE_WRITE: 
				tdp->td_counters.tc_accumulated_write_op_time += tdp->td_current_op_elapsed_time;
				tdp->td_current_bytes_written += wdp->wd_task.task_xfer_size;
				tdp->td_current_write_op_count++;
				break;
			case TASK_OP_TYPE_NOOP: 
				tdp->td_counters.tc_accumulated_noop_op_time += tdp->td_current_op_elapsed_time;
				tdp->td_current_bytes_noop += wdp->wd_task.task_xfer_size;
				tdp->td_current_noop_op_count++;
				break;
		} // End of SWITCH
	} else {// Something went wrong - issue error message
		if (xgp->global_options & GO_STOP_ON_ERROR) {
			tdp->td_abort = 1; // Remember to abort this Worker Thread
		}
		fprintf(xgp->errout, "%s: I/O ERROR on Target %d Worker Thread %d: ERROR: Status %d, I/O Transfer Size [expected status] %d, %s Operation Number %lld\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			tdp->td_counters.tc_current_io_status,
			wdp->wd_task.task_xfer_size,
			tdp->td_task.task_op_string,
			(long long)tdp->td_current_op_number);
		if (!(tdp->td_target_options & TO_SGIO)) {
			if ((tdp->td_counters.tc_current_io_status == 0) && (errno == 0)) { // Indicate this is an end-of-file condition
				fprintf(xgp->errout, "%s: Target %d Worker Thread %d: WARNING: END-OF-FILE Reached during %s Operation Number %lld\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_thread_number,
					tdp->td_task.task_op_string,
					(long long)tdp->td_current_op_number);
			} else {
				perror("reason"); // Only print the reason (aka errno text) if this is not an SGIO request
			}
		}
		tdp->td_current_error_count = 1;
	} // Done checking status
#endif
} // End of xdd_worker_thread_update_local_counters()

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_update_target_counters() - This subroutine will update the Target
 * Data counters and timers 
 * NOTE: This routine locks the Target Data so that no other Worker Threads can be
 * making simultaneous updates. The lock is only held in this routine.
 */
void
xdd_worker_thread_update_target_counters(worker_data_t *wdp) {
	target_data_t	*tdp;			// Pointer to the Tartget's Data

	// Get the pointer to the Target's Data
	tdp = wdp->wd_tdp;

	///////////////////////////////////////////////////////////////////////////////////	
	// The following section gets the "counter_mutex" of the Target Data 
	// in order to update the various counters and timers.
	//
	// LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK LOCK
	pthread_mutex_lock(&tdp->td_counters_mutex);
	// Update counters and status in the Worker Thread Data
	tdp->td_counters.tc_accumulated_op_time += tdp->td_counters.tc_current_op_elapsed_time;
	if (wdp->wd_task.task_io_status == wdp->wd_task.task_xfer_size) { // Only update counters if I/O succeeded
		tdp->td_current_bytes_completed += wdp->wd_task.task_xfer_size;
		tdp->td_counters.tc_accumulated_bytes_xfered += wdp->wd_task.task_xfer_size;
		tdp->td_counters.tc_accumulated_op_count++;
		if (tdp->td_e2ep && wdp->wd_e2ep)
			tdp->td_e2ep->e2e_sr_time += wdp->wd_e2ep->e2e_sr_time; // E2E Send/Receive Time
//		if ( (tdp->td_target_options & TO_ENDTOEND) && (tdp->td_target_options & TO_E2E_DESTINATION)) {
//			if ( wdp->wd_first_pass_start_time < tdp->td_first_pass_start_time)  // Record the proper *First* pass start time
//				tdp->td_first_pass_start_time = wdp->wd_first_pass_start_time;
//			if ( wdp->wd_counters.tc_pass_start_time < tdp->td_counters.tc_pass_start_time)  // Record the proper PASS start time
//				tdp->td_counters.tc_pass_start_time = wdp->wd_counters.tc_pass_start_time;
//		}
		// Operation-specific counters
		switch (wdp->wd_task.task_op_type) { 
			case TASK_OP_TYPE_READ: 
				tdp->td_counters.tc_accumulated_read_op_time += wdp->wd_counters.tc_current_op_elapsed_time;
				tdp->td_counters.tc_accumulated_bytes_read += wdp->wd_task.task_xfer_size;
				tdp->td_counters.tc_accumulated_read_op_count++;
				break;
			case TASK_OP_TYPE_WRITE: 
				tdp->td_counters.tc_accumulated_write_op_time += wdp->wd_counters.tc_current_op_elapsed_time;
				tdp->td_counters.tc_accumulated_bytes_written += wdp->wd_task.task_xfer_size;
				tdp->td_counters.tc_accumulated_write_op_count++;
				break;
			case TASK_OP_TYPE_NOOP: 
				tdp->td_counters.tc_accumulated_noop_op_time += wdp->wd_counters.tc_current_op_elapsed_time;
				tdp->td_counters.tc_accumulated_bytes_noop += wdp->wd_task.task_xfer_size;
				tdp->td_counters.tc_accumulated_noop_op_count++;
				break;
			default:
				break;
		} // End of SWITCH
	} // End of IF clause that updates counters

	// If this Worker Thread got an I/O error then its error count will be 1, otherwise it will be zero
	tdp->td_counters.tc_current_error_count += wdp->wd_counters.tc_current_error_count;

	pthread_mutex_unlock(&tdp->td_counters_mutex);
	// UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED UNLOCKED

	// If this Worker Thread got an I/O error then we do not want to update the TOT
	if (tdp->td_counters.tc_current_error_count) 
		return; 

	///////////////////////////////////////////////////////////////////////////////////	
	// Update the TOT entry for this last I/O
	// Since the TOT is a resource owned by the Target Thread and shared by the Worker Threads
	// it will be updated here.
	if (tdp->td_target_options & (TO_ORDERING_STORAGE_SERIAL | TO_ORDERING_STORAGE_LOOSE)) {
	    tdp->td_current_state |= CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE;
	    tot_update(tdp->td_totp,
		       wdp->wd_task.task_op_number,
		       wdp->wd_thread_number,
		       wdp->wd_task.task_byte_offset,
		       wdp->wd_task.task_xfer_size);
	    tdp->td_current_state &= ~CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE;
	}
} // End of xdd_worker_thread_update_target_counters()

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
