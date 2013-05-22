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
// -------------------------------------------------------------------
// The following variables are used to implement the lockstep options 
// Access to this structure is protected by the ls_mutex. 
struct lockstep	{
	pthread_mutex_t ls_mutex;  					// This is the lock-step mutex used by this target 
#define LS_INTERVAL_TIME  		0x00000001 		// Task type of "time" 
#define LS_INTERVAL_OP			0x00000002 		// Task type of "op" 
#define LS_INTERVAL_PERCENT		0x00000004 		// Task type of "percent" 
#define LS_INTERVAL_BYTES		0x00000008 		// Task type of "bytes" 
	uint32_t		ls_interval_type; 			// Flags used by the lock-step targets 
	char			*ls_interval_units; 		// ASCII readable units for the interval value 
	uint64_t		ls_interval_value; 			// This is the value of the interval on which the lock step occurs 
	uint64_t		ls_ops_scheduled;			// This is the number of ops that the target has scheduled but not completed
	uint64_t		ls_ops_completed_this_interval;			// This is the number of ops that the target has completed in this interval
	uint64_t		ls_ops_completed_this_pass;			// This is the number of ops that the target has completed in this pass
	uint64_t		ls_bytes_scheduled;			// This is the number of bytes that the target has scheduled but not xferred
	uint64_t		ls_bytes_completed;			// This is the number of bytes that the target has xferred during an interval
	uint64_t  	    ls_task_counter; 			// This is the number of times that a target has performed a task
#define LS_STATE_INITIALIZED	0x00000001 		// This target has been lockstep-initialized
#define LS_STATE_I_AM_THE_FIRST	0x00000020 		// This target is the FIRST target to start I/O Operations
#define LS_STATE_WAIT			0x00000400 		// This target is waiting to be released
#define LS_STATE_RELEASE_TARGET	0x00008000 		// The next target needs to be released
#define LS_STATE_PASS_COMPLETE	0x00010000 		// The target has completed its pass 
#define LS_STATE_IGNORE_LOCKSTEP	0x00200000 		// Suspend Lockstep operations 
#define LS_STATE_TARGET_ALREADY_RELEASED	0x00400000 		// This target has already been released by a different QThread
	uint32_t		ls_state;					// This is the state of this target at any given time. 
	ptds_t			*ls_next_ptdsp;				// The PTDS of the next target to start when this target is finished
	xdd_barrier_t 	Lock_Step_Barrier;	 		// The Lock Step Barrier where targets wait 
};
typedef struct lockstep lockstep_t;

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
