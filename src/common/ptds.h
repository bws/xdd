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
#include "datapatterns.h"
#include "results.h"
#include "access_pattern.h"
#include "timestamp.h"
#include "barrier.h"
#include "read_after_write.h"
#include "end_to_end.h"
#include "parse.h"
#include "target_offset_table.h"
#include "heartbeat.h"
#include "sgio.h"
#include "triggers.h"
#include "extended_stats.h"


// Bit settings that are used in the Target Options (TO_XXXXX bit definitions) 64-bit word in the PTDS
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
struct ptds {
	struct ptds 		*target_ptds; 		// Pointer back to the Parent Target PTDS 
	struct ptds 		*next_qp; 			// Pointer to the next QThread PTDS in the PTDS Substructure
	pthread_t  			target_thread;		// Handle for this Target Thread 
	pthread_t  			qthread;			// Handle for this QThread 
	int32_t   			my_target_number;	// My target number 
	int32_t   			my_qthread_number;	// My queue number within this target 
	int32_t   			my_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   			my_pid;   			// My process ID 
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
	double				start_delay; 			// number of seconds to delay the start  of this operation 
	pclk_t				start_delay_psec;		// number of picoseconds to delay the start  of this operation 
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
    // ------------------ Heartbeat stuff --------------------------------------------------
	// The following heartbeat structure and data is for the -heartbeat option
	struct heartbeat	hb;						// Heartbeat data
    //
    // ------------------ TimeStamp stuff --------------------------------------------------
	// The following "ts_" members are for the time stamping (-ts) option
	uint64_t			ts_options;  			// Time Stamping Options 
	int64_t				ts_current_entry; 		// Index into the Timestamp Table of the current entry
	int64_t				ts_size;  				// Time Stamping Size in number of entries 
	int64_t				ts_trigop;  			// Time Stamping trigger operation number 
	pclk_t				ts_trigtime; 			// Time Stamping trigger time 
	char				*ts_binary_filename; 	// Timestamp filename for the binary output file for this Target
	char				*ts_output_filename; 	// Timestamp report output filename for this Target
	//
    // ------------------ RUNTIME stuff --------------------------------------------------
    // Stuff REFERENCED during runtime
	//
	pclk_t				run_start_time; 			// This is time t0 of this run - set by xdd_main
	pclk_t				first_pass_start_time; 		// Time the first pass started but before the first operation is issued
	uint64_t			target_bytes_to_xfer_per_pass; 	// Number of bytes to xfer per pass for the entire target (all qthreads)
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
	char				abort;						// Abort this operation (either a QThread or a Target Thread)
	char				run_complete;				// Indicates that the entire RUN of all PASSES has completed
	pthread_mutex_t 	my_current_state_mutex; 	// Mutex for locking when checking or updating the state info
	int32_t				my_current_state;			// State of this thread at any given time (see Current State definitions below)
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

	// The following variables are used by the "-reopen" option
	pclk_t        		open_start_time; 			// Time just before the open is issued for this target 
	pclk_t        		open_end_time; 				// Time just after the open completes for this target 

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
	struct xdd_extended_stats	*esp;			// Extended Stats Structure Pointer
	struct xdd_triggers			*trigp;			// Triggers Structure Pointer
	struct xdd_sgio				*sgiop;			// SGIO Structure Pointer
	struct xdd_data_pattern		*dpp;			// Data Pattern Structure Pointer
	struct xdd_raw				*rawp;			// RAW Data Structure Pointer
	struct lockstep				*lockstepp;		// pointer to the lockstep structure used by the lockstep option
	struct restart				*restartp;		// pointer to the restart structure used by the restart monitor
	struct ptds					*pm1;			// ptds minus  1 - used for report print queueing - don't ask 
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
