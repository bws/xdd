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
#ifdef WIN32
#include "nt_unix_compat.h"
#endif
#include "barrier.h"
#include "results.h"
#include "access_pattern.h"
#include "timestamp.h"
#include "data_pattern.h"
#include "lockstep.h"
#include "trigger.h"
#include "read_after_write.h"
#include "end_to_end.h"
#include "parse.h"

#define HOSTNAMELENGTH 1024

// Bit settings that are used in the Target Options (TO_XXXXX bit definitions) 64-bit word in the PTDS
#define TO_READAFTERWRITE      0x0000000000000001ULL  // Read-After-Write - the -raw option 
#define TO_RAW_READER          0x0000000000000002ULL  // Read-After-Write - reader 
#define TO_RAW_WRITER          0x0000000000000004ULL  // Read-After-Write - writer 
#define TO_ENDTOEND            0x0000000000000008ULL  // End to End - aka -e2e option 
#define TO_E2E_SOURCE          0x0000000000000010ULL  // End to End - Source side 
#define TO_E2E_DESTINATION     0x0000000000000020ULL  // End to End - Destination side 
#define TO_SGIO                0x0000000000000040ULL  // Used for SCSI Generic I/O in Linux 
#define TO_DIO                 0x0000000000000080ULL  // DIRECT IO for files 
#define TO_RANDOM_PATTERN      0x0000000000000100ULL  // Use random data pattern for write operations 
#define TO_DELETEFILE          0x0000000000000200ULL  // Delete target file upon completion of write 
#define TO_REGULARFILE         0x0000000000000400ULL  // Target file is a REGULAR file 
#define TO_DEVICEFILE          0x0000000000000800ULL  // Target is a Device - could be an SG device 
#define TO_SOCKET              0x0000000000001000ULL  // Target is a Socket kind of file 
#define TO_WAITFORSTART        0x0000000000002000ULL  // Wait for START trigger 
#define TO_LOCKSTEP            0x0000000000004000ULL  // Normal Lock step mode 
#define TO_LOCKSTEPOVERLAPPED  0x0000000000008000ULL  // Overlapped lock step mode 
#define TO_SHARED_MEMORY       0x0000000000010000ULL  // Use a shared memory segment instead of malloced memmory 
#define TO_SEQUENCED_PATTERN   0x0000000000020000ULL  // Sequenced Data Pattern in the data buffer 
#define TO_ASCII_PATTERN       0x0000000000040000ULL  // ASCII Data Pattern in the data buffer 
#define TO_HEX_PATTERN         0x0000000000080000ULL  // HEXIDECIMAL Data Pattern in the data buffer 
#define TO_SINGLECHAR_PATTERN  0x0000000000100000ULL  // Single character Data Pattern in the data buffer 
#define TO_FILE_PATTERN        0x0000000000200000ULL  // Name of file that contains the Data Pattern 
#define TO_REPLICATE_PATTERN   0x0000000000400000ULL  // Replicate Data Pattern throughout the data buffer 
#define TO_LFPAT_PATTERN       0x0000000000800000ULL  // Low Frequency FC/SAS pattern 
#define TO_LTPAT_PATTERN       0x0000000001000000ULL  // Low Transition FC/SAS pattern 
#define TO_CJTPAT_PATTERN      0x0000000002000000ULL  // Receiver Jitter FC/SAS pattern 
#define TO_CRPAT_PATTERN       0x0000000004000000ULL  // Random FC/SAS pattern 
#define TO_CSPAT_PATTERN       0x0000000008000000ULL  // Supply Noise FC/SAS pattern 
#define TO_PATTERN_PREFIX      0x0000000010000000ULL  // Indicates that there is a data pattern prefix 
#define TO_INVERSE_PATTERN     0x0000000020000000ULL  // Apply a 1's compliment to the data pattern 
#define TO_NAME_PATTERN        0x0000000040000000ULL  // Use the specified name at the beginning of the data pattern
#define TO_PCPU_ABSOLUTE       0x0000000080000000ULL  // Defines the meaning of the percent CPU values on the output 
#define TO_REOPEN              0x0000000100000000ULL  // Open/Close target on each pass and record time 
#define TO_CREATE_NEW_FILES    0x0000000200000000ULL  // Create new targets for each pass 
#define TO_RECREATE            0x0000000400000000ULL  // ReCreate targets for each pass 
#define TO_SYNCWRITE           0x0000000800000000ULL  // Sync buffered write operations 
#define TO_PASS_RANDOMIZE      0x0000001000000000ULL  // Pass randomize 
#define TO_QTHREAD_INFO        0x0000002000000000ULL  // Display Q thread information 
#define TO_VERIFY_CONTENTS     0x0000004000000000ULL  // Verify the contents of the I/O buffer 
#define TO_VERIFY_LOCATION     0x0000008000000000ULL  // Verify the location of the I/O buffer (first 8 bytes) 

// Per Thread Data Structure - one for each thread 
struct ptds {
	struct				ptds *nextp; 		// pointers to the next ptds in the queue 
	pthread_t  			thread;   			// Handle for this thread 
	int32_t   			my_target_number;	// My target number 
	int32_t   			my_qthread_number;	// My queue number within this target 
	int32_t   			mythreadnum; 		// My thread number relative to the total number of threads
	int32_t   			mythreadid;  		// My system thread ID (like a process ID) 
	int32_t   			mypid;   			// My process ID 
	int32_t   			total_threads; 		// Total number of threads -> target threads + QThreads 
	volatile int32_t   	run_complete; 		// 0 = thread has not completed yet, 1= completed this run
	volatile int32_t   	pass_complete; 		// 0 = thread has not completed yet, 1= completed this pass
#ifdef WIN32
	HANDLE   			fd;    				// File HANDLE for the target device/file 
#else
	int32_t   			fd;    				// File Descriptor for the target device/file 
#endif
	unsigned char 		*rwbuf;   		// The re-aligned I/O buffers 
	int32_t				rwbuf_shmid; 		// Shared Memeory ID for rwbuf 
	unsigned char 		*rwbuf_save; 		// The original I/O buffers 
	int32_t				iobuffersize; 		// Size of the I/O buffer in bytes 
	int32_t				iosize;   			// Number of bytes per request 
	int32_t				actual_iosize;   	// Number of bytes actually transferred for this request 
	int32_t				last_iosize; 		// Number of bytes for the final request 
	int32_t				op_delay; 			// Number of seconds to delay between operations 
	int32_t				filetype;  			// Type of file: regular, device, socket, ... 
	int64_t				filesize;  			// Size of target file in bytes 
	int64_t				target_ops;  		// Total number of ops to perform per qthread 
	int64_t				total_ops;  		// Total number of ops to perform on behalf of a "target"
	int64_t				residual_ops;  		// Number of requests mod the queue depth 
	int64_t				writer_total; 		// Total number of bytes written so far - used by read after write
	seekhdr_t			seekhdr;  			// For all the seek information 
	results_t			qthread_average_results;	// Averaged results for this qthread - accumulated over all passes
#ifdef WIN32
	HANDLE				*ts_serializer_mutex; // needed to circumvent a Windows bug 
	char				ts_serializer_mutex_name[256]; // needed to circumvent a Windows bug 
#endif
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
	int32_t				time_limit;  			// timelimit in seconds for each thread 
	char				*targetdir;  			// The target directory for the target 
	char				*target;  				// devices to perform operation on 
	char				targetext[32]; 			// The target extension number 
	int32_t				processor;  			// Processor/target assignments 
	pclk_t				start_delay; 			// number of picoseconds to delay the start  of this operation 
	double				throttle;  				// Target Throttle assignments 
	double				throttle_variance;  	// Throttle Average Bandwidth variance 
#define PTDS_THROTTLE_OPS   0x00000001  	// Throttle type of OPS 
#define PTDS_THROTTLE_BW    0x00000002  	// Throttle type of Bandwidth 
#define PTDS_THROTTLE_ABW   0x00000004  	// Throttle type of Average Bandwidth 
#define PTDS_THROTTLE_DELAY 0x00000008  	// Throttle type of a constant delay or time for each op 
	uint32_t      		throttle_type; 			// Target Throttle type 
	//
    // Stuff REFERENCED during runtime
	//
	pclk_t				run_start_time; 			// This is time t0 of this run - set by xdd_main
	pclk_t				first_pass_start_time; 		// Time the first pass started but before the first operation is issued
	uint64_t			bytes_to_xfer_per_pass; 	// Number of bytes to xfer per pass 
	int32_t				block_size;  				// Size of a block in bytes for this target 
	int32_t				queue_depth; 				// Command queue depth for each target 
	int32_t				preallocate; 				// File preallocation value 
	int32_t				mem_align;   				// Memory read/write buffer alignment value in bytes 
	uint64_t			target_options; 			// I/O Options specific to each target 
	//
    // Stuff UPDATED during each pass 
	volatile int32_t	my_current_pass_number; 	// Current pass number 
	volatile int32_t	syncio_barrier_index; 		// Used to determine which syncio barrier to use at any given time
	volatile int32_t	thread_barrier_index; 		// Where threads wait for all other threads before starting a pass
	volatile int32_t	results_pass_barrier_index; // Where threads wait for all other threads to complete a pass
	volatile int32_t	results_display_barrier_index; // Where threads wait for the results_manager()to display results
	volatile int32_t	results_run_barrier_index; 	// Where threads wait for all other threads at the completion of the run
	//
	// Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
	pclk_t				my_pass_start_time; 		// The time stamp that this pass started but before the first operation is issued
	pclk_t				my_pass_end_time; 			// The time stamp that this pass ended 
	pclk_t				my_elapsed_pass_time; 		// Rime between the start and end of this pass
	pclk_t				my_first_op_start_time;		// Time this qthread was able to issue its first operation for this pass
	pclk_t				my_current_op_start_time; 	// Start time of the current op
	pclk_t				my_current_op_end_time; 	// End time of the current op
	pclk_t				my_current_op_elapsed_time;	// Elapsed time of the current op
	pclk_t				my_accumulated_op_time; 	// Accumulated time spent in I/O 
	pclk_t				my_accumulated_read_op_time; // Accumulated time spent in read 
	pclk_t				my_accumulated_write_op_time;// Accumulated time spent in write 
	pclk_t				my_accumulated_pattern_fill_time; // Accumulated time spent in data pattern fill before all I/O operations 
	pclk_t				my_accumulated_flush_time; 	// Accumulated time spent doing flush (fsync) operations
	struct	tms			my_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	struct	tms			my_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	struct	tms			my_current_cpu_times;		// CPU times from times()
	//
	// Counters  - RESET AT THE START OF EACH PASS
	volatile int32_t 	my_pass_ring;  				// What xdd hears when the time limit is exceeded for a single pass 
	volatile int64_t	my_current_op; 				// Current I/O operation number 
	volatile int64_t	my_current_op_count; 		// The number of read+write operations that have completed so far
	volatile int64_t	my_current_read_op_count;	// The number of read operations that have completed so far 
	volatile int64_t	my_current_write_op_count;	// The number of write operations that have completed so far 
	volatile int64_t	my_current_bytes_xfered;	// Total number of bytes transferred to far (to storage device, not network)
	volatile int64_t	my_current_bytes_read;		// Total number of bytes read to far (from storage device, not network)
	volatile int64_t	my_current_bytes_written;	// Total number of bytes written to far (to storage device, not network)
	volatile int64_t	my_current_byte_location; 	// Current byte location for this I/O operation 
	volatile int32_t	my_io_status; 				// I/O Status of the last I/O operation for this qthread
	volatile int32_t	my_io_errno; 				// The errno associated with the status of this I/O for this thread
	volatile int32_t	my_error_break; 			// This is set after an I/O Operation if the op failed in some way
	volatile int64_t	my_current_error_count;		// The number of I/O errors for this qthread
	volatile int32_t	my_current_state;			// State of this thread at any given time (see Current State definitions below)
	volatile int32_t	run_status;					// Used by the trigger option and is the status of this thread 0=not started, 1=running 
	// State Definitions for "my_current_state"
#define	CURRENT_STATE_IO		0x0000000000000001	// Currently waiting for an I/O operation to complete
#define	CURRENT_STATE_BTW		0x0000000000000002	// Currently between I/O operations
#define	CURRENT_STATE_BARRIER	0x0000000000000004	// Currently stuck in a barrier
	//
	// Longest and shortest op times - RESET AT THE START OF EACH PASS 
	// These values are only updated when the -extendedstats option is specified
	pclk_t		my_longest_op_time; 			// Longest op time that occured during this pass
	pclk_t		my_longest_read_op_time; 		// Longest read op time that occured during this pass
	pclk_t		my_longest_write_op_time; 		// Longest write op time that occured during this pass
	pclk_t		my_shortest_op_time; 			// Shortest op time that occurred during this pass
	pclk_t		my_shortest_read_op_time; 		// Shortest read op time that occured during this pass
	pclk_t		my_shortest_write_op_time; 		// Shortest write op time that occured during this pass

	int64_t		my_longest_op_bytes; 			// Bytes xfered when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_bytes; 		// Bytes xfered when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_bytes; 		// Bytes xfered when the longest write op time occured during this pass
	int64_t 	my_shortest_op_bytes; 			// Bytes xfered when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_bytes; 		// Bytes xfered when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_bytes;		// Bytes xfered when the shortest write op time occured during this pass

	int64_t		my_longest_op_number; 			// Operation Number when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_number; 		// Operation Number when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_number; 	// Operation Number when the longest write op time occured during this pass
	int64_t 	my_shortest_op_number; 			// Operation Number when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_number; 	// Operation Number when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_number;	// Operation Number when the shortest write op time occured during this pass

	int32_t		my_longest_op_pass_number;		// Pass Number when the longest op time occured during this pass
	int32_t		my_longest_read_op_pass_number;	// Pass Number when the longest read op time occured
	int32_t		my_longest_write_op_pass_number;// Pass Number when the longest write op time occured 
	int32_t		my_shortest_op_pass_number;		// Pass Number when the shortest op time occured 
	int32_t		my_shortest_read_op_pass_number;// Pass Number when the shortest read op time occured 
	int32_t		my_shortest_write_op_pass_number;// Pass Number when the shortest write op time occured 
    // ------------------ End of the PASS-Related stuff --------------------------------------------------


	// The following variables are used by the "-reopen" option
	pclk_t        		open_start_time; 			// Time just before the open is issued for this target 
	pclk_t        		open_end_time; 				// Time just after the open completes for this target 

	// The following variables are used for Deskew Operations
	uint64_t      		front_end_skew_bytes; 		// The number of bytes transfered during the front end skew period 
	uint64_t      		deskew_window_bytes; 		// The number of bytes transferred during the deskew window 
	uint64_t      		back_end_skew_bytes; 		// The number of bytes transfered during the back end skew period 
    // -------------------------------------------------------------------
	// The following pointers are only valid when specific options are used in which case 
	// the associated data structure is allocated and used.
	timestamp_t			*tsp;						// Defines the information for the -timestamp (aka -ts) option
	sgio_t				*sgp;						// Defines the information for the -sgio option or access to a SCSI-GENERIC (sg) device
	data_pattern_t		*dpp;						// Defines the information for the -datapattern option
	lockstep_t			*lsp;						// Defines the  information for the -lockstep option
	e2e_t				*e2ep;						// Defines the information for the -e2e option
	raw_t				*rawp;						// Defines the information for the -raw or read-after-write option
	trigger_t			*tgp;						// Defines the information for the -starttrigger/-stoptrigger options

	struct ptds			*pm1;						// ptds minus  1 - used for report print queueing - don't ask 
};
typedef struct ptds ptds_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
