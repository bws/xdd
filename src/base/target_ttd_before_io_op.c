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
// Things the Target Thread has to do before each I/O Operation is issued
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_syncio_before_io_op() - This subroutine will enter the syncio 
 * barrier if it has reached the syncio-number-of-ops. Once all the other 
 * threads enter this barrier then they will all get released and I/O starts 
 * up again.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_syncio_before_io_op(target_data_t *tdp) {


	if ((tdp->td_planp->syncio > 0) && 
	    (tdp->td_planp->number_of_targets > 1) && 
	    (tdp->td_counters.tc_current_op_number % tdp->td_planp->syncio == 0)) {
		xdd_barrier(&tdp->td_planp->main_targets_syncio_barrier,&tdp->td_occupant,0);
	}


} // End of xdd_syncio_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_start_trigger_before_io_op() - This subroutine will wait for a 
 * trigger and signal another target to start if necessary.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_start_trigger_before_io_op(target_data_t *tdp) {
	target_data_t			*tdp2;	// Ptr to the Target Data that we need to trigger
	xint_triggers_t	*trigp1;
	xint_triggers_t	*trigp2;
	nclk_t			tt;	// Trigger Time


	/* Check to see if we need to wait for another target to trigger us to start.
 	* If so, then enter the trigger_start barrier and have a beer until we get the signal to
 	* jump into action!
 	*/
	trigp1 = tdp->td_trigp;
	if (trigp1 == NULL)
		return(XDD_RC_GOOD);

	if ((tdp->td_target_options & TO_WAITFORSTART) && (trigp1->run_status == 0)) { 
		/* Enter the barrier and wait to be told to go */
		xdd_barrier(&trigp1->target_target_starttrigger_barrier,&tdp->td_occupant,1);
		trigp1->run_status = 1; /* indicate that we have been released */
	}

	/* Check to see if we need to signal some other target to start, stop, or pause.
	 * If so, tickle the appropriate semaphore for that target and get on with our business.
	 */
	if (trigp1->trigger_types) {
		tdp2 = tdp->td_planp->target_datap[trigp1->start_trigger_target];
		trigp2 = tdp2->td_trigp;
		if (trigp2->run_status == 0) {
			if (trigp1->trigger_types & TRIGGER_STARTTIME) {
			/* If we are past the start time then signal the specified target to start */
				nclk_now(&tt);
				if (tt > (trigp1->start_trigger_time + tdp->td_counters.tc_pass_start_time)) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&tdp->td_occupant,0);
				}
			}
			if (trigp1->trigger_types & TRIGGER_STARTOP) {
				/* If we are past the specified operation, then signal the specified target to start */
				if (tdp->td_counters.tc_current_op_number > trigp1->start_trigger_op) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&tdp->td_occupant,0);
				}
			}
			if (trigp1->trigger_types & TRIGGER_STARTBYTES) {
				/* If we have completed transferring the specified number of bytes, then signal the 
				* specified target to start 
				*/
				if (tdp->td_current_bytes_completed > trigp1->start_trigger_bytes) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&tdp->td_occupant,0);
				}
			}
		}
	} /* End of the trigger processing */
	return(XDD_RC_GOOD);

} // End of xdd_start_trigger_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_timelimit_before_io_op() - This subroutine will check to see if the
 * specified time limit for this pass has expired. 
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_timelimit_before_io_op(target_data_t *tdp) {
	nclk_t	current_time;		// What time is it *now*?
	nclk_t	elapsed_time;		// Elapsed time


	/* Check to see if a time limit (in seconds) was specified.
 	* If so, then check to see if we have exceeded that amount of time and
 	* set the global variable "time_limit_expired". 
    * Otherwise, return 0 and continue issuing I/Os.
 	*/
	if (tdp->td_time_limit_ticks) { 
		nclk_now(&current_time);
		elapsed_time = current_time - tdp->td_counters.tc_pass_start_time;
if (xgp->global_options & GO_DEBUG_TIME_LIMIT) fprintf(stderr,"DEBUG_TIME_LIMIT: %lld: xdd_timelimit_before_io_op: Target: %d: Worker: -: current_time: %lld: tc_pass_start_time: %lld: elapsed_time: %lld: time_limit_ticks: %lld\n", (long long int)pclk_now(),tdp->td_target_number,(long long int)current_time, (long long int)tdp->td_counters.tc_pass_start_time, (long long int)elapsed_time, (long long int)tdp->td_time_limit_ticks);
		if (elapsed_time >= tdp->td_time_limit_ticks) {
			tdp->td_time_limit_expired = 1;
			fprintf(xgp->output,"\n%s: xdd_timelimit_before_io_op: Target %d: Specified time limit of %f seconds exceeded.\n",
		 		xgp->progname,
			 	tdp->td_target_number,
			 	tdp->td_time_limit);
			return(XDD_RC_BAD);
		}
	}

	return(XDD_RC_GOOD);

} // End of xdd_timelimit_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_runtime_before_io_op() - This subroutine will check to see if the
 * specified time limit for this run has expired. 
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_runtime_before_io_op(target_data_t *tdp) {
	nclk_t	current_time;		// What time is it *now*?
	nclk_t	elapsed_time;		// Elapsed time


	/* Check to see if a time limit (in seconds) was specified.
 	* If so, then check to see if we have exceeded that amount of time and
 	* set the global variable "time_limit_expired". 
    * Otherwise, return 0 and continue issuing I/Os.
 	*/
	if (tdp->td_planp->run_time_ticks) { 
		nclk_now(&current_time);
		elapsed_time = current_time - tdp->td_planp->run_start_time;
		if (elapsed_time >= tdp->td_planp->run_time_ticks) {
			xgp->run_time_expired = 1;
			fprintf(xgp->output,"\n%s: xdd_runtime_before_io_op: Specified run time of %f seconds exceeded.\n",
		 		xgp->progname,
			 	tdp->td_planp->run_time);
			return(XDD_RC_BAD);
		}
	}

	return(XDD_RC_GOOD);

} // End of xdd_runtime_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_target_ttd_before_io_op() - This subroutine will do all the stuff 
 * needed to be done by the Target Thread before a Worker Thread is issued with 
 * an I/O task.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_target_ttd_before_io_op(target_data_t *tdp, worker_data_t *wdp) {
	int32_t	status;	// Return status from various subroutines

	// Syncio barrier - wait for all others to get here 
	xdd_syncio_before_io_op(tdp);

	// Check to see if we need to wait for another target to trigger us to start.
	xdd_start_trigger_before_io_op(tdp);

	// Check to see if we exceeded the time limit for this pass
	status = xdd_timelimit_before_io_op(tdp);
	if (status != XDD_RC_GOOD) 
		return(status);

	// Check to see if we exceeded the time limit for this run
	status = xdd_runtime_before_io_op(tdp);
	if (status != XDD_RC_GOOD) 
		return(status);

	/* init the error number and break flag for good luck */
	errno = 0;
	/* Get the location to seek to */
	if (tdp->td_seekhdr.seek_options & SO_SEEK_NONE) /* reseek to starting offset if noseek is set */
		tdp->td_counters.tc_current_byte_offset = (uint64_t)((tdp->td_target_number * tdp->td_planp->target_offset) + 
											tdp->td_seekhdr.seeks[0].block_location) * 
											tdp->td_block_size;
	else tdp->td_counters.tc_current_byte_offset = (uint64_t)((tdp->td_target_number * tdp->td_planp->target_offset) + 
											tdp->td_seekhdr.seeks[tdp->td_counters.tc_current_op_number].block_location) * 
											tdp->td_block_size;

	if (xgp->global_options & GO_INTERACTIVE)	
		xdd_barrier(&tdp->td_planp->interactive_barrier,&tdp->td_occupant,0);

	// Check to see if either the pass or run time limit has 
	// expired - if so, we need to leave this loop
	if ((tdp->td_time_limit_expired) || (xgp->run_time_expired)) 
		return(XDD_RC_BAD);

	// Check to see if we've been canceled - if so, we need 
	// to set the WTSYNC_BUSY flag and leave this loop
	if ((xgp->canceled) || (xgp->abort) || (tdp->td_abort)) {
		// When we got this Worker Thread the WTSYNC_BUSY flag was 
		// set by get_any_available_worker_thread()
		// We need to reset it so that the subsequent loop 
		// will find it with get_specific_worker_thread()
		// Normally we would get the mutex lock to do this 
		// update but at this point it is not necessary.
		wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY;
		return(XDD_RC_BAD);
	}

	return(XDD_RC_GOOD);

} // End of xdd_target_ttd_before_io_op()

 
