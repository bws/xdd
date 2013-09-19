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
#include "xint.h"

//******************************************************************************
// Contains subroutines that implement the lockstep option
//******************************************************************************
/*----------------------------------------------------------------------------*/
// xdd_lockstep_init(target_data_t *tdp) - Initialize mutex and barriers for a lockstep
//    operation.
// This subroutine is called by target_init().
// The target being initialized can be either a MASTER or a SLAVE or both.
// Assuming nothing bad happens whilst initializing things, this subroutine
// will return XDD_RC_GOOD.
//

int32_t
xdd_lockstep_init(target_data_t *tdp) {
	int32_t		status;			// Status of a function call
	lockstep_t	*lsp;	// Pointer to the Lock Step Struct
	char 		errmsg[256];


	lsp = tdp->td_lsp;
	// If there is no lockstep pointer then this target is not involved in a lockstep
	if (lsp == 0) 
		return(XDD_RC_GOOD); 

	// Check to see if this target has already been initialized
	if (lsp->ls_state & LS_STATE_INITIALIZED)
		return(XDD_RC_GOOD); 

if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_init:p:%p:lsp:%p:state:0x%x:ENTER my taget number is %d, the next target number is %d\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number,lsp->ls_next_tdp->td_target_number);
	// Init the task-counter mutex and the lockstep barrier 
	status = pthread_mutex_init(&lsp->ls_mutex, 0);
	if (status) {
		sprintf(errmsg,"%s: io_thread_init:Error initializing lock step target %d mutex",
			xgp->progname,tdp->td_target_number);
		perror(errmsg);
		fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
			xgp->progname,tdp->td_target_number);
		fflush(xgp->errout);
		xgp->abort = 1;
		return(XDD_RC_UGLY);
	}
	// Initialize barrier in this lockstep structure
	// The "MASTER" is this target and the "SLAVE" is the next target
	sprintf(lsp->Lock_Step_Barrier.name,"LockStep_M%d_S%d",tdp->td_target_number,lsp->ls_next_tdp->td_target_number);
	xdd_init_barrier(tdp->td_planp, &lsp->Lock_Step_Barrier, 2, lsp->Lock_Step_Barrier.name);

	lsp->ls_state |= LS_STATE_INITIALIZED;

if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_init:p:%p:lsp:%p:state:0x%x:EXIT \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state);
	return(XDD_RC_GOOD);
} // End of xdd_lockstep_init()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_pass() - This subroutine initializes the variables that
 * are used by the lockstep option.
 */
void
xdd_lockstep_before_pass(target_data_t *tdp) {
	lockstep_t *lsp;			// Pointer to the lock step struct


	if (tdp->td_lsp == 0) 
		return;

	lsp = tdp->td_lsp;
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_pass:p:%p:lsp:%p:state:0x%x:ENTER \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state);
	if (lsp) {
		lsp->ls_ops_completed_this_interval = 0;
		lsp->ls_ops_completed_this_pass = 0;
		lsp->ls_bytes_scheduled = 0;
		lsp->ls_bytes_completed = 0;
		lsp->ls_state &= ~LS_STATE_PASS_COMPLETE;
	}

} // End of xdd_lockstep_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep(target_data_t *tdp) 
 */
int32_t
xdd_lockstep(target_data_t *tdp) {
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t	*next_lsp;
	int32_t		status;
	worker_data_t	*wdp;
	int		q;
	int		i;
	uint64_t	ops_remaining;
	int64_t	ops_this_interval;


	lsp = tdp->td_lsp;

	// If we are not the first target to run then we need to wait
	if (!(lsp->ls_state & LS_STATE_I_AM_THE_FIRST)) {
	// This is where we wait for the other target to wake us up
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:::TARGET %d ENTERING BARRIER TO WAIT FOR SOMETHING TO DO \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number);
		xdd_barrier(&lsp->Lock_Step_Barrier,&tdp->td_occupant,0);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x::::TARGET %d GOT SOMETHING TO DO!!!\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number);
	}

	ops_remaining = tdp->td_target_ops;
	status = XDD_RC_GOOD;
	// In this version we will do all the looping stuff here
	while (ops_remaining) {
		status = XDD_RC_GOOD;
		if (ops_remaining < lsp->ls_interval_value)
			ops_this_interval = ops_remaining;
		else ops_this_interval = lsp->ls_interval_value;

		for (i = 0; i < ops_this_interval; i++) {
			// Get pointer to next Worker Thread to issue a task to
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::::GET_ANY_AVAILABLE_WORKER_THREAD bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,(long long int)tdp->td_current_bytes_remaining);
			wdp = xdd_get_any_available_worker_thread(tdp);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:GOT_A_WORKER_THREAD bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp, (long long int)tdp->td_current_bytes_remaining);
	
			// Things to do before an I/O is issued
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:THINGS_TO_DO_BEFORE_IO bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp, (long long int)tdp->td_current_bytes_remaining);
			status = xdd_target_ttd_before_io_op(tdp, wdp);
			if (status != XDD_RC_GOOD) {
				// Mark this worker_thread NOT BUSY and break out of this loop
				pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
				wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY; // Mark this WORKER_Thread NOT Busy
				pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
				break;
			}

if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:DONE_WITH_THINGS_TO_DO_BEFORE_IO bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp, (long long int)tdp->td_current_bytes_remaining);
			// Set up the task for the WORKER_Thread
			xdd_target_pass_task_setup(wdp);
	
			// Release the WORKER_Thread to let it start working on this task.
			// This effectively causes the I/O operation to be issued.
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:RELEASING_WORKER_THREAD bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp,(long long int)tdp->td_current_bytes_remaining);
			xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&tdp->td_occupant,0);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:WORKER_THREAD_RELEASED bytes_remaining=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp,(long long int)tdp->td_current_bytes_remaining);
			ops_remaining--;
		}

		// Wait for all WORKER_Threads to complete their most recent task
		// The easiest way to do this is to get the WORKER_Thread pointer for each
		// WORKER_Thread specifically and then reset it's "busy" bit to 0.
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::::WAITING FOR ALL WORKER_THREADS TO COMPLETE\n",(long long int)pclk_now()-xgp->debug_base_time,tdp);
		for (q = 0; q < tdp->td_queue_depth; q++) {
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::::Requesting WORKER_Thread %d\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,q);
			wdp = xdd_get_specific_worker_thread(tdp,q);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:::wdp:%p:Got  WORKER_Thread %d\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp,q);
			pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
			wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY; // Mark this WORKER_Thread NOT Busy
			pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
		}
		if (ops_remaining <= 0) 
			lsp->ls_state |= LS_STATE_PASS_COMPLETE;

		// Figure out what the next target is and release it so that it can run.
		status = 0;
		next_lsp = lsp->ls_next_tdp->td_lsp;
		while (next_lsp->ls_state & LS_STATE_PASS_COMPLETE) {
			status = 1; // Indicates that some target has completed their pass
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:TARGET %d NEXT_LSP IS AT END_OF_PASS - next_lsp=%p, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number, next_lsp,next_lsp->ls_state);
			next_lsp = next_lsp->ls_next_tdp->td_lsp;
			if (next_lsp == lsp)
				break;
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:TARGET %d WHAT IS NEXT NEXT LSP - next_lsp=%p, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number, next_lsp,next_lsp->ls_state);
			// At this point next_lsp points to:
			//    The next target that needs to be released
			//    or
			//    The lsp of this target (no targets get released)
		}

		// If some other target has completed their pass and the "END_STOP"
		// flag is set then we need to pretend we are done too.
		if ((status == 1) && (lsp->ls_state & LS_STATE_END_STOP)) {
			ops_remaining = 0; // This means that we stop here
			lsp->ls_state |= LS_STATE_PASS_COMPLETE;
		}

		// If the next target is simply this target, then we do not release anything and do not wait for anything
		if (next_lsp == lsp) // This is us
			continue;
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:::TARGET %d ENTERING BARRIER TO WAKE UP NEXT TARGET %d - next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number, lsp->ls_next_tdp->td_target_number,next_lsp);
		xdd_barrier(&next_lsp->Lock_Step_Barrier,&tdp->td_occupant,0);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:::TARGET %d RETURNED FROM BARRIER AFTER WAKING UP NEXT TARGET %d - next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number, lsp->ls_next_tdp->td_target_number,next_lsp);

		// If we are done with this pass then dont bother to wait...
		if (lsp->ls_state & LS_STATE_PASS_COMPLETE)
			continue;
		// This is where we wait for the other target to wake us up
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x:::TARGET %d ENTERING BARRIER <2> TO WAIT FOR SOMETHING TO DO \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number);
		xdd_barrier(&lsp->Lock_Step_Barrier,&tdp->td_occupant,0);
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_before_io_op:p:%p:lsp:%p:state:0x%x::::TARGET %d GOT SOMETHING TO DO <2> !!!\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,tdp->td_target_number);

	} // End of WHILE loop

	return(status);

} // End of xdd_lockstep()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_check_triggers()
//  This subroutine is called by either the MASTER or SLAVE to check whether
//  or not it has reached a trigger point.
//  A return of FALSE means that no trigger has been met.
//  A return of TRUE means that one of the triggers has been reached.
//
//  It is assumed that the lock for the lockstep data structure pointed to
//  by lsp is held by the caller.
//
int32_t
xdd_lockstep_check_triggers(worker_data_t *wdp, lockstep_t *lsp) {
	int32_t		status;			// Status to return to caller
	nclk_t   	time_now;		// Used by the lock step functions 
	target_data_t		*tdp;				// Pointer to the Target Data Struct


	tdp = wdp->wd_tdp;

if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_check_triggers:p:%p:lsp:%p:state:0x%x:ENTER - ls_interval_type=0x%x, ls_interval_value=%lld \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,lsp->ls_interval_type,(long long int)lsp->ls_interval_value);
	status = FALSE;
	/* Check to see if it is time to ping the slave to do something */
	if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
		lsp->ls_task_counter++;
		// If we are past the start time then signal the SLAVE to start.
		nclk_now(&time_now);
		if (time_now > lsp->ls_interval_value) {
			status=TRUE;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_OP) {
		lsp->ls_ops_completed_this_interval++;
		lsp->ls_ops_completed_this_pass++;
		// If we are past the specified operation, then signal the SLAVE to start.
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_check_triggers:p:%p:lsp:%p:state:0x%x:INTERVAL_OP - ls_interval_value=%lld, ls_ops_completed_this_interval=%lld, tdp->td_counters.tc_current_op_number=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,(long long int)lsp->ls_interval_value, (long long int)lsp->ls_ops_completed_this_interval, (long long int)tdp->td_counters.tc_current_op_number);
		if (lsp->ls_ops_completed_this_interval >= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_ops_completed_this_interval= 0;
		} 
	}
	if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		lsp->ls_task_counter++;
		// If we have completed percentage of operations then signal the SLAVE to start.
		if (tdp->td_counters.tc_current_op_number >= lsp->ls_task_counter) {
			status=TRUE;
			lsp->ls_task_counter = 0;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		lsp->ls_bytes_completed += wdp->wd_counters.tc_current_io_status;
		// If we have completed transferring the specified number of bytes, then signal the SLAVE to start.
		if (lsp->ls_bytes_completed>= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_bytes_completed= 0;
		}
	}
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_check_triggers:p:%p:lsp:%p:state:0x%x:wdp:%p:TARGET %d ls_ops_completed=%lld, target_ops=%lld \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,wdp,tdp->td_target_number, (long long int)lsp->ls_ops_completed_this_pass,(long long int)tdp->td_target_ops);
	if (lsp->ls_ops_completed_this_pass >= tdp->td_target_ops) {
		lsp->ls_state |= LS_STATE_PASS_COMPLETE;
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_check_triggers:p:%p:lsp:%p:state:0x%x:wdp:%p:TARGET %d END OF PASS - lsp->ls_state=0x%x \n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,wdp,tdp->td_target_number, lsp->ls_state);
		status = TRUE;
	}
if (xgp->global_options & GO_DEBUG_LOCKSTEP) fprintf(stdout,"%lld:lockstep_check_triggers:p:%p:lsp:%p:state:0x%x:returning status %d\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,lsp,lsp->ls_state,status);
	return(status);

} // End of xdd_lockstep_check_triggers() 
