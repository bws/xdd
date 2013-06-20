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
#include "xint_target_state.h"

// Bit settings that are used in the Target Options (TO_XXXXX bit definitions) 64-bit word in the Target_Data Struct
#define TO_READAFTERWRITE              0x0000000000000001ULL  // Read-After-Write - the -raw option 
#define TO_RAW_READER                  0x0000000000000002ULL  // Read-After-Write - reader 
#define TO_RAW_WRITER                  0x0000000000000004ULL  // Read-After-Write - writer 
#define TO_ENDTOEND                    0x0000000000000008ULL  // End to End - aka -e2e option 
#define TO_E2E_SOURCE                  0x0000000000000010ULL  // End to End - Source side 
#define TO_E2E_DESTINATION             0x0000000000000020ULL  // End to End - Destination side 
#define TO_SGIO                        0x0000000000000040ULL  // Used for SCSI Generic I/O in Linux 
#define TO_DIO                         0x0000000000000080ULL  // DIRECT IO for files 
#define TO_DELETEFILE                  0x0000000000000200ULL  // Delete target file upon completion of write 
#define TO_REGULARFILE                 0x0000000000000400ULL  // Target file is a REGULAR file 
#define TO_DEVICEFILE                  0x0000000000000800ULL  // Target is a Device - could be an SG device 
#define TO_SOCKET                      0x0000000000001000ULL  // Target is a Socket kind of file 
#define TO_WAITFORSTART                0x0000000000002000ULL  // Wait for START trigger 
#define TO_LOCKSTEP                    0x0000000000004000ULL  // Normal Lock step mode 
#define TO_LOCKSTEPOVERLAPPED          0x0000000000008000ULL  // Overlapped lock step mode 
#define TO_SHARED_MEMORY               0x0000000000010000ULL  // Use a shared memory segment instead of malloced memmory 
#define TO_PCPU_ABSOLUTE               0x0000000080000000ULL  // Defines the meaning of the percent CPU values on the output 
#define TO_REOPEN                      0x0000000100000000ULL  // Open/Close target on each pass and record time 
#define TO_CREATE_NEW_FILES            0x0000000200000000ULL  // Create new targets for each pass 
#define TO_RECREATE                    0x0000000400000000ULL  // ReCreate targets for each pass 
#define TO_SYNCWRITE                   0x0000000800000000ULL  // Sync buffered write operations 
#define TO_PASS_RANDOMIZE              0x0000001000000000ULL  // Pass randomize 
#define TO_VERIFY_CONTENTS             0x0000004000000000ULL  // Verify the contents of the I/O buffer 
#define TO_VERIFY_LOCATION             0x0000008000000000ULL  // Verify the location of the I/O buffer (first 8 bytes) 
#define TO_RESTART_ENABLE              0x0000010000000000ULL  // Restart option enabled 
#define TO_NULL_TARGET                 0x0000020000000000ULL  // Indicates that the target is infinitely fast
#define TO_E2E_SOURCE_MONITOR          0x0000040000000000ULL  // End to End - Source side monitor in target_pass_loop()
#define TO_ORDERING_STORAGE_SERIAL     0x0000080000000000ULL  // Serial Odering method applied to storage
#define TO_ORDERING_NETWORK_SERIAL     0x0000100000000000ULL  // Serial Odering method applied to network
#define TO_ORDERING_STORAGE_LOOSE      0x0000200000000000ULL  // Loose Odering method applied to storage
#define TO_ORDERING_NETWORK_LOOSE      0x0000400000000000ULL  // Loose Odering method applied to network

// Per Thread Data Structure - one for each thread 
struct xint_target_data {
    struct xdd_plan 	*td_planp;
	struct xint_worker_data		*td_next_wdp;		// Pointer to the first worker_data struct in the list
	pthread_t  			td_thread;			// Handle for this Target Thread 
	int32_t   			td_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   			td_pid;   			// My process ID 
	int32_t   			td_target_number;	// My target number 
	uint64_t			td_target_options; 			// I/O Options specific to each target 
#ifdef WIN32
	HANDLE   			td_file_desc; 		// File HANDLE for the target device/file 
#else
	int32_t   			td_file_desc;		// File Descriptor for the target device/file 
#endif
	int32_t				td_open_flags;		// Flags used during open processing of a target
	int32_t				td_iosize;   		// Number of bytes per request 
	int32_t				td_actual_iosize;  	// Number of bytes actually transferred for this request 
	int32_t				td_last_iosize; 	// Number of bytes for the final request 
	int32_t				td_op_delay; 		// Number of seconds to delay between operations 
	int32_t				td_filetype;  		// Type of file: regular, device, socket, ... 
	int64_t				td_filesize;  		// Size of target file in bytes 
	int64_t				td_worker_thread_ops;  	// Total number of ops to perform per qthread 
	int64_t				td_target_ops;  	// Total number of ops to perform on behalf of a "target"
	seekhdr_t			td_seekhdr;  		// For all the seek information 
	FILE				*td_tsfp;   		// Pointer to the time stamp output file 
	unsigned char 		*td_rwbuf;   			// The re-aligned I/O buffers 
	int32_t				td_rwbuf_shmid; 		// Shared Memeory ID for rwbuf 
	unsigned char 		*td_rwbuf_save; 		// The original I/O buffers 
	// The Occupant Strcuture used by the barriers 
	xdd_occupant_t		td_occupant;							// Used by the barriers to keep track of what is in a barrier at any given time
	char				td_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH];	// For a Target thread this is "TARGET####", for a Worker Thread it is "TARGET####WORKER####"
	xdd_barrier_t		*td_current_barrier;					// Pointer to the current barrier this Thread is in at any given time or NULL if not in a barrier

	// Target-specific variables
	xdd_barrier_t		td_target_worker_thread_init_barrier;		// Where the Target Thread waits for the Worker Thread to initialize

	xdd_barrier_t		td_targetpass_worker_thread_passcomplete_barrier;// The barrier used to sync targetpass() with all the QThreads at the end of a pass
	xdd_barrier_t		td_targetpass_worker_thread_eofcomplete_barrier;// The barrier used to sync targetpass_eof_desintation_side() with a QThread trying to recv an EOF packet

	pthread_mutex_t 	td_counter_mutex;  					// Mutex for locking when updating counters

	uint64_t			td_bytes_issued;						// The amount of data for all transfer requests that has been issued so far 
	uint64_t			td_bytes_completed;					// The amount of data for all transfer requests that has been completed so far
	uint64_t			td_bytes_remaining;					// Bytes remaining to be transferred 

    // Target-specific semaphores and associated pointers
    tot_t				*td_totp;								// Pointer to the target_offset_table for this target
    pthread_cond_t 		td_any_worker_thread_available_condition;
    pthread_mutex_t 	td_any_worker_thread_available_mutex;
    int 				td_any_worker_thread_available;

	// QThread-specific semaphores and associated pointers
	pthread_mutex_t		td_worker_thread_target_sync_mutex;			// Used to serialize access to the QThread-Target Synchronization flags
	int32_t				td_worker_thread_target_sync;				// Flags used to synchronize a QThread with its Target
#define	WTSYNC_AVAILABLE			0x00000001				// This QThread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
#define	WTSYNC_BUSY					0x00000002				// This QThread is busy
#define	WTSYNC_TARGET_WAITING		0x00000004				// The parent Target is waiting for this QThread to become available, set by xdd_get_specific_qthread, reset by qthread.
#define	WTSYNC_EOF_RECEIVED			0x00000008				// This QThread received an EOF packet from the Source Side of an E2E Operation
    //sem_t				this_qthread_is_available_sem;		// xdd_get_specific_qthread() routine waits on this for any QThread to become available
    pthread_cond_t 		td_this_wthread_is_available_condition;
    
//	xdd_barrier_t		wthread_targetpass_wait_for_task_barrier;	// The barrier where the QThread waits for targetpass() to release it with a task to perform

	// command line option values 
	int64_t				td_start_offset; 			// starting block offset value 
	int64_t				td_pass_offset; 			// number of blocks to add to seek locations between passes 
	int64_t				td_flushwrite;  			// number of write operations to perform between flushes 
	int64_t				td_flushwrite_current_count;  // Running number of write operations - used to trigger a flush (sync) operation 
	int64_t				td_bytes;   				// number of bytes to process overall 
	int64_t				td_numreqs;  				// Number of requests to perform per pass per qthread
	double				td_rwratio;  				// read/write ratios 
	nclk_t				td_report_threshold;		// reporting threshold for long operations 
	int32_t				td_reqsize;  				// number of *blocksize* byte blocks per operation for each target 
	int32_t				td_retry_count;  			// number of retries to issue on an error 
	double				td_time_limit;				// Time of a single pass in seconds
	nclk_t				td_time_limit_ticks;		// Time of a single pass in high-res clock ticks
	char				*td_target_directory; 		// The target directory for the target 
	char				*td_target_basename; 		// Basename of the target/device
	char				*td_target_full_pathname;	// Fully qualified path name to the target device/file
	char				td_target_extension[32]; 	// The target extension number 
	int32_t				td_processor;  				// Processor/target assignments 
	double				td_start_delay; 			// number of seconds to delay the start  of this operation 
	nclk_t				td_start_delay_psec;		// number of nanoseconds to delay the start  of this operation 
	char				td_random_init_state[256]; 	// Random number generator state initalizer array 
	int32_t				td_block_size;  			// Size of a block in bytes for this target 
	int32_t				td_queue_depth; 			// Command queue depth for each target 
	int64_t				td_preallocate; 			// File preallocation value 
	int32_t				td_mem_align;   			// Memory read/write buffer alignment value in bytes 
    //
    // ------------------ Heartbeat stuff --------------------------------------------------
	// The following heartbeat structure and data is for the -heartbeat option
	struct heartbeat	td_hb;					// Heartbeat data
    //
    // ------------------ RUNTIME stuff --------------------------------------------------
    // Stuff REFERENCED during runtime
	//
	nclk_t				td_run_start_time; 			// This is time t0 of this run - set by xdd_main
	nclk_t				td_first_pass_start_time; 	// Time the first pass started but before the first operation is issued
	uint64_t			td_target_bytes_to_xfer_per_pass; 	// Number of bytes to xfer per pass for the entire target (all qthreads)
	int64_t				td_last_committed_op;		// Operation number of last r/w operation relative to zero
	uint64_t			td_last_committed_location;	// Byte offset into target of last r/w operation
	int32_t				td_last_committed_length;	// Number of bytes transferred to/from last_committed_location
	//
    // Stuff UPDATED during each pass 
	int32_t				td_syncio_barrier_index; 		// Used to determine which syncio barrier to use at any given time
	int32_t				td_results_pass_barrier_index; // Where a Target thread waits for all other Target threads to complete a pass
	int32_t				td_results_display_barrier_index; // Where threads wait for the results_manager()to display results
	int32_t				td_results_run_barrier_index; 	// Where threads wait for all other threads at the completion of the run

	// The following variables are used by the "-reopen" option
	nclk_t        		td_open_start_time; 		// Time just before the open is issued for this target 
	nclk_t        		td_open_end_time; 			// Time just after the open completes for this target 

	struct xint_throttle		*td_throtp;			// Pointer to the throttle sturcture
	struct xdd_target_state		*td_tgtstp;			// Pointer to the target state struct
	struct xdd_timestamp		*td_tsp;			// Pointer to the time stamp stuff
	struct xdd_tthdr			*td_ttp;			// Pointer to the time stamp stuff
	struct xdd_e2e				*td_e2ep;			// Pointer to the e2e struct when needed
	struct xdd_extended_stats	*td_esp;			// Extended Stats Structure Pointer
	struct xdd_triggers			*td_trigp;			// Triggers Structure Pointer
	struct xdd_data_pattern		*td_dpp;			// Data Pattern Structure Pointer
	struct xdd_raw				*td_rawp;          	// RAW Data Structure Pointer
	struct lockstep				*td_lsp;			// Pointer to the lockstep structure used by the lockstep option
	struct restart				*td_restartp;		// Pointer to the restart structure used by the restart monitor
	struct ptds					*td_tdpm1;			// Target_Data minus  1 - used for report print queueing - don't ask 
#if (LINUX || DARWIN)
	struct stat					td_statbuf;			// Target File Stat buffer used by xdd_target_open()
#elif (AIX || SOLARIS)
	struct stat64				td_statbuf;			// Target File Stat buffer used by xdd_target_open()
#endif
};
typedef struct xint_target_data target_data_t;

#include "lockstep.h"
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
