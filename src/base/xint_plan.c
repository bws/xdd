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
#include "xni.h"

/*----------------------------------------------------------------------------*/
/* xint_plan_data_initialization() - Initialize a xdd plan variables  
 */
xdd_plan_t* xint_plan_data_initialization() {
    xdd_plan_t* planp;
	char			errmsg[1024];


	// Initialize the plan structure
	planp = (xdd_plan_t *)malloc(sizeof(xdd_plan_t));
	if (0 == planp) {
		sprintf(errmsg,"%s: Error allocating memory for xdd_plan struct\n",xgp->progname);
		perror(errmsg);
		return(0);
	}

	memset(planp,0,sizeof(*planp));

	// This is the default in order to avoid all the warning messages, overridden by selecting -maxall andor -proclock + -memlock
	planp->plan_options = (PLAN_NOPROCLOCK|PLAN_NOMEMLOCK);
	
	// Some of these settings may seem redundant because they are set to zero after clearing the entire data structure but they
	// are basically place holders in case their default values need to be changed.
	planp->passes = DEFAULT_PASSES;
	planp->current_pass_number = 0;
	planp->pass_delay = DEFAULT_PASSDELAY;
	planp->pass_delay_accumulated_time = 0;
	planp->ts_binary_filename_prefix = DEFAULT_TIMESTAMP;
	planp->syncio = 0;
	planp->target_offset = 0;
	planp->number_of_targets = 0;
	planp->ts_output_filename_prefix = 0;
	/* Information needed to access the Global Time Server */
	planp->gts_hostname = 0;
	planp->gts_addr = 0;
	planp->gts_time = 0;
	planp->gts_port = DEFAULT_PORT;
	planp->gts_bounce = DEFAULT_BOUNCE;
	planp->gts_delta = 0;
	planp->gts_seconds_before_starting = 0; /* number of seconds before starting I/O */
	planp->restart_frequency = 0;
	planp->number_of_iothreads = 0;    /* number of threads spawned for all targets */
	planp->estimated_end_time = 0;     /* The time at which this run (all passes) should end */
	planp->number_of_processors = 0;   /* Number of processors */ 
	planp->e2e_TCP_Win = DEFAULT_E2E_TCP_WINDOW_SIZE;	 /* e2e TCP Window Size */
	planp->ActualLocalStartTime = 0;   /* The time to start operations */
	planp->XDDMain_Thread = pthread_self();
	planp->heartbeat_flags = 0;  	/* used by results manager to suspend or cancel heartbeat displays */
	planp->format_string = DEFAULT_OUTPUT_FORMAT_STRING;

	return(planp);

} // End of xdd_plan_data_initialization()

/* This is called after the target substructures have been created */
int xint_plan_start(xdd_plan_t* planp, xdd_occupant_t* barrier_occupant) {
	int rc;
	
	/* Initialize subsystems */
	if (PLAN_ENABLE_XNI & planp->plan_options) {
		xni_initialize();
	}
	
	/* Initialize the plan barriers */
	rc =  xdd_init_barrier(planp, &planp->main_general_init_barrier, 2,
						   "main_general_init_barrier");
    rc += xdd_init_barrier(planp, &planp->main_targets_waitforstart_barrier,
						   planp->number_of_targets+1,
						   "main_targets_waitforstart_barrier");
    rc += xdd_init_barrier(planp, &planp->main_targets_syncio_barrier,
						   planp->number_of_targets,
						   "main_targets_syncio_barrier");
    rc += xdd_init_barrier(planp, &planp->main_results_final_barrier, 2,
						   "main_results_final_barrier");
    if (0 != rc)  {
        fprintf(stderr,"%s: xint_start_plan: ERROR: Cannot initialize the plan barriers - exiting now.\n",xgp->progname);
        xdd_destroy_all_barriers(planp);
        return -1;
    }

	/* Add a barrier occupant for tracking barrier participants */
	xdd_init_barrier_occupant(barrier_occupant, "XDD_MAIN", XDD_OCCUPANT_TYPE_MAIN, NULL);

	/* Start the target threads which moves them all into a waiting barrier */
	rc = xint_plan_start_targets(planp);
	if (rc < 0) {
		fprintf(xgp->errout,
				"%s: xdd_start_plan: ERROR: Could not start target threads\n",
				xgp->progname);
		xdd_destroy_all_barriers(planp);
		return -1;
	}

	/* Start the Results Manager */
	xint_plan_start_results_manager(planp);

	/* Start a heartbeat monitor if necessary */
	xint_plan_start_heartbeat(planp);

	/* Start a restart monitor if necessary */
	xint_plan_start_restart_monitor(planp);

	/* Start interactive mode if requested */
	xint_plan_start_interactive(planp);

	/* Record a start time and release the target threads from the barrier */
	nclk_now(&planp->run_start_time);
	xdd_barrier(&planp->main_targets_waitforstart_barrier, barrier_occupant,1);
	
	return 0;
}

/*----------------------------------------------------------------------------*/
/* xdd_start_targets() - Will start all the Target threads. Target Threads are 
 * responsible for starting their own worker threads. The basic idea here is that 
 * there are N targets and each target can have X instances (or worker threads as they 
 * are referred to) where X is the queue depth. The Target thread is responsible '
 * for managing the worker threads.
 * The "TARGET_DATA" array contains pointers to the TARGET_DATA for each of the Targets.
 * The Target thread creation process is serialized such that when a thread 
 * is created, it will perform its initialization tasks and then enter the 
 * "serialization" barrier. Upon entering this barrier, the *while* loop that 
 * creates these threads will be released to create the next thread. 
 * In the meantime, the thread that just finished its initialization will enter 
 * the initialization barrier waiting for all threads to become initialized 
 * before they are all released. Make sense? I didn't think so.
 */
int xint_plan_start_targets(xdd_plan_t *planp) {
	int32_t		target_number;	// Target number to work on
	int32_t		status;			// Status of a subroutine call
	target_data_t		*tdp;				// pointer to the PTS for this Worker Thread
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier


	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_TARGETS", XDD_OCCUPANT_TYPE_MAIN, NULL);
	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number]; /* Get the target_datap for this target */
		/* Create the new thread */
		tdp->td_target_number = target_number;
		tdp->td_planp = planp;
		status = pthread_create(&tdp->td_thread, NULL, xdd_target_thread, tdp);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_targets: ERROR: Cannot create thread for target %d",
				xgp->progname, 
				target_number);
			perror("Reason");
			return(-1);
		}
		// Wait for the Target Thread to complete initialization and then create the next one
		xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,1);

		// If anything in the previous target *initialization* fails, then everything needs to stop right now.
		if (xgp->abort == 1) { 
			fprintf(xgp->errout,"%s: xdd_start_targets: ERROR: xdd thread %d aborting due to previous initialization failure\n",
				xgp->progname,
				tdp->td_target_number);
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
void xint_plan_start_results_manager(xdd_plan_t *planp) {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier


	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_RESULTS_MANAGER", XDD_OCCUPANT_TYPE_MAIN, NULL);
	status = pthread_create(&planp->Results_Thread, NULL, xdd_results_manager, planp);
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_start_results_manager: ERROR: Could not start results manager\n", xgp->progname);
		xdd_destroy_all_barriers(planp);
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}
	// Enter this barrier and wait for the results monitor to initialize
	xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,1);


} // End of xdd_start_results_manager()
/*----------------------------------------------------------------------------*/
/* xdd_start_heartbeat() - Will start the heartbeat monitor thread and
 * wait for it to initialize. If something goes wrong then this routine will
 * display a WARNING message and continue on its merry way.
 */
void xint_plan_start_heartbeat(xdd_plan_t *planp) {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_HEARTBEAT", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (xgp->global_options & GO_HEARTBEAT) {
		status = pthread_create(&planp->Heartbeat_Thread, NULL, xdd_heartbeat, planp);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_heartbeat: ERROR: Could not start heartbeat\n", xgp->progname);
			fflush(xgp->errout);
		}
		// Enter this barrier and wait for the heartbeat monitor to initialize
		xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_heartbeat()
/*----------------------------------------------------------------------------*/
/* xdd_start_restart_monitor() - Will start the Restart Monitor thread and
 * wait for it to initialize. If something goes wrong then this routine will
 * destroy all barriers and exit to the OS.
 */
void xint_plan_start_restart_monitor(xdd_plan_t *planp) {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_RESTART_MONITOR", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (planp->restart_frequency) {
		status = pthread_create(&planp->Restart_Thread, NULL, xdd_restart_monitor, planp);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_restart_monitor: ERROR: Could not start restart monitor\n", xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers(planp);
			exit(XDD_RETURN_VALUE_INIT_FAILURE);
		}
		// Enter this barrier and wait for the restart monitor to initialize
		xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_restart_monitor()
/*----------------------------------------------------------------------------*/
/* xdd_start_interactive() - Will start the Interactive Command Processor Thread
 */
void xint_plan_start_interactive(xdd_plan_t *planp) {
	int32_t		status;			// Status of a subroutine call
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "XDDMAIN_START_INTERACTIVE", XDD_OCCUPANT_TYPE_MAIN, NULL);
	if (xgp->global_options & GO_INTERACTIVE) {
		status = pthread_create(&planp->Interactive_Thread, NULL, xdd_interactive, (void *)planp);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_start_interactive: ERROR: Could not start interactive control processor\n", xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers(planp);
			exit(XDD_RETURN_VALUE_INIT_FAILURE);
		}
		// Enter this barrier and wait for the restart monitor to initialize
		xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,1);
	}

} // End of xdd_start_interactive()

/*
 * Plan finalize
 */
void xint_plan_finalize(xdd_plan_t *planp) {
/* Initialize subsystems */
	if (PLAN_ENABLE_XNI & planp->plan_options)
		xni_finalize();
}

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
