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

#define XDD_COPYRIGHT "xdd - I/O Performance Inc., US DoE/DoD Extreme Scale Systems Center <ESSC> at Oak Ridge National Labs <ORNL> - Copyright 1992-2010\n"
#define XDD_DISCLAIMER "XDD DISCLAIMER:\n *** >>>> WARNING <<<<\n *** THIS PROGRAM CAN DESTROY DATA\n *** USE AT YOUR OWN RISK\n *** IOPERFORMANCE and/or THE AUTHORS ARE NOT LIABLE FOR\n *** >>>> ANYTHING BAD <<<<\n **** THAT HAPPENS WHEN YOU RUN THIS PROGRAM\n     ...although we will take credit for anything good that happens\n     but we are not *liable* for that either.\n\n" 

#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

/* misc */
#define ONEMEG 1024*1024
#define ON 1
#define OFF 0
#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILED 0
#define MAX_TARGETS 8192
#define MAX_TARGET_NAME_LENGTH 2048
#define XFS
#define MAXSEM 1

/* program defines */
#define COMMENT '#'
#define DIRECTIVE '@'
#ifndef SPACE
#define SPACE ' '
#endif
#define TAB '\t'

/* Parameter defaults */
#define DEFAULT_TARGET_OPTIONS 0x0000000000000000ULL
#define DEFAULT_OUTPUT "stdout"
#define DEFAULT_ERROUT "stderr"
#define DEFAULT_ID "No ID Specified"
#define DEFAULT_OP "read"
#define DEFAULT_SETUP ""
#define DEFAULT_TARGETDIR ""
#define DEFAULT_TARGET NULL
#define DEFAULT_TIMESTAMP "ts"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_REQSIZE 128
#define DEFAULT_REQINCR 32
#define DEFAULT_FLUSHWRITE 0
#define DEFAULT_PASSES 1
#define DEFAULT_PASSOFFSET 0
#define DEFAULT_PASSDELAY 0
#define DEFAULT_STARTOFFSET 0
#define DEFAULT_START_DELAY 0
#define DEFAULT_NUM_TARGETS 0
#define DEFAULT_DATA_PATTERN_FILENAME NULL
#define DEFAULT_DATA_PATTERN "\0"
#define DEFAULT_DATA_PATTERN_LENGTH 1
#define DEFAULT_DATA_PATTERN_PREFIX "\0"
#define DEFAULT_DATA_PATTERN_PREFIX_LENGTH 1
#define DEFAULT_PREALLOCATE 0
#define DEFAULT_TIME_LIMIT 0
#define DEFAULT_REPORT_THRESHOLD 0
#define DEFAULT_THROTTLE 0.0
#define DEFAULT_VARIANCE 0.0
#define DEFAULT_RANGE (1024*1024)
#define DEFAULT_SEED 72058
#define DEFAULT_INTERLEAVE 1
#define DEFAULT_PORT 2000
#define DEFAULT_E2E_PORT 5044
#define DEFAULT_QUEUEDEPTH 1
#define DEFAULT_BOUNCE 100
#define DEFAULT_NUM_SEEK_HIST_BUCKETS 100
#define DEFAULT_NUM_DIST_HIST_BUCKETS 100
#define DEFAULT_RAW_PORT 2004
#define DEFAULT_RAW_LAG 0
#define DEFAULT_MAX_PATTERN_NAME_LENGTH 32
#define DEFAULT_MAX_ERRORS_TO_PRINT 10
#define DEFAULT_XDD_RLIMIT_STACK_SIZE (8192*1024)
#define DEFAULT_OUTPUT_FORMAT_STRING "+WHAT+PASS+TARGET+QUEUE+BYTESXFERED+OPS+ELAPSEDTIMEPASS+BANDWIDTH+IOPS+LATENCY+PERCENTCPUTIME+OPTYPE+XFERSIZEBYTES "

/*
 * Note that IP addresses are stored in host byte order; they're
 * converted to network byte order prior to use for network routines.
 */
typedef struct flags_t {
	bool fl_server; /* true: runs as server; false: run as client */
	in_addr_t fl_addr; /* IP address to use */
	in_port_t fl_port; /* Port number to use */
	uint32_t fl_count; /* Number of time bounces */
	pclk_t fl_time; /* Global clock time to wait for */
} flags_t;
/* ------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------ */

#define BACKLOG SOMAXCONN /* Pending request limit for listen() */

/* Default flag values */
#define DFL_FL_SERVER	false	/* Client by default */
#define DFL_FL_PORT	2000	/* Port to use */
#define DFL_FL_COUNT	10  	/* Bounce count */
#define DFL_FL_TIME	99160##000000000000LL /* Zero means don't wait */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/* XXX *//* Windows might be limiting the range from 1024 to 5000 */
#define PORT_MAX USHRT_MAX /* Max value for a port number */

/* ------------------------------------------------------------------
 * Private Globals
 * ------------------------------------------------------------------ */
static flags_t flags = {
	DFL_FL_SERVER,
	DFL_FL_ADDR,
	DFL_FL_PORT,
	DFL_FL_COUNT,
	DFL_FL_TIME
};

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

struct xdd_globals {
/* Global variables relevant to all threads */
	uint64_t     			global_options;         /* I/O Options valid for all targets */
	volatile     int32_t 	canceled;       		/* Normally set to 0. Set to 1 by xdd_sigint when an interrupt occurs */
	char         			id_firsttime;           /* ID first time through flag */
	volatile     int32_t 	run_ring;       		/* The alarm that goes off when the total run time has been exceeded */
	volatile     int32_t 	run_complete;   		/* Set to a 1 to indicate that all passes have completed */
	volatile     int32_t 	deskew_ring;    		/* The alarm that goes off when the the first thread finishes */
	volatile     int32_t 	abort_io;       		/* abort the run due to some catastrophic failure */
	time_t       			current_time_for_this_run; /* run time for this run */
	char         			*progname;              /* Program name from argv[0] */
	int32_t      			argc;                   /* The original arg count */
	char         			**argv;                 /* The original *argv[]  */
	int32_t      			passes;                 /* number of passes to perform */
	int32_t      			pass_delay;             /* number of seconds to delay between passes */
	double 	     			target_start_delay;     /* number of seconds to delay the start of each target */
	int64_t      			max_errors;             /* max number of errors to tollerate */
	int32_t      			max_errors_to_print;    /* Maximum number of compare errors to print */
	char         			*output_filename;       /* name of the output file */
	char         			*errout_filename;       /* name fo the error output file */
	char         			*csvoutput_filename;    /* name of the csv output file */
	char         			*combined_output_filename; /* name of the combined output file */
	char         			*tsbinary_filename;     /* timestamp filename prefix */
	char         			*tsoutput_filename;     /* timestamp report output filename prefix */
	FILE         			*output;                /* Output file pointer*/ 
	FILE         			*errout;                /* Error Output file pointer*/ 
	FILE         			*csvoutput;             /* Comma Separated Values output file */
	FILE         			*combined_output;       /* Combined output file */
	uint32_t     			heartbeat;              /* seconds between heartbeats */
	int32_t      			syncio;                 /* the number of I/Os to perform btw syncs */
	uint64_t     			target_offset;          /* offset value */
	int32_t      			number_of_targets;      /* number of targets to operate on */
	int32_t      			number_of_iothreads;    /* number of threads spawned for all targets */
	char         			*id;                    /* ID string pointer */
	int32_t      			runtime;                /* Length of time to run all targets, all passes */
	pclk_t       			estimated_end_time;     /* The time at which this run (all passes) should end */
	int32_t      			number_of_processors;   /* Number of processors */
	char         			random_init_state[256]; /* Random number generator state initalizer array */ 
	int          			random_initialized;     /* Random number generator has been initialized */
	int						clock_tick;				/* Number of clock ticks per second */
/* information needed to access the Global Time Server */
	in_addr_t    gts_addr;               /* Clock Server IP address */
	in_port_t    gts_port;               /* Clock Server Port number */
	pclk_t       gts_time;               /* global time on which to sync */
	pclk_t       gts_seconds_before_starting; /* number of seconds before starting I/O */
	int32_t      gts_bounce;             /* number of times to bounce the time off the global time server */
	pclk_t       gts_delta;              /* Time difference returned by the clock initializer */
	char         *gts_hostname;          /* name of the time server */
	pclk_t       ActualLocalStartTime;   /* The time to start operations */
/* Thread Barriers */
	pthread_mutex_t  xdd_init_barrier_mutex; /* locking mutex for the xdd_init_barrier() routine */
	xdd_barrier_t    *barrier_chain;         /* First barrier on the chain */
	int32_t          barrier_count;          /* Number of barriers on the chain */
	xdd_barrier_t    thread_barrier[2];		/* barriers for synchronization */
	xdd_barrier_t    syncio_barrier[2];      /* barriers for syncio */
	xdd_barrier_t    serializer_barrier[2];  /* barriers for serialization of pthread_create() */
	xdd_barrier_t    cleanup_barrier;        /* barrier for cleanup sync */
	xdd_barrier_t    final_barrier;          /* barrier for all thereads to sync with xddmain */
	xdd_barrier_t    results_pass_barrier[2];    /* barrier for all thereads to sync with the results manager at the end of a pass */
	xdd_barrier_t    results_display_barrier[2]; /* barrier for all thereads to sync with the results manager while it displays information */
	xdd_barrier_t    results_run_barrier;     /* barrier for all thereads to sync with the results manager at the end of the run */
	xdd_barrier_t    results_display_final_barrier; /* barrier for all thereads to sync with the results manager while it displays run information */
	char			*format_string;			// Pointer to the format string used by the results_display() to display results 
	char			results_header_displayed;	// 1 means that the header has been displayed, 0 means no
	int32_t			heartbeat_holdoff;		/* set to 1 by the results_manager when it wants to display pass results, 
											 * set back to 0 after everything is displayed
											 * set back to 2 to tell heartbeat to exit */
#ifdef WIN32
	HANDLE       ts_serializer_mutex;        /* needed to circumvent a Windows bug */
	char         *ts_serializer_mutex_name;  /* needed to circumvent a Windows bug */
#endif

	/* teporary until DESKEW results are fixed */
	double deskew_total_rates;
	double deskew_total_time;
	uint64_t deskew_total_bytes;

	/* Target Specific variables */
	ptds_t		*ptdsp[MAX_TARGETS];					/* Pointers to the active PTDSs - Per Target Data Structures */
	results_t	*target_average_resultsp[MAX_TARGETS];/* Results area for the "target" which is a composite of all its qthreads */
#if (LINUX)
	rlim_t	rlimit;			/* This is for the stack size limit */
#endif
}; // End of Definition of the xdd_globals data structure

typedef struct xdd_globals xdd_globals_t;

// NOTE that this is where the xdd_globals structure *pointer* is defined to occupy physical memory if this is xdd.c
#ifdef XDDMAIN
xdd_globals_t   *xgp;   /* pointer to the xdd globals */
#else
extern  xdd_globals_t   *xgp;
#endif

/* function prototypes */
void     xdd_alarm(void);
int32_t  xdd_atohex(unsigned char *destp, char *sourcep);
int32_t  xdd_barrier(struct xdd_barrier *bp);
unsigned char *xdd_init_io_buffers(ptds_t *p);
void     xdd_calculate_rates(results_t *results);
int      xdd_check_option(char *op);
void     xdd_combine_results(results_t *from, results_t *to);
void     xdd_config_info(void);
int32_t  xdd_cpu_count(void);
int32_t  xdd_deskew(void);
void     xdd_destroy_all_barriers(void);
void     xdd_destroy_barrier(struct xdd_barrier *bp);
void     xdd_display_system_info(void);
char     *xdd_getnexttoken(char *tp);
ptds_t   *xdd_get_ptdsp(int32_t target_number, char *op);
void     *xdd_heartbeat(void*);
int32_t  xdd_init_all_barriers(void);
int32_t  xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name);
void     xdd_init_global_clock(pclk_t *pclkp); 
in_addr_t xdd_init_global_clock_network(char *hostname);
void     xdd_init_globals(char *progname);
void     xdd_init_new_ptds(ptds_t *p, int32_t n);
void     xdd_init_ptds(ptds_t *p, int32_t n);
void     xdd_init_results(results_t *results);
void     xdd_init_seek_list(ptds_t *p);
void     xdd_init_signals(void);
int32_t  xdd_io_loop(ptds_t *p);
int32_t	 xdd_io_loop_after_io_operation(ptds_t *p);
int32_t  xdd_io_loop_after_loop(ptds_t *p);
int32_t  xdd_io_loop_before_io_operation(ptds_t *p);
int32_t  xdd_io_loop_before_loop(ptds_t *p);
int32_t  xdd_io_loop_perform_io_operation(ptds_t *p);
void     *xdd_io_thread(void *pin);
int32_t  xdd_io_thread_cleanup(ptds_t *p);
int32_t  xdd_io_thread_init(ptds_t *p);
void     xdd_e2e_gethost_error(char *str);
void     xdd_e2e_error_check(int val, char *str);
void     xdd_e2e_err(char const *fmt, ...);
int32_t  xdd_e2e_sockets_init(void);
int32_t  xdd_e2e_setup_dest_socket(ptds_t *p);
int32_t  xdd_e2e_dest_init(ptds_t *p);
int32_t  xdd_e2e_dest_wait(ptds_t *p);
int32_t  xdd_e2e_setup_src_socket(ptds_t *p);
int32_t  xdd_e2e_src_init(ptds_t *p);
int32_t  xdd_e2e_src_send_msg(ptds_t *p);
int32_t  xdd_e2e_dest_wait_UDP(ptds_t *p);
void     xdd_e2e_after_io_operation(ptds_t *p);
int32_t  xdd_e2e_before_io_operation(ptds_t *p);
void     xdd_e2e_before_io_loop(ptds_t *p);
int32_t  xdd_linux_cpu_count(void); 
int32_t  xdd_load_seek_list(ptds_t *p);
void     xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp);
void     xdd_memory_usage_info(FILE *out);
#ifdef WIN32
HANDLE   xdd_open_target(ptds_t *p);
#else
int32_t  xdd_open_target(ptds_t *p);
#endif
void     xdd_options_info(FILE *out);
void     xdd_pattern_buffer(ptds_t *p);
void     xdd_parse(int32_t argc, char *argv[]);
void     xdd_parse_args(int32_t argc,char *argv[], uint32_t flags);
int      xdd_parse_target_number(int32_t argc, char *argv[], uint32_t flags, int *target_number);
void     xdd_processor(ptds_t *p);
int32_t  xdd_process_directive(char *lp);
int32_t  xdd_process_paramfile(char *fnp);
double   xdd_random_float(void);
int      xdd_random_int(void);
void     xdd_raw_err(char const *fmt, ...);
int32_t  xdd_raw_read_wait(ptds_t *p); 
int32_t  xdd_raw_reader_init(ptds_t *p);
int32_t  xdd_raw_setup_reader_socket(ptds_t *p);
int32_t  xdd_raw_setup_writer_socket(ptds_t *p);
int32_t  xdd_raw_sockets_init(void);
int32_t  xdd_raw_writer_init(ptds_t *p);
int32_t  xdd_raw_writer_send_msg(ptds_t *p);
void 	 xdd_results_format_id_add( char *sp );
void     *xdd_results_manager(void *);
void     *xdd_results_display(results_t *rp);
void     xdd_save_seek_list(ptds_t *p);
void     xdd_schedule_options(void);
void     xdd_set_timelimit(void);
void     xdd_show_ptds(ptds_t *p);
void     xdd_system_info(FILE *out);
void     xdd_target_info(FILE *out, ptds_t *p);
void     xdd_ts_cleanup(struct tthdr *ttp);
void     xdd_ts_overhead(struct tthdr *ttp);
void     xdd_ts_reports(ptds_t *p);
void     xdd_ts_setup(ptds_t *p);
void     xdd_ts_write(ptds_t *p);
void     xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp);
void     xdd_usage(int32_t fullhelp);
int32_t  xdd_verify(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_location(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_contents(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_checksum(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_hex(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_sequence(ptds_t *p, int32_t current_op);
int32_t  xdd_verify_singlechar(ptds_t *p, int32_t current_op);
void     clk_delta(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);
void     clk_initialize(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);
 
extern int errno;
extern int h_errno; // For socket calls

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
