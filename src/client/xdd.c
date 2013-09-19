/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

/*
   xdd - A sophisticated I/O performance measurement and analysis tool/utility
*/
#define XDDMAIN
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* This is the main entry point for xdd. This is where it all begins and ends.
 */
int32_t
main(int32_t argc,char *argv[]) {
	int32_t 			status;
	int32_t				i;
	char 				*c;
	xdd_occupant_t		barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier
	int32_t				return_value;
    xdd_plan_t 			*planp;

	
	// Initialize the global_data structure
	// See global_data.c
	xint_global_data_initialization(argv[0]);
	if (0 == xgp) {
		fprintf(stderr,"%s: Error allocating memory for global_data struct\n",argv[0]);
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}
	
	// Initialize the plan structure
	// See xint_plan.c
	planp = xint_plan_data_initialization();
	if (0 == planp) {
		fprintf(stderr,"%s: Error allocating memory for xdd_plan struct\n",xgp->progname);
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}

	// Initialize everything else
	// See initialization.c
	status = xdd_initialization(argc, argv, planp);
	if (status < 0) {
		fprintf(xgp->errout,"%s: Error during initialization\n",xgp->progname);
		xdd_destroy_all_barriers(planp);
		exit(XDD_RETURN_VALUE_INIT_FAILURE);
	}

	// Start the plan
	// See xint_plan.c
	status = xint_plan_start(planp, &barrier_occupant);
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_main: ERROR: Could not start plan\n", xgp->progname);
		xdd_destroy_all_barriers(planp);
		exit(XDD_RETURN_VALUE_TARGET_START_FAILURE);
	}
DFLOW("\n----------------------All targets should start now-------------------------\n");

	// At this point all the Target threads are running and we will enter the final_barrier 
	// waiting for them to finish or exit if this is just a dry run
	if (xgp->global_options & GO_DRYRUN) {
		// Cleanup the semaphores and barriers 
		xdd_destroy_all_barriers(planp);
		return(XDD_RETURN_VALUE_SUCCESS);
	}

	// Wait for the Results Manager to get here at which time all targets have finished
	xdd_barrier(&planp->main_results_final_barrier, &barrier_occupant, 1);

	// Display the Ending Time for this run
	planp->current_time_for_this_run = time(NULL);
	c = ctime(&planp->current_time_for_this_run);

	status = 0;
	for (i=0; i<MAX_TARGETS; i++) 
			status += planp->target_errno[i];
	if (xgp->canceled) {
		fprintf(xgp->output,"Ending time for this run, %s This run was canceled\n",c);
		return_value = XDD_RETURN_VALUE_CANCELED;
	} else if (xgp->abort) {
		fprintf(xgp->output,"Ending time for this run, %s This run terminated with errors\n",c);
		return_value = XDD_RETURN_VALUE_IOERROR;
	} else if (status != 0) {
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
	xdd_destroy_all_barriers(planp);

	/* Time to leave... sigh */
	return(return_value);
} /* end of main() */
 
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
