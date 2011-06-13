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
/* xdd_init_globals() - Initialize a xdd global variables  
 */
void
xdd_init_globals(char *progname) {

	xgp = (xdd_global_data_t *)malloc(sizeof(struct xdd_global_data));
	if (xgp == 0) {
		fprintf(stderr,"%s: Cannot allocate %d bytes of memory for global variables!\n",progname, (int)sizeof(struct xdd_global_data));
		perror("Reason");
		exit(1);
	}
	memset(xgp,0,sizeof(struct xdd_global_data));

	xgp->progname = progname;
	// This is the default in order to avoid all the warning messages, overridden by selecting -maxall andor -proclock + -memlock
	xgp->global_options = (GO_NOPROCLOCK|GO_NOMEMLOCK);
	xgp->output = stdout;
	xgp->output_filename = "stdout";
	xgp->errout = stderr;
	xgp->errout_filename = "stderr";
	xgp->csvoutput_filename = "";
	xgp->csvoutput = NULL;
	xgp->combined_output_filename = ""; /* name of the combined output file */
	xgp->combined_output = NULL;       /* Combined output file */
	xgp->id_firsttime = 1;
	
	/* Initialize the global variables */
	// Some of these settings may seem redundant because they are set to zero after clearing the entire data structure but they
	// are basically place holders in case their default values need to be changed.
	xgp->passes = DEFAULT_PASSES;
	xgp->current_pass_number = 0;
	xgp->pass_delay = DEFAULT_PASSDELAY;
	xgp->pass_delay_accumulated_time = 0;
	xgp->ts_binary_filename_prefix = DEFAULT_TIMESTAMP;
	xgp->syncio = 0;
	xgp->target_offset = 0;
	xgp->number_of_targets = 0;
	xgp->ts_output_filename_prefix = 0;
	xgp->max_errors = 0; /* set to total_ops later when we know what it is */
	xgp->max_errors_to_print = DEFAULT_MAX_ERRORS_TO_PRINT;  /* max number of errors to print information about */ 
	/* Information needed to access the Global Time Server */
	xgp->gts_hostname = 0;
	xgp->gts_addr = 0;
	xgp->gts_time = 0;
	xgp->gts_port = DEFAULT_PORT;
	xgp->gts_bounce = DEFAULT_BOUNCE;
	xgp->gts_delta = 0;
	xgp->gts_seconds_before_starting = 0; /* number of seconds before starting I/O */
	xgp->restart_frequency = 0;
	xgp->run_error_count_exceeded = 0;       /* The alarm that goes off when the number of errors for this run has been exceeded */
	xgp->run_time_expired = 0;       /* The alarm that goes off when the total run time has been exceeded */
	xgp->run_complete = 0; 
	xgp->abort = 0;       /* abort the run due to some catastrophic failure */
	xgp->number_of_iothreads = 0;    /* number of threads spawned for all targets */
	xgp->run_time = 0;               /* Length of time to run all targets, all passes */
	xgp->run_time_ticks = 0;         /* Length of time to run all targets, all passes in high-res clock ticks */
	xgp->estimated_end_time = 0;     /* The time at which this run (all passes) should end */
	xgp->number_of_processors = 0;   /* Number of processors */ 
	xgp->random_initialized = 0;     /* Random number generator has not been initialized  */
	xgp->e2e_TCP_Win = DEFAULT_E2E_TCP_WINDOW_SIZE;	 /* e2e TCP Window Size */
	xgp->ActualLocalStartTime = 0;   /* The time to start operations */
	xgp->XDDMain_Thread = pthread_self();
	xgp->heartbeat_holdoff = 0;  	/* used by results manager to suspend or cancel heartbeat displays */
	xgp->format_string = DEFAULT_OUTPUT_FORMAT_STRING;

	// Allocate a suitable buffer for the run ID ASCII string
	xgp->id = (char *)malloc(MAX_IDLEN);
	if (xgp->id == NULL) {
		fprintf(xgp->errout,"%s: Cannot allocate %d bytes of memory for ID string\n",xgp->progname,MAX_IDLEN);
		exit(1);
	}
	strcpy(xgp->id,DEFAULT_ID);

} /* end of xdd_init_globals() */

