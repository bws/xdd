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
#ifndef XINT_GLOBAL_DATA_H
#define XINT_GLOBAL_DATA_H

#include <sys/utsname.h>
#include <sys/socket.h>
#include <pthread.h>
#include "barrier.h"

// Bit field definitions for the xdd_global_options - The "GO_XXXX" definitions are specifically for the Global Options 

#define GO_SYNCIO              	0x0000000000000001ULL  /* Sync every nth I/O operation */
#define GO_NOBARRIER           	0x0000000000000002ULL  /* Do not use a barrier */
#define GO_NOMEMLOCK           	0x0000000000000004ULL  /* Do not lock memory */
#define GO_NOPROCLOCK          	0x0000000000000008ULL  /* Do not lock process */
#define GO_MAXPRI              	0x0000000000000010ULL  /* Maximum process priority */
#define GO_PLOCK               	0x0000000000000020ULL  /* Lock process in memory */
#define GO_CSV                 	0x0000000000000040ULL  /* Generate Comma Separated Values (.csv) Output file */
#define GO_COMBINED            	0x0000000000000080ULL  /* Display COMBINED output to a special file */
#define GO_VERBOSE             	0x0000000000000100ULL  /* Verbose output */
#define GO_REALLYVERBOSE       	0x0000000000000200ULL  /* Really verbose output */
#define GO_TIMER_INFO          	0x0000000000000400ULL  /* Display Timer information */
#define GO_MEMORY_USAGE_ONLY   	0x0000000000000800ULL  /* Display memory usage and exit */
#define GO_STOP_ON_ERROR       	0x0000000000001000ULL  /* Indicates that all targets/threads should stop on the first error from any target */
#define GO_ENDTOEND				0x0000000000008000ULL  /* End to End operation - be sure to add the headers for the results display */
#define GO_EXTENDED_STATS		0x0000000000010000ULL  /* Calculate Extended stats on each operation */
#define GO_DRYRUN				0x0000000000020000ULL  /* Indicates a dry run - chicken! */
#define GO_HEARTBEAT			0x0000000000040000ULL  /* Indicates that a heartbeat has been requested */
#define GO_INTERACTIVE			0x0000000400000000ULL  /* Enter Interactive Mode - oh what FUN! */
#define GO_INTERACTIVE_EXIT		0x0000000800000000ULL  /* Exit Interactive Mode */
#define GO_INTERACTIVE_STOP		0x0000001000000000ULL  /* Stop at various points in Interactive Mode */
#define GO_LOCKSTEP				0x0000002000000000ULL  /* Indicates that the lockstep MASTER has been defined */

#define GO_DEBUG_IO				0x0010000000000000ULL  /* */
#define GO_DEBUG_E2E			0x0020000000000000ULL  /* */
#define GO_DEBUG_LOCKSTEP		0x0040000000000000ULL  /* */
#define GO_DEBUG_OPEN			0x0080000000000000ULL  /* */
#define GO_DEBUG_TASK			0x0100000000000000ULL  /* */
#define GO_DEBUG_TOT			0x0200000000000000ULL  /* */
#define GO_DEBUG_TS				0x0400000000000000ULL  /* */
#define GO_DEBUG_THROTTLE		0x0800000000000000ULL  /* */
#define GO_DEBUG_TIME_LIMIT		0x1000000000000000ULL  /* */
#define GO_DEBUG_USER1			0x2000000000000000ULL  /* */
#define GO_DEBUG_RESULTS		0x4000000000000000ULL  /* */
#define GO_DEBUG_ALL			(GO_DEBUG_IO|GO_DEBUG_E2E|GO_DEBUG_LOCKSTEP|GO_DEBUG_OPEN|GO_DEBUG_TASK|GO_DEBUG_TOT|GO_DEBUG_TS|GO_DEBUG_THROTTLE|GO_DEBUG_USER1|GO_DEBUG_RESULTS)

struct xdd_global_data {
	uint64_t		global_options;        				/* I/O Options valid for all targets */
	char			*progname;              			/* Program name from argv[0] */
    	int32_t			argc;                   			/* The original arg count */
	char			**argv;                 			/* The original *argv[]  */
	FILE			*output;                			/* Output file pointer*/ 
	FILE			*errout;                			/* Error Output file pointer*/ 
	FILE			*csvoutput;             			/* Comma Separated Values output file */
	FILE			*combined_output;       			/* Combined output file */
	char			*output_filename;       			/* name of the output file */
	char			*errout_filename;       			/* name fo the error output file */
	char			*csvoutput_filename;    			/* name of the csv output file */
	char			*combined_output_filename; 			/* name of the combined output file */
	char			*id;                    			/* ID string pointer */
	uint64_t			max_errors;             			/* max number of errors to tollerate */
	uint64_t			max_errors_to_print;    			/* Maximum number of compare errors to print */
	uint32_t			number_of_processors;   			/* Number of processors */    
	int32_t			clock_tick;							/* Number of clock ticks per second */
	nclk_t			debug_base_time;					/* The "t=0" time used by DEBUG operation time stamps */
    
// Indicators that are used to control exit conditions and the like
	char			id_firsttime;           			/* ID first time through flag */
	char			run_time_expired;  					/* The alarm that goes off when the total run time has been exceeded */
	char			run_error_count_exceeded; 			/* The alarm that goes off when the number of errors has been exceeded */
	char			run_complete;   					/* Set to a 1 to indicate that all passes have completed */
	char			abort;       						/* Abort the run due to some catastrophic failure */
	char			canceled;       					/* Program canceled by user */
	char                    random_initialized;                             /* Random number generator has been initialized */    
	char                    random_init_state[256];                         /* Random number generator state initalizer array */
	unsigned int            random_init_seed;                               /* Random number generator state seed value */
	struct sigaction sa;								/* Used by the signal handlers to determine what to do */
    
// PThread structures for the main threads
	pthread_t 		XDDMain_Thread;						// PThread struct for the XDD main thread
	pthread_t 		Results_Thread;						// PThread struct for results manager
	pthread_t 		Heartbeat_Thread;					// PThread struct for heartbeat monitor
	pthread_t 		Restart_Thread;						// PThread struct for restart monitor
	pthread_t 		Interactive_Thread;					// PThread struct for Interactive Control processor Thread

}; // End of Definition of the xdd_globals data structure

// Return Values
#define	XDD_RETURN_VALUE_SUCCESS				0		// Successful execution
#define	XDD_RETURN_VALUE_INIT_FAILURE			1		// Something bad happened during initialization
#define	XDD_RETURN_VALUE_INVALID_ARGUMENT		2		// An invalid argument was specified as part of a valid option
#define	XDD_RETURN_VALUE_INVALID_OPTION			3		// An invalid option was specified
#define	XDD_RETURN_VALUE_TARGET_START_FAILURE	4		// Could not start one or more targets
#define	XDD_RETURN_VALUE_CANCELED				5		// Run was canceled
#define	XDD_RETURN_VALUE_IOERROR				6		// Run ended due to an I/O error

typedef	struct 		xdd_global_data 	xdd_global_data_t;

// NOTE that this is where the xdd_globals structure *pointer* is defined to occupy physical memory if this is xdd.c
#ifdef XDDMAIN
xdd_global_data_t   		*xgp;   					// pointer to the xdd global data that xdd_main uses

// Inject a symbol into the namespace is xfs support is configured
#ifdef HAVE_ENABLE_XFS
int xgp_xfs_enabled;
#endif

#else
extern  xdd_global_data_t   *xgp;						// pointer to the xdd global data that all routines use 
#endif

#endif
