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
#define GO_DESKEW              	0x0000000000002000ULL  /* Display memory usage and exit */
#define GO_DEBUG				0x0000000000004000ULL  /* DEBUG flag used by the Write After Read routines */
#define GO_ENDTOEND				0x0000000000008000ULL  /* End to End operation - be sure to add the headers for the results display */
#define GO_EXTENDED_STATS		0x0000000000010000ULL  /* Calculate Extended stats on each operation */
#define GO_DRYRUN				0x0000000000020000ULL  /* Indicates a dry run - chicken! */
#define GO_HEARTBEAT			0x0000000000040000ULL  /* Indicates that a heartbeat has been requested */
#define GO_AVAILABLE2			0x0000000000080000ULL  /* AVAILABLE */
#define GO_INTERACTIVE			0x0000000400000000ULL  /* Enter Interactive Mode - oh what FUN! */
#define GO_INTERACTIVE_EXIT		0x0000000800000000ULL  /* Exit Interactive Mode */
#define GO_INTERACTIVE_STOP		0x0000001000000000ULL  /* Stop at various points in Interactive Mode */

struct xdd_global_data {
/* Global variables relevant to all threads */
	uint64_t		global_options;         			/* I/O Options valid for all targets */
	time_t			current_time_for_this_run; 			/* run time for this run */
	char			*progname;              			/* Program name from argv[0] */
	int32_t			argc;                   			/* The original arg count */
	char			**argv;                 			/* The original *argv[]  */
	int32_t			passes;                 			/* number of passes to perform */
	int32_t			current_pass_number;      			/* Current pass number at any given time */
	double			pass_delay;             			/* number of seconds to delay between passes */
	int32_t			pass_delay_usec;          			/* number of microseconds to delay between passes */
	nclk_t			pass_delay_accumulated_time;		/* number of high-res clock ticks of accumulated time during all pass delays */
	int64_t			max_errors;             			/* max number of errors to tollerate */
	int64_t			max_errors_to_print;    			/* Maximum number of compare errors to print */
	char			*output_filename;       			/* name of the output file */
	char			*errout_filename;       			/* name fo the error output file */
	char			*csvoutput_filename;    			/* name of the csv output file */
	char			*combined_output_filename; 			/* name of the combined output file */
	char			*ts_binary_filename_prefix; 			/* timestamp filename prefix */
	char			*ts_output_filename_prefix; 			/* timestamp report output filename prefix */
	FILE			*output;                			/* Output file pointer*/ 
	FILE			*errout;                			/* Error Output file pointer*/ 
	FILE			*csvoutput;             			/* Comma Separated Values output file */
	FILE			*combined_output;       			/* Combined output file */
	uint32_t		restart_frequency;      			/* seconds between restart monitor checks */
	int32_t			syncio;                 			/* the number of I/Os to perform btw syncs */
	uint64_t		target_offset;          			/* offset value */
	int32_t			number_of_targets;      			/* number of targets to operate on */
	int32_t			number_of_iothreads;    			/* number of threads spawned for all targets */
	char			*id;                    			/* ID string pointer */
	double			run_time;                			/* Length of time to run all targets, all passes */
	nclk_t			run_time_ticks;            			/* Length of time to run all targets, all passes - in high-res clock ticks*/
	nclk_t			base_time;     						/* The time that xdd was started - set during initialization */
	nclk_t			run_start_time; 					/* The time that the targets will start their first pass - set after initialization */
	nclk_t			estimated_end_time;     			/* The time at which this run (all passes) should end */
	int32_t			number_of_processors;   			/* Number of processors */
	int32_t			clock_tick;							/* Number of clock ticks per second */
// Indicators that are used to control exit conditions and the like
	char			id_firsttime;           			/* ID first time through flag */
	char			run_time_expired;  					/* The alarm that goes off when the total run time has been exceeded */
	char			run_error_count_exceeded; 			/* The alarm that goes off when the number of errors has been exceeded */
	char			run_complete;   					/* Set to a 1 to indicate that all passes have completed */
	char			abort;       						/* Abort the run due to some catastrophic failure */
	char			canceled;       					/* Program canceled by user */
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
	pthread_mutex_t	xdd_init_barrier_mutex;				/* Locking mutex for the xdd_init_barrier() routine */

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
	int32_t			heartbeat_holdoff;					/* set to 1 by the results_manager when it wants to display pass results, 
											 			 * set back to 0 after everything is displayed
											 			 * set back to 2 to tell heartbeat to exit 
														 */
#ifdef WIN32
	HANDLE			ts_serializer_mutex;        		/* needed to circumvent a Windows bug */
	char			*ts_serializer_mutex_name;  		/* needed to circumvent a Windows bug */
#endif

	/* Target Specific variables */
	ptds_t			*ptdsp[MAX_TARGETS];				/* Pointers to the active PTDSs - Per Target Data Structures */
	results_t		*target_average_resultsp[MAX_TARGETS];/* Results area for the "target" which is a composite of all its qthreads */

#ifdef LINUX
	rlim_t	rlimit;
#endif

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
#else
extern  xdd_global_data_t   *xgp;						// pointer to the xdd global data that all routines use 
#endif
