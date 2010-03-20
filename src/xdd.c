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
   xdd - A sophisticated I/O performance measurement and analysis tool/utility
*/
#define XDDMAIN /* This is needed to make the xdd_globals pointer a static variable here and an extern everywhere else as defined in xdd.h */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* This is the main entry point for xdd. This is where it all begins and ends.
 */
int32_t
main(int32_t argc,char *argv[]) {
	int32_t status;
	pthread_t heartbeat_thread;
	pthread_t results_thread;
	pthread_t restart_thread;
	char *c;

	
	status = xdd_initialization(argc, argv);
	if (status < 0) {
		fprintf(xgp->errout,"%s: Error during initialization\n",xgp->progname);
		xdd_destroy_all_barriers();
		exit(1);
	}

	// Start the target threads and associated qthreads
	status = xdd_target_start();
	if (status < 0) {
		fprintf(xgp->errout,"%s: Error during initialization\n",xgp->progname);
		xdd_destroy_all_barriers();
		exit(1);
	}

	/* Start the Results Manager */
	status = pthread_create(&results_thread, NULL, xdd_results_manager, (void *)(unsigned long)0);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_main: Error creating Results Manager thread", xgp->progname);
		fflush(xgp->errout);
	}
	// Enter this barrier and wait for the results monitor to initialize
	xdd_barrier(&xgp->results_initialization_barrier);

	/* start a heartbeat monitor if necessary */
	if (xgp->heartbeat) {
		status = pthread_create(&heartbeat_thread, NULL, xdd_heartbeat, (void *)(unsigned long)0);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_main: Error creating heartbeat monitor thread", xgp->progname);
			fflush(xgp->errout);
		}
		// Enter this barrier and wait for the heartbeat monitor to initialize
		xdd_barrier(&xgp->heartbeat_initialization_barrier);
	}

	/* start a restart monitor if necessary */
	if (xgp->restart_frequency) {
		status = pthread_create(&restart_thread, NULL, xdd_restart_monitor, (void *)(unsigned long)0);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_main: Error creating restart monitor thread", xgp->progname);
			fflush(xgp->errout);
		}
		// Enter this barrier and wait for the restart monitor to initialize
		xdd_barrier(&xgp->restart_initialization_barrier);
	}

	// Entering this barrier will tell all the target qthreads to begin now that all the monitor threads have started
	xdd_barrier(&xgp->initialization_barrier);

	// At this point all the target qthreads are running and we will enter the final_barrier waiting for them to finish
	if (xgp->global_options & GO_DRYRUN) {
		// Cleanup the semaphores and barriers 
		xdd_destroy_all_barriers();
		return(0);
	}

	// Wait for all children to finish 
	xdd_barrier(&xgp->final_barrier);

	// Display the Ending Time for this run
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(xgp->output,"Ending time for this run, %s\n",c);
	if (xgp->csvoutput)
		fprintf(xgp->csvoutput,"Ending time for this run, %s\n",c);

	// Cleanup the semaphores and barriers 
	xdd_destroy_all_barriers();

	/* Time to leave... sigh */
	return(0);
} /* end of main() */
 
