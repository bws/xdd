/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines used by target_pass() or targetpass_loop() 
 * that are specific to an End-to-End (E2E) operation.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_loop_dst() - This subroutine will manage assigning tasks to
 * Worker Threads during an E2E operation but only on the destination side of an
 * E2E operation. 
 * Called from xdd_targetpass()
 * 
 */
void
xdd_targetpass_e2e_loop_dst(xdd_plan_t* planp, target_data_t *tdp) {
	worker_data_t	*wdp;
	xdd_ts_tte_t	*ttep;
	int		q;

	// The idea is to keep all the Worker Threads busy reading whatever is sent to them
	// from their respective Worker Threads on the Source Side.
	// The "normal" targetpass task assignment loop counts down the number
	// of bytes it has assigned to be transferred until the number of bytes remaining
	// becomes zero.
	// This task assignment loop is based on the availability of Worker Threads to perform
	// I/O tasks - namely a recvfrom/write task which is specific to the 
	// Destination Side of an E2E operation. Each Worker Thread will continue to perform
	// these recvfrom/write tasks until it receives an End-of-File (EOF) packet from 
	// the Source Side. At this point that Worker Thread remains "unavailable" but also 
	// turns on its "EOF" flag to indicate that it has received an EOF. It will also
	// enter the targetpass_worker_thread_passcomplete barrier so that by the time this 
	// subroutine returns, all the Worker Threads will be at the targetpass_worker_thread_passcomplete
	// barrier.
	//
	// This routine will keep requesting Worker Threads from the Worker Thread Locator until it
	// returns "zero" which indicates that all the Worker Threads have received an EOF.
	// At that point this routine will return.

	// Get the first Worker Thread pointer for this Target
	wdp = xdd_get_any_available_worker_thread(tdp);

	//////////////////////////// BEGIN I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////
	while (wdp) { 

		// Check to see if we've been canceled - if so, we need to leave this loop
		if ((xgp->canceled) || (xgp->abort) || (tdp->td_abort)) {
			// When we got this Worker Thread the WTSYNC_BUSY flag was set by get_any_available_worker_thread()
			// We need to reset it so that the subsequent loop will find it with get_specific_worker_thread()
			// Normally we would get the mutex lock to do this update but at this point it is not necessary.
			wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY;
			break;
		}

	
		// Make sure the Worker Thread does not think the pass is complete
		// Get the most recent File Descriptor in case it changed...
		wdp->wd_task.task_file_desc = tdp->td_file_desc;
		wdp->wd_task.task_request = TASK_REQ_IO;
		wdp->wd_task.task_op_type = TASK_OP_TYPE_WRITE;
		wdp->wd_task.task_op_number = tdp->td_counters.tc_current_op_number;
		wdp->wd_task.task_byte_offset = -1; // To be filled in after data received
		wdp->wd_task.task_xfer_size = -1; 	// To be filled in after data received
		wdp->wd_task.task_io_status = -1; 	// To be filled in after data received
		wdp->wd_task.task_errno = 0; 		// To be filled in after data received
//		if (tdp->td_counters.tc_current_op_number == 0) {
//			nclk_now(&tdp->td_counters.tc_time_first_op_issued_this_pass);
//		}

   		// If time stamping is on then assign a time stamp entry to this Worker Thread
   		if ((tdp->td_ts_table.ts_options & (TS_ON|TS_TRIGGERED))) {
			wdp->wd_ts_entry = tdp->td_ts_table.ts_current_entry;	
			ttep = & tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
			tdp->td_ts_table.ts_current_entry++;
			if (tdp->td_ts_table.ts_options & TS_ONESHOT) { // Check to see if we are at the end of the ts buffer
				if (tdp->td_ts_table.ts_current_entry == tdp->td_ts_table.ts_size)
					tdp->td_ts_table.ts_options &= ~TS_ON; // Turn off Time Stamping now that we are at the end of the time stamp buffer
			} else if (tdp->td_ts_table.ts_options & TS_WRAP) {
				tdp->td_ts_table.ts_current_entry = 0; // Wrap to the beginning of the time stamp buffer
			}
			ttep->tte_pass_number = tdp->td_counters.tc_pass_number;
			ttep->tte_worker_thread_number = wdp->wd_worker_number;
			ttep->tte_thread_id     = wdp->wd_thread_id;
			ttep->tte_op_type = TASK_OP_TYPE_WRITE;
			ttep->tte_op_number = -1; 		// to be filled in after data received
			ttep->tte_byte_offset = -1; 	// to be filled in after data received
			ttep->tte_disk_xfer_size = 0; 	// to be filled in after data received
			ttep->tte_net_xfer_size = 0; 	// to be filled in after data received
		}

		// Release the Worker Thread to let it start working on this task
		xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&tdp->td_occupant,0);
		// At this point the Worker Thread is running. The first thing it will do is perform all the 
		// Things To Do (ttd) before the I/O operation. This includes receiving data from the Source
		// which will block until it gets the data. Once the data is received, the Worker Thread will
		// perform the requested WRITE operation to the Destination file. The byte offset and length
		// to WRITE is provided by the Source in the Header of the data received. 
		// If the next data blob received is an End Of File (EOF) packet, then things come to a halt.
	
		tdp->td_counters.tc_current_op_number++;
		// Get another Worker Thread and lets keepit roling...
		wdp = xdd_get_any_available_worker_thread(tdp);
	
	} // End of WHILE loop
	//////////////////////////// END OF I/O LOOP FOR ENTIRE PASS ///////////////////////////////////////////
        
	// Check to see if we've been canceled - if so, we need to leave 
	if (xgp->canceled) {
		fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
			xgp->progname,
			tdp->td_target_number);
		return;
	}
	// Wait for all Worker Threads to complete their most recent task
	// The easiest way to do this is to get the Worker Thread pointer for each
	// Worker Thread specifically and then reset it's "busy" bit to 0.
	for (q = 0; q < tdp->td_queue_depth; q++) {
		wdp = xdd_get_specific_worker_thread(tdp,q);
		pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
		wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY; // Mark this Worker Thread NOT Busy
		pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
		// Check to see if we've been canceled - if so, we need to leave 
		if (xgp->canceled) {
			fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
				xgp->progname,
				tdp->td_target_number);
			break;
		}
	}

	if (tdp->td_counters.tc_current_io_status != 0) 
		planp->target_errno[tdp->td_target_number] = XDD_RETURN_VALUE_IOERROR;

	return;

} // End of xdd_targetpass_e2e_loop_dst()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_loop_src() - This subroutine will assign tasks to Worker Threads until
 * all bytes have been processed. It will then issue an End-of-Data Task to 
 * all Worker Threads one at a time. The End-of-Data Task will send an End-of-Data
 * packet to the Destination Side so that those Worker Threads know that there
 * is no more data to receive.
 * 
 * This subroutine is called by xdd_targetpass().
 */
void
xdd_targetpass_e2e_loop_src(xdd_plan_t* planp, target_data_t *tdp) {
	worker_data_t	*wdp;
	int		q;
	int32_t	status;


	while (tdp->td_current_bytes_remaining) {
		// Get pointer to next Worker Thread to issue a task to
		wdp = xdd_get_any_available_worker_thread(tdp);

		// Things to do before an I/O is issued
		status = xdd_target_ttd_before_io_op(tdp,wdp);
		// Check to see if either the pass or run time limit has expired - if so, we need to leave this loop
		if (status != XDD_RC_GOOD)
			break;

		// Set up the task for the Worker Thread
		xdd_targetpass_e2e_task_setup_src(wdp);

		// Release the Worker Thread to let it start working on this task
		xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&tdp->td_occupant,0);

	} // End of WHILE loop that transfers data for a single pass

	// Check to see if we've been canceled - if so, we need to leave 
	if (xgp->canceled) {
		fprintf(xgp->errout,"\n%s: xdd_targetpass_e2e_loop_src: Target %d: ERROR: Canceled!\n",
			xgp->progname,
			tdp->td_target_number);
		return;
	}

	// Assign each of the Worker Threads an End-of-Data Task
	xdd_targetpass_e2e_eof_src(tdp);


	// Wait for all Worker Threads to complete their most recent task
	// The easiest way to do this is to get the Worker Thread pointer for each
	// Worker Thread specifically and then reset it's "busy" bit to 0.
	for (q = 0; q < tdp->td_queue_depth; q++) {
		wdp = xdd_get_specific_worker_thread(tdp,q);
		pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
		wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY; // Mark this Worker Thread NOT Busy
		pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
	}

	if (tdp->td_counters.tc_current_io_status != 0) 
		planp->target_errno[tdp->td_target_number] = XDD_RETURN_VALUE_IOERROR;

	return;

} // End of xdd_targetpass_e2e_loop_src()
/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_task_setup_src() - This subroutine will set up the task info for an I/O
 */
void
xdd_targetpass_e2e_task_setup_src(worker_data_t *wdp) {
	target_data_t	*tdp;
	xdd_ts_tte_t	*ttep;

	tdp = wdp->wd_tdp;
	// Assign an IO task to this worker thread
	wdp->wd_task.task_request = TASK_REQ_IO;

	// Get the most recent File Descriptor in case it changed...
	wdp->wd_task.task_file_desc = tdp->td_file_desc;

	// Set the Operation Type
	if (tdp->td_seekhdr.seeks[tdp->td_counters.tc_current_op_number].operation == SO_OP_WRITE) { // Write Operation
		wdp->wd_task.task_op_type = TASK_OP_TYPE_WRITE;
		wdp->wd_task.task_op_string = "WRITE";
	} else if (tdp->td_seekhdr.seeks[tdp->td_counters.tc_current_op_number].operation == SO_OP_READ) { // READ Operation
		wdp->wd_task.task_op_type = TASK_OP_TYPE_READ;
		wdp->wd_task.task_op_string = "READ";
	} else { 
		wdp->wd_task.task_op_type = TASK_OP_TYPE_NOOP;
		wdp->wd_task.task_op_string = "NOOP";
	}
	 
	// Figure out the transfer size to use for this I/O
	if (tdp->td_current_bytes_remaining < (uint64_t)tdp->td_xfer_size)
		wdp->wd_task.task_xfer_size = tdp->td_current_bytes_remaining;
	else wdp->wd_task.task_xfer_size = tdp->td_xfer_size;
	wdp->wd_task.task_io_status = 0;
	wdp->wd_task.task_errno = 0;

	// Set the location to seek to 
	wdp->wd_task.task_byte_offset = tdp->td_counters.tc_current_byte_offset;

	// Remember the operation number for this target
	wdp->wd_task.task_op_number = tdp->td_counters.tc_current_op_number;

	wdp->wd_e2ep->e2e_msg_sequence_number = tdp->td_e2ep->e2e_msg_sequence_number;
	tdp->td_e2ep->e2e_msg_sequence_number++;

	// Remember the operation number for this target
	wdp->wd_task.task_op_number = tdp->td_counters.tc_current_op_number;

   	// If time stamping is on then assign a time stamp entry to this Worker Thread
   	if ((tdp->td_ts_table.ts_options & (TS_ON|TS_TRIGGERED))) {
		wdp->wd_ts_entry = tdp->td_ts_table.ts_current_entry;	
		ttep = & tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
		tdp->td_ts_table.ts_current_entry++;
		if (tdp->td_ts_table.ts_options & TS_ONESHOT) { // Check to see if we are at the end of the ts buffer
			if (tdp->td_ts_table.ts_current_entry == tdp->td_ts_table.ts_size)
				tdp->td_ts_table.ts_options &= ~TS_ON; // Turn off Time Stamping now that we are at the end of the time stamp buffer
		} else if (tdp->td_ts_table.ts_options & TS_WRAP) {
			tdp->td_ts_table.ts_current_entry = 0; // Wrap to the beginning of the time stamp buffer
		}
		ttep->tte_pass_number = tdp->td_counters.tc_pass_number;
		ttep->tte_worker_thread_number = wdp->wd_worker_number;
		ttep->tte_thread_id = wdp->wd_thread_id;
		ttep->tte_op_type = wdp->wd_task.task_op_type;
		ttep->tte_op_number = wdp->wd_task.task_op_number;
		ttep->tte_byte_offset = wdp->wd_task.task_byte_offset;
	}
if (xgp->global_options & GO_DEBUG_TASK) fprintf(stderr,"DEBUG_TASK: %lld: xdd_targetpass_e2e_task_setup_src: Target: %d: Worker: %d: task_request: 0x%x: file_desc: %d: datap: %p: op_type: %d, op_string: %s: op_number: %lld: xfer_size: %d, byte_offset: %lld: e2e_sequence_number: %lld\n ", (long long int)pclk_now(),tdp->td_target_number,wdp->wd_worker_number,wdp->wd_task.task_request,wdp->wd_task.task_file_desc,wdp->wd_task.task_datap,wdp->wd_task.task_op_type,wdp->wd_task.task_op_string,(unsigned long long int)wdp->wd_task.task_op_number,(int)wdp->wd_task.task_xfer_size,(long long int)wdp->wd_task.task_byte_offset,(unsigned long long int)wdp->wd_task.task_e2e_sequence_number);
	// Update the pointers/counters in the Target Data Struct to get ready for the next I/O operation
	tdp->td_counters.tc_current_byte_offset += wdp->wd_task.task_xfer_size;
	tdp->td_counters.tc_current_op_number++;
	tdp->td_current_bytes_issued += wdp->wd_task.task_xfer_size;
	tdp->td_current_bytes_remaining -= wdp->wd_task.task_xfer_size;
	
	// E2E Source Side needs to be monitored...
	if (tdp->td_target_options & TO_E2E_SOURCE_MONITOR)
		xdd_targetpass_e2e_monitor(tdp);

} // End of xdd_targetpass_e2e_task_setup_src()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_eof_source_side() - This subroutine will manage End-Of-File
 * processing for an End-to-End operation on the source side only.
 * This subroutine will cycle through all the Worker Threads for a specific Target Thread.
 * Upon completion of this routine all the Worker Threads on the SOURCE side will have
 * been given a task to send an EOF packet to their corresponding Worker Thread on the
 * Destination side.
 *
 * We need to issue an End-of-File task for this Worker Thread so that it sends an EOF
 * packet to the corresponding Worker Thread on the Destination Side. We do not have to wait for
 * that operation to complete. Just cycle through all the Worker Threads and later wait at the 
 * targetpass_worker_thread_passcomplete barrier. 
 *
 */
void
xdd_targetpass_e2e_eof_src(target_data_t *tdp) {
	worker_data_t	*wdp;
	xdd_ts_tte_t	*ttep;
	int		q;

	for (q = 0; q < tdp->td_queue_depth; q++) {
		wdp = xdd_get_specific_worker_thread(tdp,q);
		wdp->wd_task.task_request = TASK_REQ_EOF;

   		// If time stamping is on then assign a time stamp entry to this Worker Thread
   		if ((tdp->td_ts_table.ts_options & (TS_ON|TS_TRIGGERED))) {
			wdp->wd_ts_entry = tdp->td_ts_table.ts_current_entry;	
			ttep = & tdp->td_ts_table.ts_hdrp->tsh_tte[wdp->wd_ts_entry];
			tdp->td_ts_table.ts_current_entry++;
			if (tdp->td_ts_table.ts_options & TS_ONESHOT) { // Check to see if we are at the end of the ts buffer
				if (tdp->td_ts_table.ts_current_entry == tdp->td_ts_table.ts_size)
					tdp->td_ts_table.ts_options &= ~TS_ON; // Turn off Time Stamping now that we are at the end of the time stamp buffer
			} else if (tdp->td_ts_table.ts_options & TS_WRAP) {
				tdp->td_ts_table.ts_current_entry = 0; // Wrap to the beginning of the time stamp buffer
			}
			ttep->tte_pass_number = tdp->td_counters.tc_pass_number;
			ttep->tte_worker_thread_number = wdp->wd_worker_number;
			ttep->tte_thread_id = wdp->wd_thread_id;
			ttep->tte_op_type = TASK_OP_TYPE_EOF;
			ttep->tte_op_number = -1*wdp->wd_worker_number;
			ttep->tte_byte_offset = -1;
		}
	
		// Release the Worker Thread to let it start working on this task
		xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&tdp->td_occupant,0);
	
	}
} // End of xdd_targetpass_eof_source_side()

/*----------------------------------------------------------------------------*/
/* xdd_targetpass_e2e_monitor() - This subroutine will monitor and display
 * information about the Worker Threads that are running on the Source Side of an
 * E2E operation.
 * 
 * This subroutine is called by xdd_targetpass_loop().
 */
void
xdd_targetpass_e2e_monitor(target_data_t *tdp) {
	worker_data_t	*tmpwdp;
	int qmax, qmin;
	uint64_t opmax, opmin;
	int qavail;


	if ((tdp->td_counters.tc_current_op_number > 0) && ((tdp->td_counters.tc_current_op_number % tdp->td_queue_depth) == 0)) {
		qmin = 0;
		qmax = 0;
		opmin = tdp->td_target_ops;
		opmax = 0;
		qavail = 0;
		tmpwdp = tdp->td_next_wdp; // first Worker Thread on the chain
		while (tmpwdp) { // Scan the Worker Threads to determine the one furthest ahead and the one furthest behind
			if (tmpwdp->wd_worker_thread_target_sync & WTSYNC_BUSY) {
				if (tdp->td_counters.tc_current_op_number < opmin) {
					opmin = tdp->td_counters.tc_current_op_number;
					qmin = tmpwdp->wd_worker_number;
				}
				if (tdp->td_counters.tc_current_op_number > opmax) {
					opmax = tdp->td_counters.tc_current_op_number;
					qmax = tmpwdp->wd_worker_number;
				}
			} else {
				qavail++;
			}
			tmpwdp = tmpwdp->wd_next_wdp;
		}
		fprintf(stderr,"\n\nopmin %4lld, qmin %4d, opmax %4lld, qmax %4d, separation is %4lld, %4d worker threads busy, %lld percent complete\n\n",
			(long long int)opmin, qmin, (long long int)opmax, qmax, (long long int)(opmax-opmin+1), tdp->td_queue_depth-qavail, (long long int)((opmax*100)/tdp->td_target_ops));
	}
} // End of xdd_targetpass_e2e_monitor();

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
