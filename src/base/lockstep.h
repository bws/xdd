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
struct lockstep	{
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
	uint64_t		ls_op_counter;			// This is the number of ops that the master/slave has completed during an interval
	uint64_t		ls_byte_counter;		// This is the number of bytes that the master/slave has xferred during an interval
#define LS_I_AM_A_SLAVE				0x00000001 		// This target is a SLAVE to the MASTER target specified in ls_master
#define LS_I_AM_A_MASTER			0x00000002 		// This target is a MASTER to the SLAVE target specified in ls_slave
//
#define LS_SLAVE_STARTUP_WAIT		0x00000004 		// The slave is waiting for the master to enter the ls_barrier 
#define LS_SLAVE_STARTUP_RUN 		0x00000008		// The slave should start running immediately 
//
#define LS_SLAVE_COMPLETION_COMPLETE 0x00000010 		// The slave should complete all operations after this I/O 
#define LS_SLAVE_COMPLETION_STOP	0x00000020 		// The slave should abort after this I/O 
//
#define LS_SLAVE_FINISHED			0x00000040 		// The slave is finished 
#define LS_MASTER_PASS_COMPLETED	0x00000080 		// The master has completed its pass 
#define LS_SLAVE_WAITING			0x00000100 		// The slave is waiting at the barrier 
#define LS_MASTER_WAITING			0x00000200 		// The master is waiting at the barrier 
#define LS_SLAVE_TIME_TO_WAKEUP_MASTER			0x00000400 		// The master is waiting at the barrier 
	uint32_t		ls_ms_state;			// This is the state of the master and slave at any given time. 
											// If this is set to SLAVE_WAITING
									 		// then the slave has entered the ls_barrier and is waiting for the master to enter
									 		// so that it can continue. This is checked by the master so that it will enter only
									 		// if the slave is there waiting. This prevents the master from being blocked when
									 		// doing overlapped-lock-step operations.
	int32_t			ls_ms_target;	 		// The target number of the SLAVE or MASTER
	ptds_t			*ls_master_ptdsp;  		// My MASTER's PTDS 
	ptds_t			*ls_slave_ptdsp;		// My SLAVE's PTDS
	xdd_barrier_t 	Lock_Step_Barrier;	 	// The Lock Step Barrier for synchronous lockstep
};
typedef struct lockstep lockstep_t;

// Lockstep function prototypes
//int32_t xdd_lockstep_init(ptds_t *p);
//int32_t xdd_lockstep_before_io_operation(ptds_t *p);
//void	xdd_lockstep_before_io_loop(ptds_t *p);
//int32_t xdd_lockstep_slave_before_io_op(ptds_t *p);
//int32_t xdd_lockstep_after_io_loop(ptds_t *p);
//int32_t xdd_lockstep_after_io_op(ptds_t *p);
//int32_t xdd_lockstep_master_after_io_op(ptds_t *p);
//int32_t xdd_lockstep_master_before_io_op(ptds_t *p);
//int32_t xdd_lockstep_after_pass(ptds_t *p);
//
//void    xdd_lockstep_before_pass(ptds_t *p);





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
