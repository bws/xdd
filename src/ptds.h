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
#define HOSTNAMELENGTH 1024
#ifdef WIN32
#include "nt_unix_compat.h"
#endif
#include "results.h"
#include "access_pattern.h"
#include "timestamp.h"
#include "barrier.h"
#include "read_after_write.h"
#include "end_to_end.h"
#include "parse.h"
#include "target_offset_table.h"


// Bit settings that are used in the Target Options (TO_XXXXX bit definitions) 64-bit word in the PTDS
#define TO_READAFTERWRITE              0x0000000000000001ULL  // Read-After-Write - the -raw option 
#define TO_RAW_READER                  0x0000000000000002ULL  // Read-After-Write - reader 
#define TO_RAW_WRITER                  0x0000000000000004ULL  // Read-After-Write - writer 
#define TO_ENDTOEND                    0x0000000000000008ULL  // End to End - aka -e2e option 
#define TO_E2E_SOURCE                  0x0000000000000010ULL  // End to End - Source side 
#define TO_E2E_DESTINATION             0x0000000000000020ULL  // End to End - Destination side 
#define TO_SGIO                        0x0000000000000040ULL  // Used for SCSI Generic I/O in Linux 
#define TO_DIO                         0x0000000000000080ULL  // DIRECT IO for files 
#define TO_RANDOM_PATTERN              0x0000000000000100ULL  // Use random data pattern for write operations 
#define TO_DELETEFILE                  0x0000000000000200ULL  // Delete target file upon completion of write 
#define TO_REGULARFILE                 0x0000000000000400ULL  // Target file is a REGULAR file 
#define TO_DEVICEFILE                  0x0000000000000800ULL  // Target is a Device - could be an SG device 
#define TO_SOCKET                      0x0000000000001000ULL  // Target is a Socket kind of file 
#define TO_WAITFORSTART                0x0000000000002000ULL  // Wait for START trigger 
#define TO_LOCKSTEP                    0x0000000000004000ULL  // Normal Lock step mode 
#define TO_LOCKSTEPOVERLAPPED          0x0000000000008000ULL  // Overlapped lock step mode 
#define TO_SHARED_MEMORY               0x0000000000010000ULL  // Use a shared memory segment instead of malloced memmory 
#define TO_SEQUENCED_PATTERN           0x0000000000020000ULL  // Sequenced Data Pattern in the data buffer 
#define TO_ASCII_PATTERN               0x0000000000040000ULL  // ASCII Data Pattern in the data buffer 
#define TO_HEX_PATTERN                 0x0000000000080000ULL  // HEXIDECIMAL Data Pattern in the data buffer 
#define TO_SINGLECHAR_PATTERN          0x0000000000100000ULL  // Single character Data Pattern in the data buffer 
#define TO_FILE_PATTERN                0x0000000000200000ULL  // Name of file that contains the Data Pattern 
#define TO_REPLICATE_PATTERN           0x0000000000400000ULL  // Replicate Data Pattern throughout the data buffer 
#define TO_LFPAT_PATTERN               0x0000000000800000ULL  // Low Frequency FC/SAS pattern 
#define TO_LTPAT_PATTERN               0x0000000001000000ULL  // Low Transition FC/SAS pattern 
#define TO_CJTPAT_PATTERN              0x0000000002000000ULL  // Receiver Jitter FC/SAS pattern 
#define TO_CRPAT_PATTERN               0x0000000004000000ULL  // Random FC/SAS pattern 
#define TO_CSPAT_PATTERN               0x0000000008000000ULL  // Supply Noise FC/SAS pattern 
#define TO_PATTERN_PREFIX              0x0000000010000000ULL  // Indicates that there is a data pattern prefix 
#define TO_INVERSE_PATTERN             0x0000000020000000ULL  // Apply a 1's compliment to the data pattern 
#define TO_NAME_PATTERN                0x0000000040000000ULL  // Use the specified name at the beginning of the data pattern
#define TO_PCPU_ABSOLUTE               0x0000000080000000ULL  // Defines the meaning of the percent CPU values on the output 
#define TO_REOPEN                      0x0000000100000000ULL  // Open/Close target on each pass and record time 
#define TO_CREATE_NEW_FILES            0x0000000200000000ULL  // Create new targets for each pass 
#define TO_RECREATE                    0x0000000400000000ULL  // ReCreate targets for each pass 
#define TO_SYNCWRITE                   0x0000000800000000ULL  // Sync buffered write operations 
#define TO_PASS_RANDOMIZE              0x0000001000000000ULL  // Pass randomize 
#define TO_QTHREAD_INFO                0x0000002000000000ULL  // Display Q thread information 
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
struct ptds {
	struct ptds 		*target_ptds; 		// Pointer back to the Parent Target PTDS 
	struct ptds 		*next_qp; 			// Pointer to the next QThread PTDS in the PTDS Substructure
	pthread_t  			target_thread;		// Handle for this Target Thread 
	pthread_t  			qthread;			// Handle for this QThread 
	pthread_t  			issue_thread;		// Handle for this Target's Issue Thread 
	pthread_t  			completion_thread;	// Handle for this Target's Completion Thread 
	int32_t   			my_target_number;	// My target number 
	int32_t   			my_qthread_number;	// My queue number within this target 
	int32_t   			my_thread_number; 	// My thread number relative to the total number of threads
	int32_t   			my_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   			my_pid;   			// My process ID 
	int32_t   			total_threads; 		// Total number of threads -> target threads + QThreads 
	int32_t   			run_complete; 		// 0 = thread has not completed yet, 1= completed this run
#ifdef WIN32
	HANDLE   			fd;    				// File HANDLE for the target device/file 
#else
	int32_t   			fd;    				// File Descriptor for the target device/file 
#endif
	int32_t				target_open_flags;	// Flags used during open processing of a target
	unsigned char 		*rwbuf;   			// The re-aligned I/O buffers 
	int32_t				rwbuf_shmid; 		// Shared Memeory ID for rwbuf 
	unsigned char 		*rwbuf_save; 		// The original I/O buffers 
	tthdr_t				*ttp;   			// Pointers to the timestamp table 
	int32_t				iobuffersize; 		// Size of the I/O buffer in bytes 
	int32_t				iosize;   			// Number of bytes per request 
	int32_t				actual_iosize;   	// Number of bytes actually transferred for this request 
	int32_t				last_iosize; 		// Number of bytes for the final request 
	int32_t				op_delay; 			// Number of seconds to delay between operations 
	int32_t				filetype;  			// Type of file: regular, device, socket, ... 
	int64_t				filesize;  			// Size of target file in bytes 
	int64_t				qthread_ops;  		// Total number of ops to perform per qthread 
	int64_t				target_ops;  		// Total number of ops to perform on behalf of a "target"
	seekhdr_t			seekhdr;  			// For all the seek information 
	results_t			qthread_average_results;	// Averaged results for this qthread - accumulated over all passes
	FILE				*tsfp;   			// Pointer to the time stamp output file 
#ifdef WIN32
	HANDLE				*ts_serializer_mutex; // needed to circumvent a Windows bug 
	char				ts_serializer_mutex_name[256]; // needed to circumvent a Windows bug 
#endif
	// The Occupant Strcuture used by the barriers 
	xdd_occupant_t		occupant;							// Used by the barriers to keep track of what is in a barrier at any given time
	char				occupant_name[32];					// For a Target thread this is "TARGET####", for a QThread it is "TARGET####QTHREAD####"
	xdd_barrier_t		*current_barrier;					// Pointer to the current barrier this PTDS is in at any given time or NULL if not in a barrier

	// Target-specific variables
	xdd_barrier_t		target_qthread_init_barrier;		// Where the Target Thread waits for the QThread to initialize

	xdd_barrier_t		targetpass_qthread_passcomplete_barrier;// The barrier used to sync targetpass() with all the QThreads at the end of a pass
	xdd_barrier_t		targetpass_qthread_eofcomplete_barrier;// The barrier used to sync targetpass_eof_desintation_side() with a QThread trying to recv an EOF packet

	pthread_mutex_t 	counter_mutex;  					// Mutex for locking when updating counters

	uint64_t			bytes_issued;						// The amount of data for all transfer requests that has been issued so far 
	uint64_t			bytes_completed;					// The amount of data for all transfer requests that has been completed so far
	uint64_t			bytes_remaining;					// Bytes remaining to be transferred 

	// Target-specific semaphores and associated pointers
	tot_t				*totp;								// Pointer to the target_offset_table for this target
	sem_t				any_qthread_available_sem;			// The xdd_get_any_available_qthread() routine waits on this for any QThread to become available

	// QThread-specific semaphores and associated pointers
	pthread_mutex_t		qthread_target_sync_mutex;			// Used to serialize access to the QThread-Target Synchronization flags
	int32_t				qthread_target_sync;				// Flags used to synchronize a QThread with its Target
#define	QTSYNC_AVAILABLE			0x00000001				// This QThread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
#define	QTSYNC_BUSY					0x00000002				// This QThread is busy
#define	QTSYNC_TARGET_WAITING		0x00000004				// The parent Target is waiting for this QThread to become available, set by xdd_get_specific_qthread, reset by qthread.
#define	QTSYNC_EOF_RECEIVED			0x00000008				// This QThread received an EOF packet from the Source Side of an E2E Operation
	sem_t				this_qthread_is_available_sem;		// xdd_get_specific_qthread() routine waits on this for any QThread to become available

	xdd_barrier_t		qthread_targetpass_wait_for_task_barrier;	// The barrier where the QThread waits for targetpass() to release it with a task to perform

// The task variables
	char				task_request;						// Type of Task to perform
#define TASK_REQ_IO				0x01						// Perform an IO Operation 
#define TASK_REQ_REOPEN			0x02						// Re-Open the target device/file
#define TASK_REQ_STOP			0x03						// Stop doing work and exit
#define TASK_REQ_EOF			0x04						// Send an EOF to the Destination or Revceive an EOF from the Source
	char				task_op;							// Operation to perform
	uint32_t			task_xfer_size;						// Number of bytes to transfer
	uint64_t			task_byte_location;					// Offset into the file where this transfer starts
	uint64_t			task_e2e_sequence_number;			// Sequence number of this task when part of an End-to-End operation
	pclk_t				task_time_to_issue;					// Time to issue the I/O operation or 0 if not used


	// command line option values 
	int64_t				start_offset; 			// starting block offset value 
	int64_t				pass_offset; 			// number of blocks to add to seek locations between passes 
	int64_t				flushwrite;  			// number of write operations to perform between flushes 
	int64_t				flushwrite_current_count;  // Running number of write operations - used to trigger a flush (sync) operation 
	int64_t				bytes;   				// number of bytes to process overall 
	int64_t				numreqs;  				// Number of requests to perform per pass per qthread
	double				rwratio;  				// read/write ratios 
	pclk_t				report_threshold;		// reporting threshold for long operations 
	int32_t				reqsize;  				// number of *blocksize* byte blocks per operation for each target 
	int32_t				retry_count;  			// number of retries to issue on an error 
	double				time_limit;				// Time of a single pass in seconds
	pclk_t				time_limit_ticks;		// Time of a single pass in high-res clock ticks
	char				*target_directory; 		// The target directory for the target 
	char				*target_basename; 		// Basename of the target/device
	char				*target_full_pathname;	// Fully qualified path name to the target device/file
	char				target_extension[32]; 	// The target extension number 
	int32_t				processor;  			// Processor/target assignments 
	pclk_t				start_delay; 			// number of picoseconds to delay the start  of this operation 
    // ------------------ Throttle stuff --------------------------------------------------
	// The following "throttle_" members are for the -throttle option
	double				throttle;  				// Target Throttle assignments 
	double				throttle_variance;  	// Throttle Average Bandwidth variance 
	uint32_t      		throttle_type; 			// Target Throttle type 
#define PTDS_THROTTLE_OPS   0x00000001  		// Throttle type of OPS 
#define PTDS_THROTTLE_BW    0x00000002  		// Throttle type of Bandwidth 
#define PTDS_THROTTLE_ABW   0x00000004  		// Throttle type of Average Bandwidth 
#define PTDS_THROTTLE_DELAY 0x00000008  		// Throttle type of a constant delay or time for each op 
    //
    // ------------------ TimeStamp stuff --------------------------------------------------
	// The following "ts_" members are for the time stamping (-ts) option
	uint64_t			ts_options;  			// Time Stamping Options 
	int64_t				ts_current_entry; 		// Index into the Timestamp Table of the current entry
	int64_t				ts_size;  				// Time Stamping Size in number of entries 
	int64_t				ts_trigop;  			// Time Stamping trigger operation number 
	pclk_t				ts_trigtime; 			// Time Stamping trigger time 
	//
    // ------------------ SGIO stuff --------------------------------------------------
	// The following variables are used by the SCSI Generic I/O (SGIO) Routines
	//
#define SENSE_BUFF_LEN 64					// Number of bytes for the Sense buffer 
	uint64_t			sg_from_block;			// Starting block location for this operation 
	uint32_t			sg_blocks;				// The number of blocks to transfer 
	uint32_t			sg_blocksize;			// The size of a single block for an SG operation - generally == p->blocksize 
    unsigned char 		sg_sense[SENSE_BUFF_LEN]; // The Sense Buffer  
	uint32_t   			sg_num_sectors;			// Number of Sectors from Read Capacity command 
	uint32_t   			sg_sector_size;			// Sector Size in bytes from Read Capacity command 
	//
    // ------------------ Datapattern stuff --------------------------------------------------
	// Stuff related to the -datapattern option
	//
	unsigned char		*data_pattern; 				// Data pattern for write operations for each target - 
													//      This is the ASCII string representation of the pattern 
	unsigned char		*data_pattern_value; 		// This is the 64-bit hex value  
	size_t				data_pattern_length; 		// Length of ASCII data pattern for write operations for each target 
	unsigned char		*data_pattern_prefix; 		// Data pattern prefix which is a string of hex digits less than 8 bytes (16 nibbles) 
	size_t				data_pattern_prefix_length; // Data pattern prefix 
	unsigned char		*data_pattern_prefix_value; // This is the N-bit prefix value fully shifted to the right 
	uint64_t			data_pattern_prefix_binary; // This is the 64-bit prefix value fully shifted to the left 
	unsigned char		*data_pattern_name; 		// Data pattern name which is an ASCII string  
	int32_t				data_pattern_name_length;	// Length of the data pattern name string 
	char				*data_pattern_filename; 	// Name of a file that contains a data pattern to use 
	int64_t				compare_errors;				// Number of content/sequence compare errors from the verify() subroutines
	//
    // ------------------ RUNTIME stuff --------------------------------------------------
    // Stuff REFERENCED during runtime
	//
	pclk_t				run_start_time; 			// This is time t0 of this run - set by xdd_main
	pclk_t				first_pass_start_time; 		// Time the first pass started but before the first operation is issued
	uint64_t			target_bytes_to_xfer_per_pass; 	// Number of bytes to xfer per pass for the entire target (all qthreads)
	uint64_t			qthread_bytes_to_xfer_per_pass;	// Number of bytes to xfer per pass for this qthread
	int32_t				block_size;  				// Size of a block in bytes for this target 
	int32_t				queue_depth; 				// Command queue depth for each target 
	int64_t				preallocate; 				// File preallocation value 
	int32_t				mem_align;   				// Memory read/write buffer alignment value in bytes 
	uint64_t			target_options; 			// I/O Options specific to each target 
	int64_t				last_committed_op;		// Operation number of last r/w operation relative to zero
	uint64_t			last_committed_location;	// Byte offset into target of last r/w operation
	int32_t				last_committed_length;		// Number of bytes transferred to/from last_committed_location
	//
    // Stuff UPDATED during each pass 
	int32_t				my_current_pass_number; 	// Current pass number 
	int32_t				syncio_barrier_index; 		// Used to determine which syncio barrier to use at any given time
	int32_t				results_pass_barrier_index; // Where a Target thread waits for all other Target threads to complete a pass
	int32_t				results_display_barrier_index; // Where threads wait for the results_manager()to display results
	int32_t				results_run_barrier_index; 	// Where threads wait for all other threads at the completion of the run
	//
	// Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
	pclk_t				my_pass_start_time; 		// The time stamp that this pass started but before the first operation is issued
	pclk_t				my_pass_end_time; 			// The time stamp that this pass ended 
	struct	tms			my_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	struct	tms			my_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	struct	tms			my_current_cpu_times;		// CPU times from times()
	//
	// Updated by xdd_issue() at at the start of a Task IO request to a QThread
	int64_t				my_current_byte_location; 	// Current byte location for this I/O operation 
	int32_t				my_current_io_size; 		// Size of the I/O to be performed
	int32_t				my_error_break; 			// When set it indicates an error that will cause the xdd_issue() loop to stop
	char				*my_current_op_str; 		// Pointer to an ASCII string of the I/O operation type - "READ", "WRITE", or "NOOP"
	int32_t				my_current_op_type; 		// Current I/O operation type - OP_TYPE_READ or OP_TYPE_WRITE
#define OP_TYPE_READ	0x01						// used with my_current_op_type
#define OP_TYPE_WRITE	0x02						// used with my_current_op_type
#define OP_TYPE_NOOP	0x03						// used with my_current_op_type
#define OP_TYPE_EOF		0x04						// used with my_current_op_type - indicates End-of-File processing when present in the Time Stamp Table
	// Updated by the QThread upon completion of an I/O operation
	int64_t				target_op_number;			// The operation number for the target that this I/O represents
	int64_t				my_current_op_number;		// Current I/O operation number 
	int64_t				my_current_op_count; 		// The number of read+write operations that have completed so far
	int64_t				my_current_read_op_count;	// The number of read operations that have completed so far 
	int64_t				my_current_write_op_count;	// The number of write operations that have completed so far 
	int64_t				my_current_noop_op_count;	// The number of noops that have completed so far 
	int64_t				my_current_bytes_xfered_this_op; // Total number of bytes transferred for the most recent completed I/O operation
	int64_t				my_current_bytes_xfered;	// Total number of bytes transferred so far (to storage device, not network)
	int64_t				my_current_bytes_read;		// Total number of bytes read so far (from storage device, not network)
	int64_t				my_current_bytes_written;	// Total number of bytes written so far (to storage device, not network)
	int64_t				my_current_bytes_noop;		// Total number of bytes processed by noops so far
	int32_t				my_current_io_status; 		// I/O Status of the last I/O operation for this qthread
	int32_t				my_current_io_errno; 		// The errno associated with the status of this I/O for this thread
	int64_t				my_current_error_count;		// The number of I/O errors for this qthread
	pclk_t				my_elapsed_pass_time; 		// Rime between the start and end of this pass
	pclk_t				my_first_op_start_time;		// Time this qthread was able to issue its first operation for this pass
	pclk_t				my_current_op_start_time; 	// Start time of the current op
	pclk_t				my_current_op_end_time; 	// End time of the current op
	pclk_t				my_current_op_elapsed_time;	// Elapsed time of the current op
	pclk_t				my_current_net_start_time; 	// Start time of the current network op (e2e only)
	pclk_t				my_current_net_end_time; 	// End time of the current network op (e2e only)
	pclk_t				my_current_net_elapsed_time;// Elapsed time of the current network op (e2e only)
	pclk_t				my_accumulated_op_time; 	// Accumulated time spent in I/O 
	pclk_t				my_accumulated_read_op_time; // Accumulated time spent in read 
	pclk_t				my_accumulated_write_op_time;// Accumulated time spent in write 
	pclk_t				my_accumulated_noop_op_time;// Accumulated time spent in noops 
	pclk_t				my_accumulated_pattern_fill_time; // Accumulated time spent in data pattern fill before all I/O operations 
	pclk_t				my_accumulated_flush_time; 	// Accumulated time spent doing flush (fsync) operations
	// Updated by the QThread at different times
	char				my_time_limit_expired;		// Time limit expired indicator
	char				available1;					// padding
	char				available2;					// padding
	char				available3;					// padding
	char				available4;					// padding
	char				available5;					// padding
	char				available6;					// padding
	char				available7;					// padding
	int32_t				my_current_state;			// State of this thread at any given time (see Current State definitions below)
	pthread_mutex_t 	my_current_state_mutex; 	// Mutex for locking when checking or updating the state info
	// State Definitions for "my_current_state"
#define	CURRENT_STATE_INIT								0x0000000000000001	// Initialization 
#define	CURRENT_STATE_IO								0x0000000000000002	// Waiting for an I/O operation to complete
#define	CURRENT_STATE_DEST_RECEIVE						0x0000000000000004	// Waiting to receive data - Destination side of an E2E operation
#define	CURRENT_STATE_SRC_SEND							0x0000000000000008	// Waiting for "send" to send data - Source side of an E2E operation
#define	CURRENT_STATE_BARRIER							0x0000000000000010	// Waiting inside a barrier
#define	CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE		0x0000000000000020	// Waiting on the "any qthread available" semaphore
#define	CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE	0x0000000000000040	// Waiting on the "This QThread Available" semaphore
#define	CURRENT_STATE_PASS_COMPLETE						0x0000000000000080	// Indicates that this Target Thread has completed a pass
#define	CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_UPDATE	0x0000000000000100	// QThread is waiting for the TOT lock in order to update the block number
#define	CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_RELEASE	0x0000000000000200	// QThread is waiting for the TOT lock in order to release the next I/O
#define	CURRENT_STATE_QT_WAITING_FOR_TOT_LOCK_TS		0x0000000000000400	// QThread is waiting for the TOT lock to set the "wait" time stamp
#define	CURRENT_STATE_QT_WAITING_FOR_PREVIOUS_IO		0x0000000000000800	// Waiting on the previous I/O op semaphore

	//
	// Longest and shortest op times - RESET AT THE START OF EACH PASS 
	// These values are only updated when the -extendedstats option is specified
	pclk_t		my_longest_op_time; 			// Longest op time that occured during this pass
	pclk_t		my_longest_read_op_time; 		// Longest read op time that occured during this pass
	pclk_t		my_longest_write_op_time; 		// Longest write op time that occured during this pass
	pclk_t		my_longest_noop_op_time; 		// Longest noop op time that occured during this pass
	pclk_t		my_shortest_op_time; 			// Shortest op time that occurred during this pass
	pclk_t		my_shortest_read_op_time; 		// Shortest read op time that occured during this pass
	pclk_t		my_shortest_write_op_time; 		// Shortest write op time that occured during this pass
	pclk_t		my_shortest_noop_op_time; 		// Shortest noop op time that occured during this pass

	int64_t		my_longest_op_bytes; 			// Bytes xfered when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_bytes; 		// Bytes xfered when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_bytes; 		// Bytes xfered when the longest write op time occured during this pass
	int64_t 	my_longest_noop_op_bytes; 		// Bytes xfered when the longest noop op time occured during this pass
	int64_t 	my_shortest_op_bytes; 			// Bytes xfered when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_bytes; 		// Bytes xfered when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_bytes;		// Bytes xfered when the shortest write op time occured during this pass
	int64_t 	my_shortest_noop_op_bytes;		// Bytes xfered when the shortest noop op time occured during this pass

	int64_t		my_longest_op_number; 			// Operation Number when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_number; 		// Operation Number when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_number; 	// Operation Number when the longest write op time occured during this pass
	int64_t 	my_longest_noop_op_number; 		// Operation Number when the longest noop op time occured during this pass
	int64_t 	my_shortest_op_number; 			// Operation Number when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_number; 	// Operation Number when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_number;	// Operation Number when the shortest write op time occured during this pass
	int64_t 	my_shortest_noop_op_number;		// Operation Number when the shortest noop op time occured during this pass

	int32_t		my_longest_op_pass_number;		// Pass Number when the longest op time occured during this pass
	int32_t		my_longest_read_op_pass_number;	// Pass Number when the longest read op time occured
	int32_t		my_longest_write_op_pass_number;// Pass Number when the longest write op time occured 
	int32_t		my_longest_noop_op_pass_number;	// Pass Number when the longest noop op time occured 
	int32_t		my_shortest_op_pass_number;		// Pass Number when the shortest op time occured 
	int32_t		my_shortest_read_op_pass_number;// Pass Number when the shortest read op time occured 
	int32_t		my_shortest_write_op_pass_number;// Pass Number when the shortest write op time occured 
	int32_t		my_shortest_noop_op_pass_number;// Pass Number when the shortest noop op time occured 
    // ------------------ End of the PASS-Related stuff --------------------------------------------------

	// The following variables are used by the "-reopen" option
	pclk_t        		open_start_time; 			// Time just before the open is issued for this target 
	pclk_t        		open_end_time; 				// Time just after the open completes for this target 

	// The following variables are used for Deskew Operations
	uint64_t      		front_end_skew_bytes; 		// The number of bytes transfered during the front end skew period 
	uint64_t      		deskew_window_bytes; 		// The number of bytes transferred during the deskew window 
	uint64_t      		back_end_skew_bytes; 		// The number of bytes transfered during the back end skew period 
    // -------------------------------------------------------------------
	// The following variables are used to implement the various trigger options 
	pclk_t        		start_trigger_time; 		// Time to trigger another target to start 
	pclk_t        		stop_trigger_time; 			// Time to trigger another target to stop 
	int64_t       		start_trigger_op; 			// Operation number to trigger another target to start 
	int64_t       		stop_trigger_op; 			// Operation number  to trigger another target to stop
	double        		start_trigger_percent; 		// Percentage of ops before triggering another target to start 
	double        		stop_trigger_percent; 		// Percentage of ops before triggering another target to stop 
	int64_t       		start_trigger_bytes; 		// Number of bytes to transfer before triggering another target to start 
	int64_t       		stop_trigger_bytes; 		// Number of bytes to transfer before triggering another target to stop 
#define TRIGGER_STARTTIME    0x00000001				// Trigger type of "time" 
#define TRIGGER_STARTOP      0x00000002				// Trigger type of "op" 
#define TRIGGER_STARTPERCENT 0x00000004				// Trigger type of "percent" 
#define TRIGGER_STARTBYTES   0x00000008				// Trigger type of "bytes" 
	uint32_t			trigger_types;				// This is the type of trigger to administer to another target 
	int32_t				start_trigger_target;		// The number of the target to send the start trigger to 
	int32_t				stop_trigger_target;		// The number of the target to send the stop trigger to 
	uint32_t			run_status; 				// This is the status of this thread 0=not started, 1=running 
	xdd_barrier_t		target_target_starttrigger_barrier;	// Start Trigger Barrier 

    // -------------------------------------------------------------------
	// The following "e2e_" members are for the End to End ( aka -e2e ) option
	// The Target Option Flag will have either TO_E2E_DESTINATION or TO_E2E_SOURCE set to indicate
	// which side of the E2E operation this target will act as.
	//
	char				*e2e_dest_hostname; 	// Name of the Destination machine 
	char				*e2e_src_hostname; 		// Name of the Source machine 
	struct hostent 		*e2e_dest_hostent; 		// For the destination host information 
	struct hostent 		*e2e_src_hostent; 		// For the source host information 
	in_addr_t			e2e_dest_addr;  		// Destination Address number of the E2E socket 
	in_port_t 			e2e_dest_base_port;  	// The first of one or more ports to be used by multiple queuethreads 
	in_port_t			e2e_dest_port;  		// Port number to use for the E2E socket 
	int32_t				e2e_sd;   				// Socket descriptor for the E2E message port 
	int32_t				e2e_nd;   				// Number of Socket descriptors in the read set 
	sd_t				e2e_csd[FD_SETSIZE];	// Client socket descriptors 
	fd_set				e2e_active;  			// This set contains the sockets currently active 
	fd_set				e2e_readset; 			// This set is passed to select() 
	struct sockaddr_in  e2e_sname; 				// used by setup_server_socket 
	uint32_t			e2e_snamelen; 			// the length of the socket name 
	struct sockaddr_in  e2e_rname; 				// used by destination machine to remember the name of the source machine 
	uint32_t			e2e_rnamelen; 			// the length of the source socket name 
	int32_t				e2e_current_csd; 		// the current csd used by the select call on the destination side
	int32_t				e2e_next_csd; 			// The next available csd to use 
	int32_t				e2e_iosize;   			// Number of bytes per End to End request - size of data buffer plus size of E2E Header
	int32_t				e2e_send_status; 		// Current Send Status
	int32_t				e2e_recv_status; 		// Current Recv status
#define PTDS_E2E_MAGIC 	0x07201959 				// The magic number that should appear at the beginning of each message 
#define PTDS_E2E_MAGIQ 	0x07201960 				// The magic number that should appear in a message signaling destination to quit 
	xdd_e2e_header_t 	e2e_header;				// Header (actually a trailer) in the data packet of each message sent/received
	int64_t				e2e_msg_sequence_number;// The Message Sequence Number of the most recent message sent or to be received
	int32_t				e2e_msg_sent; 			// The number of messages sent 
	int32_t				e2e_msg_recv; 			// The number of messages received 
	int64_t				e2e_prev_loc; 			// The previous location from a e2e message from the source 
	int64_t				e2e_prev_len; 			// The previous length from a e2e message from the source 
	int64_t				e2e_data_recvd; 		// The amount of data that is received each time we call xdd_e2e_dest_recv()
	int64_t				e2e_data_length; 		// The amount of data that is ready to be read for this operation 
	pclk_t				e2e_wait_1st_msg;		// Time in picosecs destination waited for 1st source data to arrive 
	pclk_t				e2e_first_packet_received_this_pass;// Time that the first packet was received by the destination from the source
	pclk_t				e2e_last_packet_received_this_pass;// Time that the last packet was received by the destination from the source
	pclk_t				e2e_first_packet_received_this_run;// Time that the first packet was received by the destination from the source
	pclk_t				e2e_last_packet_received_this_run;// Time that the last packet was received by the destination from the source
	pclk_t				e2e_sr_time; 			// Time spent sending or receiving data for End-to-End operation
	int32_t				e2e_address_table_host_count;	// Cumulative number of hosts represented in the e2e address table
	int32_t				e2e_address_table_port_count;	// Cumulative number of ports represented in the e2e address table
	int32_t				e2e_address_table_next_entry;	// Next available entry in the e2e_address_table
	xdd_e2e_ate_t		e2e_address_table[E2E_ADDRESS_TABLE_ENTRIES]; // Used by E2E to stripe over multiple IP Addresses
	// ------------------ End of the End to End (E2E) stuff --------------------------------------
	//
	// The following "raw_" members are for the ReadAfterWrite (-raw) option
	char				*raw_myhostname; 		// Hostname of the reader machine as seen by the reader 
	char				*raw_hostname; 			// Name of the host doing the reading in a read-after-write 
	struct hostent 		*raw_hostent; 			// for the reader/writer host information 
	in_port_t			raw_port;  				// Port number to use for the read-after-write socket 
	in_addr_t			raw_addr;  				// Address number of the read-after-write socket
	int32_t				raw_lag;  				// Number of blocks the reader should lag behind the writer 
#define PTDS_RAW_STAT 0x00000001  				// Read-after-write should use stat() to trigger read operations 
#define PTDS_RAW_MP   0x00000002  				// Read-after-write should use message passing from the writer to trigger read operations 
	uint32_t			raw_trigger; 			// Read-After-Write trigger mechanism 
	int32_t				raw_sd;   				// Socket descriptor for the read-after-write message port 
	int32_t				raw_nd;   				// Number of Socket descriptors in the read set 
	sd_t				raw_csd[FD_SETSIZE];	// Client socket descriptors 
	fd_set				raw_active;  			// This set contains the sockets currently active 
	fd_set				raw_readset; 			// This set is passed to select() 
	struct sockaddr_in  raw_sname; 				// used by setup_server_socket 
	uint32_t			raw_snamelen; 			// the length of the socket name 
	int32_t				raw_current_csd; 		// the current csd used by the raw reader select call 
	int32_t				raw_next_csd; 			// The next available csd to use 
#define PTDS_RAW_MAGIC 0x07201958 				// The magic number that should appear at the beginning of each message 
	xdd_raw_msg_t		raw_msg;  				// The message sent in from the writer 
	int64_t				raw_msg_last_sequence;	// The last raw msg sequence number received 
	int32_t				raw_msg_sent; 			// The number of messages sent 
	int32_t				raw_msg_recv; 			// The number of messages received 
	int64_t				raw_prev_loc; 			// The previous location from a RAW message from the source 
	int64_t				raw_prev_len; 			// The previous length from a RAW message from the source 
	int64_t				raw_data_ready; 		// The amount of data that is ready to be read in an RAW op 
	int64_t				raw_data_length; 		// The amount of data that is ready to be read for this operation 
	// ------------------ End of the ReadAfterWrite stuff --------------------------------------
	struct lockstep		*lockstepp;				// pointer to the lockstep structure used by the lockstep option
	struct restart		*restartp;				// pointer to the restart structure used by the restart monitor
	struct ptds			*pm1;					// ptds minus  1 - used for report print queueing - don't ask 
#if (LINUX)
	struct stat			statbuf;				// Target File Stat buffer used by xdd_target_open()
#elif (AIX || SOLARIS)
	struct stat64		statbuf;				// Target File Stat buffer used by xdd_target_open()
#endif
};
typedef struct ptds ptds_t;

#include "lockstep.h"
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
