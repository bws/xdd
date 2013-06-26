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

// Worker Thread Data Structure - one for each worker thread 
struct xint_worker_data {
	target_data_t 				*wd_tdp;	 		// Pointer back to the Parent Target Data
	struct xint_worker_data		*wd_next_wdp;		// Pointer to the next worker_data struct in the list
	pthread_t  					wd_thread;			// Handle for this Worker_Thread
	int32_t   					wd_thread_number;	// My queue number within this target 
	int32_t   					wd_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   					wd_pid;   			// My process ID 
	int32_t   					wd_file_desc; 		// File Descriptor for the target device/file 
	// The Occupant Strcuture used by the barriers 
	xdd_occupant_t				wd_occupant;			// Used by the barriers to keep track of what is in a barrier at any given time
	char						wd_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH];	// For a Target thread this is "TARGET####", for a Worker_Thread it is "TARGET####WORKER####"
	xdd_barrier_t				*wd_current_barrier;	// Pointer to the current barrier this PTDS is in at any given time or NULL if not in a barrier
	nclk_t						wd_first_pass_start_time;	// Start time for the first I/O operation this Worker Thread performed
	nclk_t						wd_pass_start_time;			// Start time for the Worker Thread in this pass
	nclk_t						wd_pass_end_time;			// End time for the Worker Thread in this pass
	struct	tms					wd_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	struct	tms					wd_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	struct	tms					wd_current_cpu_times;		// CPU times from times()
	nclk_t						wd_current_op_start_time;	// Start time for a specific I/O operation
	nclk_t						wd_current_op_end_time;		// End time for a specific I/O operation
	nclk_t						wd_current_net_start_time;	// Start time for a specific I/O operation
	nclk_t						wd_current_net_end_time;		// End time for a specific I/O operation
	int64_t						wd_current_byte_location; 	// Current byte location for this I/O operation 
	int64_t						wd_current_op_number; 		// Current I/O operation number that this worker thread is performing for the Target
	int32_t						wd_current_io_size; 		// Size of the I/O to be performed
	int32_t						wd_current_io_status; 		// Status of the last I/O performed
	char						*wd_current_op_str; 		// Pointer to an ASCII string of the I/O operation type - "READ", "WRITE", or "NOOP"
	int32_t						wd_current_op_type; 		// Current I/O operation type - OP_TYPE_READ or OP_TYPE_WRITE
	int64_t						wd_current_error_count;		// The number of I/O errors for this qthread
	int32_t						wd_current_io_errno; 		// The errno associated with the status of this I/O for this thread
	unsigned char 				*wd_current_rwbuf;   		// Pointer to the current read/write buffer
	int32_t						wd_rwbuf_shmid; 			// Shared Memory ID
	int64_t						wd_ts_current_entry;		// The TimeStamp entry to use when time-stamping an operation
// The task variables
	char				wd_task_request;						// Type of Task to perform
#define TASK_REQ_IO				0x01						// Perform an IO Operation 
#define TASK_REQ_REOPEN			0x02						// Re-Open the target device/file
#define TASK_REQ_STOP			0x03						// Stop doing work and exit
#define TASK_REQ_EOF			0x04						// Send an EOF to the Destination or Revceive an EOF from the Source
	char				wd_task_op;							// Operation to perform
	uint32_t			wd_task_xfer_size;						// Number of bytes to transfer
	uint64_t			wd_task_byte_location;					// Offset into the file where this transfer starts
	uint64_t			wd_task_e2e_sequence_number;			// Sequence number of this task when part of an End-to-End operation
	nclk_t				wd_task_time_to_issue;					// Time to issue the I/O operation or 0 if not used


	// Worker Thread-specific locks and associated pointers
	pthread_mutex_t		wd_worker_thread_target_sync_mutex;			// Used to serialize access to the Worker_Thread-Target Synchronization flags
	int32_t				wd_worker_thread_target_sync;				// Flags used to synchronize a Worker_Thread with its Target
#define	WDSYNC_AVAILABLE			0x00000001				// This Worker_Thread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
#define	WDSYNC_BUSY					0x00000002				// This Worker_Thread is busy
#define	WDSYNC_TARGET_WAITING		0x00000004				// The parent Target is waiting for this Worker_Thread to become available, set by xdd_get_specific_qthread, reset by qthread.
#define	WDSYNC_EOF_RECEIVED			0x00000008				// This Worker_Thread received an EOF packet from the Source Side of an E2E Operation
    //sem_t				this_qthread_is_available_sem;		// xdd_get_specific_qthread() routine waits on this for any Worker_Thread to become available
    pthread_cond_t wd_this_worker_thread_is_available_condition;
    
	xdd_barrier_t		wd_thread_targetpass_wait_for_task_barrier;	// The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
	xint_e2e_t			*wd_e2ep;			// Pointer to the e2e struct when needed
	xdd_sgio_t			*wd_sgiop;			// SGIO Structure Pointer
};
typedef struct xint_worker_data worker_data_t;
struct xint_buf_mgt {
	unsigned char 				*bm_rwbuf;   		// The re-aligned I/O buffers 
	int32_t						bm_rwbuf_shmid; 	// Shared Memeory ID for rwbuf 
	unsigned char 				*bm_rwbuf_save; 	// The original I/O buffers 
	int32_t						bm_iobuffersize; 	// Size of the I/O buffer in bytes 
};
typedef struct xint_buf_mgt xint_buf_mgt_t;
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
