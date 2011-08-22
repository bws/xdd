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
	char *c;
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier
	int32_t	return_value;

	
	status = xdd_initialization(argc, argv);
	if (status < 0) {
		fprintf(xgp->errout,"%s: Error during initialization\n",xgp->progname);
		xdd_destroy_all_barriers();
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}

	// Initialize the barrier used to synchronize all the initialization routines
	status =  xdd_init_barrier(&xgp->main_general_init_barrier, 2, "main_general_init_barrier");
	status += xdd_init_barrier(&xgp->main_targets_waitforstart_barrier, xgp->number_of_targets+1, "main_targets_waitforstart_barrier");
	status += xdd_init_barrier(&xgp->main_targets_syncio_barrier, xgp->number_of_targets, "main_targets_syncio_barrier");
	status += xdd_init_barrier(&xgp->main_results_final_barrier, 2, "main_results_final_barrier");
	if (status < 0)  {
		fprintf(stderr,"%s: xdd_main: ERROR: Cannot initialize the main barriers - exiting now.\n",xgp->progname);
		xdd_destroy_all_barriers();
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}
	xdd_init_barrier_occupant(&barrier_occupant, "XDD_MAIN", XDD_OCCUPANT_TYPE_MAIN, NULL);

	// Start up all the necessary threads 
	// Start the Target Threads 
	// The Target Threads will start their QThreads as part of their initialization process
	status = xdd_start_targets();
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_main: ERROR: Could not start target threads\n", xgp->progname);
		xdd_destroy_all_barriers();
		exit(XDD_RETURN_VALUE_TARGET_START_FAILURE);
	}

	// At this point the the target threads are all waiting at the Initialization Barrier to start.
	// Before they can start we need to start the Results Manager and optionally the 
	// heartbeat and/or restart threads.

	/* Start the Results Manager */
	xdd_start_results_manager();

	/* start a heartbeat monitor if necessary */
	xdd_start_heartbeat();

	/* start a restart monitor if necessary */
	xdd_start_restart_monitor();

	/* start a restart monitor if necessary */
	xdd_start_interactive();

	/* Record the approximate time that the targets will actually start running */
	nclk_now(&xgp->run_start_time);

	// Entering this barrier will tell all the Target threads to begin now that all the monitor threads have started
	xdd_barrier(&xgp->main_targets_waitforstart_barrier,&barrier_occupant,1);
DFLOW("\n----------------------All targets should start now-------------------------\n");

	// At this point all the Target threads are running and we will enter the final_barrier 
	// waiting for them to finish or exit if this is just a dry run
	if (xgp->global_options & GO_DRYRUN) {
		// Cleanup the semaphores and barriers 
		xdd_destroy_all_barriers();
		return(XDD_RETURN_VALUE_SUCCESS);
	}

	// Wait for the Results Manager to get here at which time all targets have finished
	xdd_barrier(&xgp->main_results_final_barrier,&barrier_occupant,1);

	// Display the Ending Time for this run
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	if (xgp->canceled) {
		fprintf(xgp->output,"Ending time for this run, %s This run was canceled\n",c);
		return_value = XDD_RETURN_VALUE_CANCELED;
	} else if (xgp->abort) {
		fprintf(xgp->output,"Ending time for this run, %s This run terminated with errors\n",c);
		return_value = XDD_RETURN_VALUE_IOERROR;
	} else {
		fprintf(xgp->output,"Ending time for this run, %s This run terminated normally\n",c);
		return_value = XDD_RETURN_VALUE_SUCCESS;
	}
	if (xgp->csvoutput) {
		if (xgp->canceled) {
			fprintf(xgp->csvoutput,"Ending time for this run, %s This run was canceled\n",c);
		} else if (xgp->abort) {
			fprintf(xgp->csvoutput,"Ending time for this run, %s This run terminated with errors\n",c);
		} else {
			fprintf(xgp->csvoutput,"Ending time for this run, %s This run terminated normally\n",c);
		}
	}

	// Cleanup the semaphores and barriers 
	xdd_destroy_all_barriers();

	/* Time to leave... sigh */
	return(return_value);
} /* end of main() */
 
/*----------------------------------------------------------------------------*/
/* xdd_start_targets() - Will start all the Target threads. Target Threads are 
 * responsible for starting their own qthreads. The basic idea here is that 
 * there are N targets and each target can have X instances (or qthreads as they 
 * are referred to) where X is the queue depth. The Target thread is responsible '
 * for managing the qthreads.
 * The "PTDS" array contains pointers to the PTDSs for each of the Targets.
 * The Target thread creation process is serialized such that when a thread 
 * is created, it will perform its initialization tasks and then enter the 
 * "serialization" barrier. Upon entering this barrier, the *while* loop that 
 * creates these threads will be released to create the next thread. 
 * In the meantime, the thread that just finished its initialization will enter 
 * the initialization barrier waiting for all threads to become initialized 
 * before they are all released. Make sense? I didn't think so.
 */
int32_t
xdd_start_targets() {
	int32_t		target_number;	// Target number to work on
	int32_t		status;			// Status of a subroutine call
	ptds_t		*p;				// pointer to the PTS for this QThread
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier


	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_TARGETS", XDD_OCCUPANT_TYPE_MAIN, NULL);
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number]; /* Get the ptds for this target */
		/* Create the new thread */
		p->my_target_number = target_number;
		status = pthread_create(&p->target_thread, NULL, xdd_target_thread, p);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_targets: ERROR: Cannot create thread for target %d",
				xgp->progname, 
				target_number);
			perror("Reason");
			return(-1);
		}
		// Wait for the Target Thread to complete initialization and then create the next one
		xdd_barrier(&xgp->main_general_init_barrier,&barrier_occupant,1);

		// If anything in the previous target *initialization* fails, then everything needs to stop right now.
		if (xgp->abort == 1) { 
			fprintf(xgp->errout,"%s: xdd_start_targets: ERROR: xdd thread %d aborting due to previous initialization failure\n",
				xgp->progname,
				p->my_target_number);
			return(-1);
		}
	}


	return(0);
} // End of xdd_start_targets()

/*----------------------------------------------------------------------------*/
/* xdd_start_results_manager() - Will start the Results Manager thread and
 * wait for it to initialize. If something goes wrong then this routine will
 * destroy all barriers and exit to the OS.
 */
void
xdd_start_results_manager() {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier


	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_RESULTS_MANAGER", XDD_OCCUPANT_TYPE_MAIN, NULL);
	status = pthread_create(&xgp->Results_Thread, NULL, xdd_results_manager, (void *)(unsigned long)0);
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_start_results_manager: ERROR: Could not start results manager\n", xgp->progname);
		xdd_destroy_all_barriers();
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}
	// Enter this barrier and wait for the results monitor to initialize
	xdd_barrier(&xgp->main_general_init_barrier,&barrier_occupant,1);


} // End of xdd_start_results_manager()
/*----------------------------------------------------------------------------*/
/* xdd_start_heartbeat() - Will start the heartbeat monitor thread and
 * wait for it to initialize. If something goes wrong then this routine will
 * display a WARNING message and continue on its merry way.
 */
void
xdd_start_heartbeat() {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_HEARTBEAT", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (xgp->global_options & GO_HEARTBEAT) {
		status = pthread_create(&xgp->Heartbeat_Thread, NULL, xdd_heartbeat, (void *)(unsigned long)0);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_heartbeat: ERROR: Could not start heartbeat\n", xgp->progname);
			fflush(xgp->errout);
		}
		// Enter this barrier and wait for the heartbeat monitor to initialize
		xdd_barrier(&xgp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_heartbeat()
/*----------------------------------------------------------------------------*/
/* xdd_start_restart_monitor() - Will start the Restart Monitor thread and
 * wait for it to initialize. If something goes wrong then this routine will
 * destroy all barriers and exit to the OS.
 */
void
xdd_start_restart_monitor() {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_RESTART_MONITOR", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (xgp->restart_frequency) {
		status = pthread_create(&xgp->Restart_Thread, NULL, xdd_restart_monitor, (void *)(unsigned long)0);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_restart_monitor: ERROR: Could not start restart monitor\n", xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(XDD_RETURN_VALUE_INIT_FAILURE);
		}
		// Enter this barrier and wait for the restart monitor to initialize
		xdd_barrier(&xgp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_restart_monitor()
/*----------------------------------------------------------------------------*/
/* xdd_start_interactive() - Will start the Interactive Command Processor Thread
 */
void
xdd_start_interactive() {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_INTERACTIVE", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (xgp->global_options & GO_INTERACTIVE) {
		status = pthread_create(&xgp->Interactive_Thread, NULL, xdd_interactive, (void *)(unsigned long)0);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_interactive: ERROR: Could not start interactive control processor\n", xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(XDD_RETURN_VALUE_INIT_FAILURE);
		}
		// Enter this barrier and wait for the restart monitor to initialize
		xdd_barrier(&xgp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_interactive()
