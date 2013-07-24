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
/* xdd_global_initialization() - Initialize the "global_data" structure
 */
xdd_global_data_t*
xint_global_data_initialization(char* progname) {
	char errmsg[1024];


	xgp = (xdd_global_data_t *)malloc(sizeof(struct xdd_global_data));
	if (xgp == 0) {
		sprintf(errmsg,
                        "%s: Cannot allocate %d bytes of memory for global variables!\n",
                        progname, (int)sizeof(struct xdd_global_data));
		perror(errmsg);
		return(0);
	}
	memset(xgp,0,sizeof(struct xdd_global_data));

	xgp->progname = progname;
	// This is the default in order to avoid all the warning messages, overridden by selecting -maxall andor -proclock + -memlock
	xgp->output = stdout;
	xgp->errout = stderr;
	xgp->csvoutput = NULL;
	xgp->combined_output = NULL;      
	xgp->output_filename = "stdout";
	xgp->errout_filename = "stderr";
	xgp->csvoutput_filename = "";
	xgp->combined_output_filename = ""; 

	xgp->id = (char *)malloc(MAX_IDLEN); // Allocate a suitable buffer for the run ID ASCII string
	if (xgp->id == NULL) {
		sprintf(errmsg,"%s: Cannot allocate %d bytes of memory for ID string\n",xgp->progname,MAX_IDLEN);
		perror(errmsg);
	}
	if (xgp->id)
		strcpy(xgp->id,DEFAULT_ID);

	xgp->max_errors = 0; /* set to total_ops later when we know what it is */
	xgp->max_errors_to_print = DEFAULT_MAX_ERRORS_TO_PRINT;  /* max number of errors to print information about */ 
	xgp->number_of_processors = 0;   /* Number of processors */ 
	xgp->id_firsttime = 1;
	xgp->run_time_expired = 0;       /* The alarm that goes off when the total run time has been exceeded */
	xgp->run_error_count_exceeded = 0;       /* The alarm that goes off when the number of errors for this run has been exceeded */
	xgp->run_complete = 0; 
	xgp->abort = 0;       /* abort the run due to some catastrophic failure */
	xgp->canceled = 0;       /* abort the run due to some catastrophic failure */
	xgp->random_initialized = 0;     /* Random number generator has not been initialized  */
	xgp->XDDMain_Thread = pthread_self();

	return(xgp);

} /* end of xdd_init_globals() */

