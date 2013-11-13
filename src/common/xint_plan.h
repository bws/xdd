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
#ifndef XINT_PLAN_H
#define XINT_PLAN_H
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <pthread.h>
#include "access_pattern.h"
#include "barrier.h"
#include "results.h"
#include "end_to_end.h"
#include "target_offset_table.h"
#include "heartbeat.h"
#include "sgio.h"
#include "xint_triggers.h"
#include "xint_datapatterns.h"
#include "xint_extended_stats.h"
#include "xint_throttle.h"
#include "xint_common.h"
#include "xint_nclk.h"
#include "xint_task.h"
#include "xint_target_counters.h"
#include "xint_timestamp.h"
#include "xint_td.h"
#include "xint_wd.h"
#include "xint_read_after_write.h"
// REMOVE LATER struct ptds;

// Bit field definitions for the xdd_global_options - The "PLAN_XXXX" definitions are specifically for the Global Options 

#define PLAN_SYNCIO             0x0000000000000001ULL  /* Sync every nth I/O operation */
#define PLAN_NOBARRIER          0x0000000000000002ULL  /* Do not use a barrier */
#define PLAN_NOMEMLOCK          0x0000000000000004ULL  /* Do not lock memory */
#define PLAN_NOPROCLOCK         0x0000000000000008ULL  /* Do not lock process */
#define PLAN_MAXPRI             0x0000000000000010ULL  /* Maximum process priority */
#define PLAN_PLOCK              0x0000000000000020ULL  /* Lock process in memory */
#define PLAN_CSV                0x0000000000000040ULL  /* Generate Comma Separated Values (.csv) Output file */
#define PLAN_COMBINED           0x0000000000000080ULL  /* Display COMBINED output to a special file */
#define PLAN_VERBOSE            0x0000000000000100ULL  /* Verbose output */
#define PLAN_REALLYVERBOSE      0x0000000000000200ULL  /* Really verbose output */
#define PLAN_TIMER_INFO         0x0000000000000400ULL  /* Display Timer information */
#define PLAN_MEMORY_USAGE_ONLY  0x0000000000000800ULL  /* Display memory usage and exit */
#define PLAN_STOP_ON_ERROR      0x0000000000001000ULL  /* Indicates that all targets/threads should stop on the first error from any target */
#define PLAN_DEBUG				0x0000000000004000ULL  /* DEBUG flag used by the Write After Read routines */
#define PLAN_ENDTOEND			0x0000000000008000ULL  /* End to End operation - be sure to add the headers for the results display */
#define PLAN_EXTENDED_STATS		0x0000000000010000ULL  /* Calculate Extended stats on each operation */
#define PLAN_DRYRUN				0x0000000000020000ULL  /* Indicates a dry run - chicken! */
#define PLAN_HEARTBEAT			0x0000000000040000ULL  /* Indicates that a heartbeat has been requested */
#define PLAN_AVAILABLE2			0x0000000000080000ULL  /* AVAILABLE */
#define PLAN_INTERACTIVE		0x0000000400000000ULL  /* Enter Interactive Mode - oh what FUN! */
#define PLAN_INTERACTIVE_EXIT	0x0000000800000000ULL  /* Exit Interactive Mode */
#define PLAN_INTERACTIVE_STOP	0x0000001000000000ULL  /* Stop at various points in Interactive Mode */

struct xint_plan {
/* Global variables relevant to all threads */
	uint64_t		plan_options;         				/* I/O Options valid for all targets */
	time_t			current_time_for_this_run; 			/* run time for this run */
	int32_t			passes;                 			/* number of passes to perform */
	int32_t			current_pass_number;      			/* Current pass number at any given time */
	double			pass_delay;             			/* number of seconds to delay between passes */
	int32_t			pass_delay_usec;          			/* number of microseconds to delay between passes */
	nclk_t			pass_delay_accumulated_time;		/* number of high-res clock ticks of accumulated time during all pass delays */
	char			*ts_binary_filename_prefix; 			/* timestamp filename prefix */
	char			*ts_output_filename_prefix; 			/* timestamp report output filename prefix */
	uint32_t		restart_frequency;      			/* seconds between restart monitor checks */
	int32_t			syncio;                 			/* the number of I/Os to perform btw syncs */
	uint64_t		target_offset;          			/* offset value */
	int32_t			number_of_targets;      			/* number of targets to operate on */
	int32_t			number_of_iothreads;    			/* number of threads spawned for all targets */
	double			run_time;                			/* Length of time to run all targets, all passes */
	nclk_t			run_time_ticks;            			/* Length of time to run all targets, all passes - in high-res clock ticks*/
	nclk_t			base_time;     						/* The time that xdd was started - set during initialization */
	nclk_t			run_start_time; 					/* The time that the targets will start their first pass - set after initialization */
	nclk_t			estimated_end_time;     			/* The time at which this run (all passes) should end */
	int32_t			number_of_processors;   			/* Number of processors */

	int32_t			clock_tick;							/* Number of clock ticks per second */
// Indicators that are used to control exit conditions and the like
	int 			e2e_TCP_Win;						/* TCP Window Size - used by e2e */
	struct linger	e2e_SO_Linger;						/* Used by the SO_LINGER Socket Option - used by e2e */
	struct sigaction sa;								/* Used by the signal handlers to determine what to do */
/* information needed to access the Global Time Server */
	in_addr_t		gts_addr;               			/* Clock Server IP address */
	in_port_t		gts_port;               			/* Clock Server Port number */
	nclk_t			gts_time;               			/* global time on which to sync */
	nclk_t			gts_seconds_before_starting; 		/* number of seconds before starting I/O */
	int32_t			gts_bounce;             			/* number of times to bounce the time off the global time server */
	nclk_t			gts_delta;              			/* Time difference returned by the clock initializer */
	char			*gts_hostname;          			/* name of the time server */
	nclk_t			ActualLocalStartTime;   			/* The time to start operations */
	struct			utsname hostname;					/* The name of the computer this instance of XDD is running on */

// PThread structures for the main threads
	pthread_t 		XDDMain_Thread;						// PThread struct for the XDD main thread
	pthread_t 		Results_Thread;						// PThread struct for results manager
	pthread_t 		Heartbeat_Thread;					// PThread struct for heartbeat monitor
	pthread_t 		Restart_Thread;						// PThread struct for restart monitor
	pthread_t 		Interactive_Thread;					// PThread struct for Interactive Control processor Thread

// Thread Barriers 
	pthread_mutex_t	barrier_chain_mutex;				/* Locking mutex for the barrier chain */
	xdd_barrier_t	*barrier_chain_first;        		/* First barrier on the chain */
	xdd_barrier_t	*barrier_chain_last;        		/* Last barrier on the chain */
	int32_t			barrier_count;         				/* Number of barriers on the chain */

	xdd_barrier_t	main_general_init_barrier;			// Barrier for xdd_main to make sure that the various support threads have initialized 

	xdd_barrier_t	main_targets_waitforstart_barrier;	// Barrier where all target threads go waiting for main to tell them to start

	xdd_barrier_t	main_targets_syncio_barrier;    	// Barrier for syncio 

	xdd_barrier_t	main_results_final_barrier;        	// Barrier for the Results Manager to sync with xdd_main after all Target Threads have terminated

	xdd_barrier_t	results_targets_startpass_barrier;	// Barrier for synchronizing target threads - all targets gather here at the beginning of a pass 

	xdd_barrier_t	results_targets_endpass_barrier;	// Barrier for synchronizing target threads - all targets gather here at the end of a pass 

	xdd_barrier_t	results_targets_display_barrier;	// Barrier for all Target Thereads to sync with the Results Manager while it displays information 

	xdd_barrier_t	results_targets_run_barrier;     	// Barrier for all Target Thereads to sync with the results manager at the end of the run 

	xdd_barrier_t	results_targets_waitforcleanup_barrier; // Barrier for all thereads to sync with the results manager when they have completed cleanup

	xdd_barrier_t	interactive_barrier;				// Barriers for interactive flow control
	xdd_occupant_t	interactive_occupant;				// Occupant structure for the Interactive functions



// Variables used by the Results Manager
	char			*format_string;						// Pointer to the format string used by the results_display() to display results 
	char			results_header_displayed;			// 1 means that the header has been displayed, 0 means no
	uint32_t		heartbeat_flags;					// Tell the heartbeat thread what to do and when
#define	HEARTBEAT_ACTIVE	0x00000001					// The HEARTBEAT_ACTIVE is set by the heartbeat thread when it is running
#define HEARTBEAT_HOLDOFF	0x00000002					// The results_manager will set HEARTBEAT_HOLDOFF bit when it wants to display pass results, 
											 			// and unset HEARTBEAT_HOLDOFF after everything is displayed
#define HEARTBEAT_EXIT		0x00000004		 			// The results_manager will set HEARTBEAT_EXIT to tell heartbeat to exit 
														//
#ifdef WIN32
	HANDLE			ts_serializer_mutex;        		/* needed to circumvent a Windows bug */
	char			*ts_serializer_mutex_name;  		/* needed to circumvent a Windows bug */
#endif

	/* Target Specific variables */
	target_data_t	*target_datap[MAX_TARGETS];			/* Pointers to the active Target Data Structs */
	results_t		*target_average_resultsp[MAX_TARGETS];/* Results area for the "target" which is a composite of all its worker threads */
	int64_t			target_errno[MAX_TARGETS];			// Is set by each target to indicate its final return code

#ifdef LINUX
	rlim_t	rlimit;
#endif

}; // End of Definition of the xdd_globals data structure
typedef	struct 		xint_plan 	xdd_plan_t;

xdd_plan_t* xint_plan_data_initialization();

int xint_plan_start(xdd_plan_t* planp, xdd_occupant_t* occupant);
	
int xint_plan_start_targets(xdd_plan_t* planp);

void xint_plan_start_results_manager(xdd_plan_t* planp);

void xint_plan_start_heartbeat(xdd_plan_t* planp);

void xint_plan_start_restart_monitor(xdd_plan_t* planp);

void xint_plan_start_interactive(xdd_plan_t* planp);
	
#endif
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
