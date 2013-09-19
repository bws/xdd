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
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_target_thread() - This thread represents a single target device or file.
 * One Target thread is started for each Target device or file by the xdd_target_start() subroutine.
 * Each Target thread will subsequently start 1 to N Worker Threads where N is the queue depth
 * for the specific target. 
 * When the Target thread is released it will get Worker Threads that are Available 
 * and give each one a single location to access, an operation
 * to perform (read or write), the amount of data to transfer, and the global clock
 * time at which to start the operation if throttling is being used. 
 * When the Worker Thread has completed its operation it will report its status, 
 * update counters in the Target Data Struct, and make itself Available.
 */
void *
xdd_target_thread(void *pin) {
	int32_t  		status;		// Status of various function calls
	target_data_t	*tdp;		// Pointer to this Target's Data Struct
	xdd_plan_t		*planp;

	tdp = (target_data_t *)pin;
	planp = tdp->td_planp;

	// Call xint_target_thread_init() to create all the Worker Threads for this target
	status = xint_target_init(tdp);
	if (status != 0 ) {
		fprintf(xgp->errout,"%s: xdd_target_thread: ERROR: Aborting due to previous initialization failure for target number %d name '%s'\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		fflush(xgp->errout);
		xgp->abort = 1; // This will tell xdd_start_targets() to abort
	}

	// Enter this barrier to release xdd_start_targets() 
	xdd_barrier(&planp->main_general_init_barrier,&tdp->td_occupant,0);
	if ( xgp->abort == 1) // Something went wrong during thread initialization so let's just leave
		return(0);

	// Enter the "wait for start"  barrier and wait here until all other Target Threads have started
	// After all other Target Threads have started, xdd_main() will enter this barrier thus releasing all Target Threads
	xdd_barrier(&planp->main_targets_waitforstart_barrier,&tdp->td_occupant,0);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN) {
		return(0);
	}

	/* Start the main pass loop */
	while (1) {
		// Perform a single pass
	    xdd_target_pass(planp, tdp);

		// Check to see if we got canceled or if we need to abort for some reason
		if ((xgp->canceled) || (xgp->abort) || (tdp->td_abort)) {
			break;
		}

		/* Check to see if the run time has been exceeded - if so, then exit this loop.
		 * Otherwise, if there was a run time specified and we have not reached that run time
		 * and this is the last pass, then add one to the pass count so that we keep going.
		 */
		if (planp->run_time_ticks > 0) {
			if (xgp->run_time_expired) /* This is the alarm that goes off when the total run time specified has been exceeded */
				break; /* Time to leave */
			else if (tdp->td_counters.tc_pass_number == planp->passes) /* Otherwise if we just finished the last pass, we need to keep going */
				planp->passes++;
		}

		// If this is the Destination side of an E2E then set the RESTART SUCCESSFUL COMPLETION
		// flag in the restart structure if there is one...
		if (tdp->td_target_options & TO_E2E_DESTINATION) {
			if( tdp->td_restartp) {
				pthread_mutex_lock(&tdp->td_restartp->restart_lock);
				tdp->td_restartp->flags |= RESTART_FLAG_SUCCESSFUL_COMPLETION;
				if (tdp->td_restartp->fp) {
					// Display an appropriate Successful Completion in the restart file and close it
					// Seek to the beginning of the file 
					status = fseek(tdp->td_restartp->fp, 0L, SEEK_SET);
					if (status < 0) {
						fprintf(xgp->errout,"%s: Target %d: WARNING: Seek to beginning of restart file %s failed\n",
							xgp->progname,
							tdp->td_target_number,
							tdp->td_restartp->restart_filename);
						perror("Reason");
					}
					
					// Put the Normal Completion information into the restart file
					fprintf(tdp->td_restartp->fp,"File Copy Operation completed successfully.\n");
					fprintf(tdp->td_restartp->fp,"%lld bytes written to file %s\n",(long long int)tdp->td_current_bytes_completed,tdp->td_target_full_pathname);
					fflush(tdp->td_restartp->fp);
					fclose(tdp->td_restartp->fp);
				}
				pthread_mutex_unlock(&tdp->td_restartp->restart_lock);
			} 
		} // End of IF clause that deals with a restart file if there is one

		// Check to see if we completed all passes in this run
 		if (tdp->td_counters.tc_pass_number >= planp->passes)
			break; 

        /* Increment pass number and start work for next pass */
        tdp->td_counters.tc_pass_number++;

		/* Close current file, create a new target file, and open the new (or existing) file is requested */
		if ((tdp->td_target_options & TO_CREATE_NEW_FILES) || 
		    (tdp->td_target_options & TO_REOPEN) || 
		    (tdp->td_target_options & TO_RECREATE)) {
			// Tell all Worker Threads to close and reopen the new file
			xdd_target_reopen(tdp);
		}
	} /* end of FOR loop tdp->td_counters.tc_pass_number */

	// If this is an E2E operation and we had gotten canceled - just return
	if ((tdp->td_target_options & TO_ENDTOEND) && (xgp->canceled))
		exit(2); 
	
	// Indicate that this run has completed
	xgp->run_complete = 1; // This is the "global" run_complete

	// Enter this barrier and wait for all other Target Threads to get here. 
	// After all Target Threads get here, the results_manager will display
	// the run results.
	// Note: The results manager at this point is actually waiting at the 
	// "results_target_startpass_barrier" because it thinks it might need
	// to display results for yet another pass. However, when all the
	// Target Threads enter the "startpass" barrier, the results manager
	// will be released and immediately notice that the "xgp->run_complete"
	// flag is set and therefore display results for this last pass
	// and then go on to displaying results for the run.
	xdd_barrier(&planp->results_targets_startpass_barrier,&tdp->td_occupant,0);

	// All Target Threads wait here until the results_manager displays the pass results.
	xdd_barrier(&planp->results_targets_endpass_barrier,&tdp->td_occupant,0);

	// All Target Threads wait here until the results_manager displays the run results.
	xdd_barrier(&planp->results_targets_display_barrier,&tdp->td_occupant,0);

	// At this point all the Target Threads have completed the run and have
	// passed thru the previous barriers and the results_manager() 
	// has processed and displayed the results for this run. 
	// Now it is time to cleanup after this Target Thread

	// Display output and cleanup 
	xdd_target_thread_cleanup(tdp);

	// Wait for all the other threads to complete their cleanup
	xdd_barrier(&planp->results_targets_waitforcleanup_barrier,&tdp->td_occupant,0);

    return(0);
} /* end of xdd_target_thread() */

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
