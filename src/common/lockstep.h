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
	uint64_t		ls_ops_completed_this_interval;			// This is the number of ops that the target has completed in this interval
	uint64_t		ls_ops_completed_this_pass;			// This is the number of ops that the target has completed in this pass
	uint64_t		ls_bytes_scheduled;			// This is the number of bytes that the target has scheduled but not xferred
	uint64_t		ls_bytes_completed;			// This is the number of bytes that the target has xferred during an interval
	uint64_t  	    ls_task_counter; 			// This is the number of times that a target has performed a task
#define LS_STATE_INITIALIZED	0x00000001 		// This target is the FIRST target to start I/O Operations
#define LS_STATE_I_AM_THE_FIRST	0x00000020 		// This target is the FIRST target to start I/O Operations
#define LS_STATE_PASS_COMPLETE	0x00000400		// The target has completed its pass 
#define LS_STATE_START_RUN		0x00008000		// The target will start running immediately
#define LS_STATE_START_WAIT		0x00010000		// The target will wait to be released before running
#define LS_STATE_END_COMPLETE	0x00200000		// The target will complete all I/O operations regardless of when the other targets end
#define LS_STATE_END_STOP		0x04000000		// The target will stop immediately when any other start stops
	uint32_t		ls_state;					// This is the state of this target at any given time. 
	target_data_t	*ls_next_tdp;				// The Target_Data of the next target to start when this target is finished
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
