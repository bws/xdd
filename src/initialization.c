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
 * This file contains the subroutines that perform various initialization 
 * functions when xdd is started.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* The arguments pass in are the same argc and argv from main();
 */
int32_t
xdd_initialization(int32_t argc,char *argv[]) {
	pclk_t tt; 


	// Initialize the Global Data Structure
	// See global_data.c
	xdd_init_globals(argv[0]);

	// Init the barrier chain before any barriers get initialized
	// See barrier.c
	xdd_init_barrier_chain();

	// Parse the input arguments 
	// See parse.c
	xgp->argc = argc; // remember the original arg count
	xgp->argv = argv; // remember the original argv 
	xdd_parse(argc,argv);

	// Init output format header
	if (xgp->global_options & GO_ENDTOEND) 
		xdd_results_format_id_add("+E2ESRTIME+E2EIOTIME+E2EPERCENTSRTIME ");

	// Optimize runtime priorities and all that 
	// See schedule.c
	xdd_schedule_options();

	// initialize the signal handlers 
	// See signals.c
	xdd_signal_init();


#if WIN32
	/* Init the ts serializer mutex to compensate for a Windows bug */
	xgp->ts_serializer_mutex_name = "ts_serializer_mutex";
	ts_serializer_init(&xgp->ts_serializer_mutex, xgp->ts_serializer_mutex_name);
#endif

	/* initialize the clocks */
	pclk_initialize(&tt);
	if (tt == PCLK_BAD) {
		fprintf(xgp->errout, "%s: ERROR: Cannot initialize the picosecond clock\n",xgp->progname);
		fflush(xgp->errout);
		xdd_destroy_all_barriers();
		return(-1);
	}
	pclk_now(&xgp->base_time);

	// Init the Global Clock 
	// See global_clock.c
	xdd_init_global_clock(&xgp->ActualLocalStartTime);

	// display configuration information about this run 
	// See info_display.c
	xdd_config_info();

	return(0);
} // End of initialization()
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
