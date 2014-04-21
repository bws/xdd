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
#define TRIGGER_STARTTIME    0x00000001				// Trigger type of "time" 
#define TRIGGER_STARTOP      0x00000002				// Trigger type of "op" 
#define TRIGGER_STARTPERCENT 0x00000004				// Trigger type of "percent" 
#define TRIGGER_STARTBYTES   0x00000008				// Trigger type of "bytes" 
struct	xint_triggers	{
    // -------------------------------------------------------------------
	// The following variables are used to implement the various trigger options 
	nclk_t        		start_trigger_time; 		// Time to trigger another target to start 
	nclk_t        		stop_trigger_time; 			// Time to trigger another target to stop 
	uint64_t       		start_trigger_op; 			// Operation number to trigger another target to start 
	uint64_t       		stop_trigger_op; 			// Operation number  to trigger another target to stop
	double        		start_trigger_percent; 		// Percentage of ops before triggering another target to start 
	double        		stop_trigger_percent; 		// Percentage of ops before triggering another target to stop 
	uint64_t       		start_trigger_bytes; 		// Number of bytes to transfer before triggering another target to start 
	uint64_t       		stop_trigger_bytes; 		// Number of bytes to transfer before triggering another target to stop 
	uint32_t			trigger_types;				// This is the type of trigger to administer to another target 
	int32_t				start_trigger_target;		// The number of the target to send the start trigger to 
	int32_t				stop_trigger_target;		// The number of the target to send the stop trigger to 
	char				run_status;					// 0= this thread is not running, 1=running
	xdd_barrier_t		target_target_starttrigger_barrier;	// Start Trigger Barrier 
};
typedef struct xint_triggers xint_triggers_t;

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
