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

/*----------------------------------------------------------------------------*/
/* xdd_worker_thread() - This is the WorkerThread routine that actually performs I/O
 * on behalf of a Target thread.
 * This thread, of which there can be  many, is started by the xdd_target_thread_init()
 * routine. 
 * The WorkerThread will initialize itself and make itself Available
 * and wait for something to do.
 */
void *
xdd_worker_thread(void *pin) {
	int32_t  		status; 	// Status of various system calls
	worker_data_t	*wdp;		// Pointer to this WorkerThread's Data Struct
	target_data_t	*tdp;			// Pointer to this WorkerThread's Target Data Struct
	nclk_t			checktime;

	wdp = (worker_data_t *)pin; 
	tdp = wdp->wd_tdp;	// This is the pointer to this WorkerThread's Target Data Struct

	//fprintf(stderr,"Worker thread starting: %s\n", tdp->td_target_directory);
	status = xdd_worker_thread_init(wdp);
	if (status != 0) {
		fprintf(xgp->errout,"%s: xdd_worker_thread: Aborting target due to previous initialization failure for Target number %d name '%s' WorkerThread %d.\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname,
			wdp->wd_worker_number);
		fflush(xgp->errout);
		xgp->abort = 1; // This will prevent all other threads from being created...
	}

	// Enter the WorkerThread_Init barrier so that the next WorkerThread can start 
	xdd_barrier(&tdp->td_target_worker_thread_init_barrier,&wdp->wd_occupant,0);

	if ( xgp->abort == 1) // Something went wrong during thread initialization so let's just leave
		return(0);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN)
		return(0);

	//////////////////////////////////// Begin the WHILE loop waiting for tasks to perform /////////////////////////////////
	// The subroutine that is called for any particular task will set the "xgp->canceled" flag to
	// indicate that there was a condition that warrants canceling the entire run
	while (1) {
		status = 0;
		// Enter the WorkerThread_TargetPass_Wait barrier until we are assigned something to do by targetpass()
		nclk_now(&checktime);
		xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&wdp->wd_occupant,1);

		// Look at Task request 
		switch (wdp->wd_task.task_request) {
			case TASK_REQ_IO:
				// Perform the requested I/O operation
				xdd_worker_thread_io(wdp);
				break;
			case TASK_REQ_REOPEN:
				// Reopen the target as requested
				xdd_target_reopen(wdp->wd_tdp);
				break;
			case TASK_REQ_STOP:
				// This indicates that we should clean up and exit this subroutine
				xdd_worker_thread_cleanup(wdp);
				return(0);
			case TASK_REQ_EOF:
				// E2E Source Side only - send EOF packets to Destination 
				status = xdd_e2e_eof_source_side(wdp);
				if (status) // Only set the status in the Target Data Struct if it is non-zero
					tdp->td_counters.tc_current_io_status = status;
				break;
			default:
				// Technically, we should never see this....
				fprintf(xgp->errout,"%s: xdd_worker_thread: WARNING: Target number %d name '%s' WorkerThread %d - unknown work request: 0x%x.\n",
					xgp->progname,
					tdp->td_target_number,
					tdp->td_target_full_pathname,
					wdp->wd_worker_number,
					wdp->wd_task.task_request);
				break;
		} // End of SWITCH stmnt that determines the TASK
		
		
// Time stamp if requested
//		if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) {
//			// Record the amount of system and user time used so far...
//			status = getrusage(RUSAGE_THREAD, &usage);
//			errno_save = errno;
//			if (status) {
//				fprintf(xgp->errout,"%s: xdd_worker_thread: WARNING: Target number %d name '%s' WorkerThread %d - calle to getrusage failed\n",
//					xgp->progname,
//					wdp->my_target_number,
//					wdp->target_full_pathname,
//					wdp->my_worker_thread_number);
//				errno = errno_save;
//				perror("Reason");
//			}
//			p->ttp->tte[wdp->tsp->ts_current_entry].usage_utime.tv_sec  = usage.ru_utime.tv_sec;
//			p->ttp->tte[wdp->tsp->ts_current_entry].usage_utime.tv_usec = usage.ru_utime.tv_usec;
//			p->ttp->tte[wdp->tsp->ts_current_entry].usage_stime.tv_sec  = usage.ru_stime.tv_sec;
//			p->ttp->tte[wdp->tsp->ts_current_entry].usage_stime.tv_usec = usage.ru_stime.tv_usec;
//			p->ttp->tte[wdp->tsp->ts_current_entry].nvcsw = usage.ru_nvcsw;
//			p->ttp->tte[wdp->tsp->ts_current_entry].nivcsw = usage.ru_nivcsw;
//		}

		// Mark this WorkerThread Available
		pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
		nclk_now(&checktime);
		wdp->wd_worker_thread_target_sync &= ~WTSYNC_BUSY; // Mark this WorkerThread NOT Busy
		if (wdp->wd_worker_thread_target_sync & WTSYNC_TARGET_WAITING) {
		    // Release the target that is waiting on this WorkerThread
		    //status = sem_post(&wdp->this_worker_thread_is_available_sem);
		    status = pthread_cond_broadcast(&wdp->wd_this_worker_thread_is_available_condition);
		    if (status) {
			fprintf(xgp->errout,"%s: xdd_worker_thread: Target %d WorkerThread %d: WARNING: Bad status from sem_post on this_worker_thread_is_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number,
				status,
				errno);
		    }
			// Turn off the TARGET_WAITING Flag
			wdp->wd_worker_thread_target_sync &= ~WTSYNC_TARGET_WAITING; 
		}

		nclk_now(&checktime);
		
		// Unlock the worker_thread-target synchronization mutex
		pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);

		// Release the WorkerThread Locator which might be waiting for ANY available WorkerThread
		pthread_mutex_lock(&tdp->td_any_worker_thread_available_mutex);
		tdp->td_any_worker_thread_available++;
		status = pthread_cond_broadcast(&tdp->td_any_worker_thread_available_condition);
		pthread_mutex_unlock(&tdp->td_any_worker_thread_available_mutex);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_worker_thread: Target %d WorkerThread %d: WARNING: Bad status from sem_post on sem_any_worker_thread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number,
				status,
				errno);
		}

	} // end of WHILE loop 

} /* end of xdd_worker_thread() */

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
