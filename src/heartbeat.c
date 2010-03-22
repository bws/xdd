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
/* xdd_heartbeat() 
 * Heartbeat runs as a separate thread that will periodically interogate the
 * PTDS of each running qthread and display the:
 *    - Current pass number
 *    - Ops completed or issued so far across all qthreads  
 *    - Estimated BW
 * 
 * A global variable called "heartbeat_holdoff" is used by the results_manager
 * to prevent heartbeat from displaying information while it is trying to
 * display information (heartbeat_holdoff == 1). 
 * It is also used to tell heartbeat to exit (heartbeat_holdoff == 2)
 */
static	char	activity_indicators[8] = {'|','/','-','\\','*'};
void *
xdd_heartbeat(void *junk) {
	int32_t 	i;						// If it is not obvious what "i" is then you should not be reading this
	pclk_t 		now;					// Current time
	pclk_t 		earliest_start_time;	// The earliest start time of the qthreads for a target
	pclk_t 		latest_end_time;		// The latest End Time of all qthreads
	int64_t		total_bytes_xferred;	// The total number of bytes xferred for all qthreads for a target
	int64_t		total_ops_issued;		// This is the total number of ops issued/completed up til now
	double		elapsed;				// Elapsed time for this qthread
	double 		bw;						// Bandwidth
	ptds_t 		*p;						// Pointer to the Target's PTDS
	int			activity_index;			// A number from 0 to 4 to index into the activity indicators table
	int			prior_activity_index;	// Used to save the state of the activity index 


	// Init indices to 0
	activity_index = 0; 
	prior_activity_index = 0;

	// Enter this barrier and wait for the heartbeat monitor to initialize
	xdd_barrier(&xgp->heartbeat_initialization_barrier);

	while (1) {
		sleep(xgp->heartbeat);
		if (xgp->heartbeat_holdoff == 1) 
			continue;
		if (xgp->heartbeat_holdoff == 2) 
			return(0);
		fprintf(stderr,"\r");
		fprintf(stderr,"Pass %04d Op/AvgRate ",xgp->ptdsp[0]->my_current_pass_number);
		for (i = 0; i < xgp->number_of_targets; i++) {
			total_bytes_xferred = 0;
			earliest_start_time = PCLK_MAX;
			latest_end_time = 0.0;
			total_ops_issued = 0;
			p = xgp->ptdsp[i];
			while (p) {
				// Increment the number of bytes xferred
				total_bytes_xferred += p->my_current_bytes_xfered;
				total_ops_issued += p->my_current_op;
				// Determine if this is the earliest start time
				if (earliest_start_time > p->my_pass_start_time) 
					earliest_start_time = p->my_pass_start_time;
				p = p->nextp; // Next qthread for this target
			}
			// At this point we have the total number of bytes transferred for this target
			// as well as the time the first qthread started and the most recent op number issued.
			// From that we can calculate the estimated BW for the target as a whole 

			p = xgp->ptdsp[i];
			if (p->pass_complete) {
				now = p->my_pass_end_time;
				prior_activity_index = activity_index;
				activity_index = 4;
			} else pclk_now(&now);

			elapsed = ((double)((double)now - (double)earliest_start_time)) / FLOAT_TRILLION;

			if (elapsed != 0.0) 
				bw = (total_bytes_xferred/elapsed)/FLOAT_MILLION;
			else
				bw = -1.0;

                        if (1 == xgp->heartbeat_simple)
				fprintf(stderr,"[%c] %06lld/%06.2f + ",
					activity_indicators[activity_index],
					(long long)total_ops_issued, 
					bw);
			else
				fprintf(stderr,"[%c] %06.2f%% complete",
					activity_indicators[activity_index],
					(double)total_ops_issued/p->target_ops);
				
			if (activity_index == 4)
				activity_index = prior_activity_index;

		} // End of FOR loop that processes all targets
		if (activity_index >= 3)
			activity_index = 0;
		else activity_index++;
	}
} /* end of xdd_heartbeat() */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
