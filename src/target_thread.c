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
 *       Brad Settlemyer, DoE/ORNL
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

/*----------------------------------------------------------------------------*/
/* xdd_target_thread() - This thread represents a single target device or file.
 * One Target thread is started for each Target device or file by the xdd_target_start() subroutine.
 * Each Target thread will subsequently start 1 to N QThreads where N is the queue depth
 * for the specific target. 
 * When the Target thread is released it will get QThreads that are Available 
 * and give each one a single location to access, an operation
 * to perform (read or write), the amount of data to transfer, and the global clock
 * time at which to start the operation if throttling is being used. 
 * When the QThread has completed its operation it will report its status, 
 * update counters in the Target PTDS, and make itself Available.
 */
void *
xdd_target_thread(void *pin) {
	int32_t  	status;		// Status of various function calls
	ptds_t		*p;			// Pointer to this Target's PTDS


	p = (ptds_t *)pin; 

	// Call xdd_target_thread_init() to create all the QThreads for this target
	status = xdd_target_init(p);
	if (status != 0 ) {
		fprintf(xgp->errout,"%s: xdd_target_thread: ERROR: Aborting due to previous initialization failure for target number %d name '%s'\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		xgp->abort = 1; // This will tell xdd_start_targets() to abort
	}

	// Enter this barrier to release xdd_start_targets() 
	xdd_barrier(&xgp->main_general_init_barrier,&p->occupant,0);
	if ( xgp->abort == 1) // Something went wrong during thread initialization so let's just leave
		return(0);

	// Enter the "wait for start"  barrier and wait here until all other Target Threads have started
	// After all other Target Threads have started, xdd_main() will enter this barrier thus releasing all Target Threads
	xdd_barrier(&xgp->main_targets_waitforstart_barrier,&p->occupant,0);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN) {
		return(0);
	}

	/* Start the main pass loop */
	while (1) {
		// Perform a single pass
		xdd_targetpass(p);

		// Check to see if we got canceled
		if (xgp->canceled) 
			break;

		/* Check to see if the run time has been exceeded - if so, then exit this loop.
		 * Otherwise, if there was a run time specified and we have not reached that run time
		 * and this is the last pass, then add one to the pass count so that we keep going.
		 */
		if (xgp->runtime > 0) {
			if (xgp->run_ring) /* This is the alarm that goes off when the total run time specified has been exceeded */
				break; /* Time to leave */
			else if (p->my_current_pass_number == xgp->passes) /* Otherwise if we just finished the last pass, we need to keep going */
				xgp->passes++;
		}

		// If this is the Destination side of an E2E then set the RESTART SUCCESSFUL COMPLETION
		// flag in the restart structure if there is one...
		if (p->target_options & TO_E2E_DESTINATION) {
			if( p->restartp) {
				pthread_mutex_lock(&p->restartp->restart_lock);
				p->restartp->flags |= RESTART_FLAG_SUCCESSFUL_COMPLETION;
				if (p->restartp->fp) {
					// Display an appropriate Successful Completion in the restart file and close it
					// Seek to the beginning of the file 
					status = fseek(p->restartp->fp, 0L, SEEK_SET);
					if (status < 0) {
						fprintf(xgp->errout,"%s: Target %d: WARNING: Seek to beginning of restart file %s failed\n",
							xgp->progname,
							p->my_target_number,
							p->restartp->restart_filename);
						perror("Reason");
					}
					
					// Put the Normal Completion information into the restart file
					fprintf(p->restartp->fp,"File Copy Operation completed successfully.\n");
					fprintf(p->restartp->fp,"%lld bytes written to file %s\n",(long long int)p->my_current_bytes_xfered,p->target_full_pathname);
					fflush(p->restartp->fp);
					fclose(p->restartp->fp);
				}
				pthread_mutex_unlock(&p->restartp->restart_lock);
			} 
		} // End of IF clause that deals with a restart file if there is one

		// Check to see if we completed all passes in this run
 		if (p->my_current_pass_number >= xgp->passes)
			break; 

		/* Insert a delay of "pass_delay" seconds if requested */
		if (xgp->pass_delay > 0)
			sleep(xgp->pass_delay);

                /* Increment pass number and start work for next pass */
                p->my_current_pass_number++;

		/* Close current file, create a new target file, and open the new (or existing) file is requested */
		if ((p->target_options & TO_CREATE_NEW_FILES) || 
		    (p->target_options & TO_REOPEN) || 
		    (p->target_options & TO_RECREATE)) {
			// Tell all QThreads to close and reopen the new file
			xdd_target_reopen(p);
		}
	} /* end of FOR loop p->my_current_pass_number */

	// If this is an E2E operation and we had gotten canceled - just return
	if ((p->target_options & TO_ENDTOEND) && (xgp->canceled))
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
	xdd_barrier(&xgp->results_targets_startpass_barrier,&p->occupant,0);

	// All Target Threads wait here until the results_manager displays the pass results.
	xdd_barrier(&xgp->results_targets_endpass_barrier,&p->occupant,0);

	// All Target Threads wait here until the results_manager displays the run results.
	xdd_barrier(&xgp->results_targets_display_barrier,&p->occupant,0);

	// At this point all the Target Threads have completed the run and have
	// passed thru the previous barriers and the results_manager() 
	// has processed and displayed the results for this run. 
	// Now it is time to cleanup after this Target Thread

	// Display output and cleanup 
	xdd_target_thread_cleanup(p);

	// Wait for all the other threads to complete their cleanup
	xdd_barrier(&xgp->results_targets_waitforcleanup_barrier,&p->occupant,0);

    return(0);
} /* end of xdd_target_thread() */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
