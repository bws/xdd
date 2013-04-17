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
#include "xint.h"
/*----------------------------------------------------------------------------*/
/* xdd_plan_data_initialization() - Initialize a xdd plan variables  
 */
xdd_plan_t* xdd_plan_data_initialization() {
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
	planp->heartbeat_holdoff = 0;  	/* used by results manager to suspend or cancel heartbeat displays */
	planp->format_string = DEFAULT_OUTPUT_FORMAT_STRING;

	return(planp);

} // End of xdd_plan_data_initialization()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
