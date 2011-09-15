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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
#define TRIGGER_STARTTIME    0x00000001				// Trigger type of "time" 
#define TRIGGER_STARTOP      0x00000002				// Trigger type of "op" 
#define TRIGGER_STARTPERCENT 0x00000004				// Trigger type of "percent" 
#define TRIGGER_STARTBYTES   0x00000008				// Trigger type of "bytes" 
struct	xdd_triggers	{
    // -------------------------------------------------------------------
	// The following variables are used to implement the various trigger options 
	nclk_t        		start_trigger_time; 		// Time to trigger another target to start 
	nclk_t        		stop_trigger_time; 			// Time to trigger another target to stop 
	int64_t       		start_trigger_op; 			// Operation number to trigger another target to start 
	int64_t       		stop_trigger_op; 			// Operation number  to trigger another target to stop
	double        		start_trigger_percent; 		// Percentage of ops before triggering another target to start 
	double        		stop_trigger_percent; 		// Percentage of ops before triggering another target to stop 
	int64_t       		start_trigger_bytes; 		// Number of bytes to transfer before triggering another target to start 
	int64_t       		stop_trigger_bytes; 		// Number of bytes to transfer before triggering another target to stop 
	uint32_t			trigger_types;				// This is the type of trigger to administer to another target 
	int32_t				start_trigger_target;		// The number of the target to send the start trigger to 
	int32_t				stop_trigger_target;		// The number of the target to send the stop trigger to 
	char				run_status;					// 0= this thread is not running, 1=running
	xdd_barrier_t		target_target_starttrigger_barrier;	// Start Trigger Barrier 
};
typedef struct xdd_triggers xdd_triggers_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
