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
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

// The following structure is used to implement the lockstep options 

struct lockstep {
	pthread_mutex_t ls_mutex;  				// This is the lock-step mutex used by this target 
	int32_t       ls_task_counter; 			// This is the number of times that the master has requested this 
					  						// slave to perform a task. 
					  						// Each time the master needs this slave to perform a task this counter is
					  						// incremented by 1.
					  						// Each time this slave completes a task, this counter is decremented by 1.
					  						// Access to this counter is protected by the ls_mutex. 
#define LS_INTERVAL_TIME  	0x00000001 		// Task type of "time" 
#define LS_INTERVAL_OP		0x00000002 		// Task type of "op" 
#define LS_INTERVAL_PERCENT	0x00000004 		// Task type of "percent" 
#define LS_INTERVAL_BYTES	0x00000008 		// Task type of "bytes" 
	uint32_t		ls_interval_type; 		// Flags used by the lock-step master 
	char			*ls_interval_units; 	// ASCII readable units for the interval value 
	int64_t			ls_interval_value; 		// This is the value of the interval on which the lock step occurs 
	uint64_t		ls_interval_base_value;	// This is the base value to which the interval value is compared to 
#define LS_TASK_TIME		0x00000001 		// Task type of "time" 
#define LS_TASK_OP			0x00000002 		// Task type of "op" 
#define LS_TASK_PERCENT		0x00000004 		// Task type of "percent" 
#define LS_TASK_BYTES		0x00000008 		// Task type of "bytes" 
	uint32_t		ls_task_type; 			// Flags used by the lock-step master 
	char			*ls_task_units; 		// ASCII readable units for the task value 
	int64_t			ls_task_value; 			// Depending on the task type (time, ops, or bytes) this variable will be
									 		// set to the appropriate number of seconds, ops, or bytes to run/execute/transfer
									 		// per "task".
	uint64_t		ls_task_base_value;		// This is the base value to which the task value is compared to 
#define LS_SLAVE_WAITING	0x00000001 		// The slave is waiting for the master to enter the ls_barrier 
#define LS_SLAVE_RUN_IMMEDIATELY 0x00000002	// The slave should start running immediately 
#define LS_SLAVE_COMPLETE	0x00000004 		// The slave should complete all operations after this I/O 
#define LS_SLAVE_STOP		0x00000008 		// The slave should abort after this I/O 
#define LS_SLAVE_FINISHED	0x00000010 		// The slave is finished 
#define LS_MASTER_FINISHED	0x00000020 		// The master has completed its pass 
#define LS_MASTER_WAITING	0x00000040 		// The master is waiting at the barrier 
	uint32_t		ls_ms_state;			// This is the state of the master and slave at any given time. 
											// If this is set to SLAVE_WAITING
									 		// then the slave has entered the ls_barrier and is waiting for the master to enter
									 		// so that it can continue. This is checked by the master so that it will enter only
									 		// if the slave is there waiting. This prevents the master from being blocked when
									 		// doing overlapped-lock-step operations.
	int32_t			ls_master;  			// The target number that is the master if this target is a slave 
	int32_t			ls_slave;  				// The target number that is the slave if this target is the master 
	int32_t			ls_slave_loop_counter;	// Keeps track of the number of times through the I/O loop *
	xdd_barrier_t 	Lock_Step_Barrier[2]; 	// Lock Step Barrier for non-overlapped lock stepping 
	int32_t			Lock_Step_Barrier_Master_Index; // The index for the Lock Step Barrier 
	int32_t			Lock_Step_Barrier_Slave_Index; // The index for the Lock Step Barrier 
    // ------------------ End of the LockStep stuff --------------------------------------------------
};
typedef struct lockstep lockstep_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
