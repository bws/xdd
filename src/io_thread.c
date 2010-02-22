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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_io_thread() - control of the I/O for the specified reqsize      
 * Notes: Two thread_barriers are used due to an observed race-condition
 * in the xdd_barrier() code, most likely in the semaphore op,
 * that causes some threads to be woken up prematurely and getting the
 * intended synchronization out of sync hanging subsequent threads.
 */
void *
xdd_io_thread(void *pin) {
	int32_t  	i;  /* working variables */
	int32_t  	status;


	ptds_t *p = (ptds_t *)pin; 

	status = xdd_io_thread_init(p);
	if (status == FAILED) {
		fprintf(xgp->errout,"%s: xdd_io_thread: Aborting target %d due to previous initialization failure.\n",
			xgp->progname,
			p->my_target_number);
		fflush(xgp->errout);
		xgp->abort_io = 1; // This will prevent all other threads from being created...
	}

	/* Enter the serializer barrier so that the next thread can start */
	xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
	if ( xgp->abort_io == 1) // Something went wrong during thread initialization so let's just leave
		return(0);

	/* Enter the initialization barrier - we get released after all the montior threads are started */
	xdd_barrier(&xgp->initialization_barrier);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN)
		return(0);

	// OK - let's get the road on the show here...
	xgp->run_complete = 0; 
	p->thread_barrier_index = 0;
	/* Start the main pass loop */
	for (p->my_current_pass_number = 1; p->my_current_pass_number <= xgp->passes; p->my_current_pass_number++) {
		/* Before we get started, check to see if we need to reset the 
		 * run_status in case we are using the start trigger.
		 */
		if (p->target_options & TO_WAITFORSTART) 
			p->run_status = 0;
		/* place barrier to ensure that everyone starts at same time */
		xdd_barrier(&xgp->thread_barrier[p->thread_barrier_index]);
		p->thread_barrier_index ^= 1; /* toggle the barrier index */

		/* Check to see if any of the other threads have aborted */
		if (xgp->abort_io) {
			fprintf(xgp->errout,"%s: xdd_io_thread: Aborting I/O for target %d\n",xgp->progname,p->my_target_number);
			fflush(xgp->errout);
			break;
		}
		p->pass_complete = 0;

	 	/* Do the actual I/O Loop */
		xdd_io_loop(p);

		p->pass_complete = 1;
		// This first barrier is where the results_manager() is waiting in 
		// order to process/display the results for this pass
		// Once all the threads reach this barrier, the results_manager() will 
		// be released to process the results and the threads will enter the
		// next barrier waiting for the results_manager() to complete.
		p->my_current_state = CURRENT_STATE_BARRIER;
		xdd_barrier(&xgp->results_pass_barrier[p->results_pass_barrier_index]);
		p->results_pass_barrier_index ^= 1; /* toggle the barrier index */

		// At this point all the threads have completed their pass and have
		// passed thru the previous barrier releasing the results_manager() 
		// to process/display the results for this pass. 

		// Wait at this barrier for the results_manager() to process/display 
		// the results for this last pass
		xdd_barrier(&xgp->results_display_barrier[p->results_display_barrier_index]);
		p->results_display_barrier_index ^= 1; /* toggle the barrier index */

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
		/* Insert a delay of "pass_delay" seconds if requested */
		if (xgp->pass_delay > 0)
			sleep(xgp->pass_delay);
		/* Re-randomize the seek list if requested */
		if (p->target_options & TO_PASS_RANDOMIZE) {
			p->seekhdr.seek_seed++;
			xdd_init_seek_list(p);
		}

		/* Add an offset to each location in the seeklist */
		for (i = 0; i < p->qthread_ops; i++)
			p->seekhdr.seeks[i].block_location += p->pass_offset;

		/* Close current file, create a new target file, and open the new (or existing) file is requested */
		if ((p->target_options & TO_CREATE_NEW_FILES) || 
		    (p->target_options & TO_REOPEN) || 
		    (p->target_options & TO_RECREATE)) {
			// Close the existing target
#if (WIN32)
			CloseHandle(p->fd);
#else
			close(p->fd);
#endif
			if (p->target_options & TO_CREATE_NEW_FILES) { // Create a new file name for this target
				sprintf(p->targetext,"%08d",p->my_current_pass_number+1);
			}
			// Check to see if this is the last pass - in which case do not create a new file because it will not get used
			if (p->my_current_pass_number == xgp->passes)
				continue;
                        // If we need to "recreate" the file for each pass then we should delete it here before we re-open it 
			if (p->target_options & TO_RECREATE)	
#ifdef WIN32
				DeleteFile(p->target);
#else
				unlink(p->target);
#endif
			/* open the old/new/recreated target file */
			status = xdd_target_open(p);
			if (status != 0) { /* error openning target */
				fprintf(xgp->errout,"%s: xdd_io_thread: Aborting I/O for target %d due to open failure\n",xgp->progname,p->my_target_number);
				fflush(xgp->errout);
				xgp->abort_io = 1;
			}
		} // End of section that creates new target files and/or re-opens the target

	} /* end of FOR loop p->my_current_pass_number */

	
	/* indicate that this thread completed this this run */
	p->run_complete = 1;
	xgp->run_complete = 1; 

	// This first barrier is where the results_manager() is waiting in 
	// order to process/display the results for this pass
	// Once all the threads reach this barrier, the results_manager() will 
	// be released to process the results and the threads will enter the
	// next barrier waiting for the results_manager() to complete.
	xdd_barrier(&xgp->results_pass_barrier[p->results_pass_barrier_index]);
	xdd_barrier(&xgp->results_run_barrier);

	// At this point all the threads have completed the run and have
	// passed thru the previous barrier releasing the results_manager() 
	// to process/display the results for this run. 

	// Wait at this barrier for the results_manager() to process/display 
	// the results for the run and the go do the cleanup stuff
	xdd_barrier(&xgp->results_display_final_barrier);


	// Display output and cleanup 
	status = xdd_io_thread_cleanup(p);

	// Wait for all the other threads to complete
	xdd_barrier(&xgp->final_barrier);

    return(0);
} /* end of xdd_io_thread() */
 
