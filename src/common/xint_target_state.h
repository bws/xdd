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
#include "xint_nclk.h"
#include "xint_plan.h"
struct xint_target_state {
	// Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
	nclk_t				tc_pass_start_time; 		// The time stamp that this pass started but before the first operation is issued
	nclk_t				tc_pass_end_time; 			// The time stamp that this pass ended 
	struct	tms			tc_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	struct	tms			tc_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	struct	tms			tc_current_cpu_times;		// CPU times from times()
	//
	// Updated by xdd_issue() at at the start of a Task IO request to a Worker Thread
	int32_t				tc_current_pass_number; 	// Current pass number 
	int64_t				tc_current_byte_location; 	// Current byte location for this I/O operation 
	int32_t				tc_current_io_size; 		// Size of the I/O to be performed
	char				*tc_current_op_str; 		// Pointer to an ASCII string of the I/O operation type - "READ", "WRITE", or "NOOP"
	int32_t				tc_current_op_type; 		// Current I/O operation type - OP_TYPE_READ or OP_TYPE_WRITE
#define OP_TYPE_READ	0x01						// used with my_current_op_type
#define OP_TYPE_WRITE	0x02						// used with my_current_op_type
#define OP_TYPE_NOOP	0x03						// used with my_current_op_type
#define OP_TYPE_EOF		0x04						// used with my_current_op_type - indicates End-of-File processing when present in the Time Stamp Table
	// Updated by the Worker Thread upon completion of an I/O operation
	int64_t				tc_target_op_number;			// The operation number for the target that this I/O represents
	int64_t				tc_current_op_number;		// Current I/O operation number 
	int64_t				tc_current_op_count; 		// The number of read+write operations that have completed so far
	int64_t				tc_current_read_op_count;	// The number of read operations that have completed so far 
	int64_t				tc_current_write_op_count;	// The number of write operations that have completed so far 
	int64_t				tc_current_noop_op_count;	// The number of noops that have completed so far 
	int64_t				tc_current_bytes_xfered_this_op; // Total number of bytes transferred for the most recent completed I/O operation
	int64_t				tc_current_bytes_xfered;	// Total number of bytes transferred so far (to storage device, not network)
	int64_t				tc_current_bytes_read;		// Total number of bytes read so far (from storage device, not network)
	int64_t				tc_current_bytes_written;	// Total number of bytes written so far (to storage device, not network)
	int64_t				tc_current_bytes_noop;		// Total number of bytes processed by noops so far
	int32_t				tc_current_io_status; 		// I/O Status of the last I/O operation for this Worker Thread
	int32_t				tc_current_io_errno; 		// The errno associated with the status of this I/O for this thread
	int64_t				tc_current_error_count;		// The number of I/O errors for this Worker Thread
	nclk_t				tc_elapsed_pass_time; 		// Rime between the start and end of this pass
	nclk_t				tc_first_op_start_time;		// Time this Worker Thread was able to issue its first operation for this pass
	nclk_t				tc_current_op_start_time; 	// Start time of the current op
	nclk_t				tc_current_op_end_time; 	// End time of the current op
	nclk_t				tc_current_op_elapsed_time;	// Elapsed time of the current op
	nclk_t				tc_current_net_start_time; 	// Start time of the current network op (e2e only)
	nclk_t				tc_current_net_end_time; 	// End time of the current network op (e2e only)
	nclk_t				tc_current_net_elapsed_time;// Elapsed time of the current network op (e2e only)
	nclk_t				tc_accumulated_op_time; 	// Accumulated time spent in I/O 
	nclk_t				tc_accumulated_read_op_time; // Accumulated time spent in read 
	nclk_t				tc_accumulated_write_op_time;// Accumulated time spent in write 
	nclk_t				tc_accumulated_noop_op_time;// Accumulated time spent in noops 
	nclk_t				tc_accumulated_pattern_fill_time; // Accumulated time spent in data pattern fill before all I/O operations 
	nclk_t				tc_accumulated_flush_time; 	// Accumulated time spent doing flush (fsync) operations
	// Updated by the Worker Thread at different times
	char				tc_time_limit_expired;		// Time limit expired indicator
	char				abort;						// Abort this operation (either a Worker Thread or a Target Thread)
	char				run_complete;				// Indicates that the entire RUN of all PASSES has completed
	pthread_mutex_t 	my_current_state_mutex; 	// Mutex for locking when checking or updating the state info
	int32_t				my_current_state;			// State of this thread at any given time (see Current State definitions below)
	// State Definitions for "my_current_state"
#define	CURRENT_STATE_INIT								0x0000000000000001	// Initialization 
#define	CURRENT_STATE_IO								0x0000000000000002	// Waiting for an I/O operation to complete
#define	CURRENT_STATE_DEST_RECEIVE						0x0000000000000004	// Waiting to receive data - Destination side of an E2E operation
#define	CURRENT_STATE_SRC_SEND							0x0000000000000008	// Waiting for "send" to send data - Source side of an E2E operation
#define	CURRENT_STATE_BARRIER							0x0000000000000010	// Waiting inside a barrier
#define	CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE	0x0000000000000020	// Waiting on the "any Worker Thread available" semaphore
#define	CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE	0x0000000000000040	// Waiting on the "This Worker Thread Available" semaphore
#define	CURRENT_STATE_PASS_COMPLETE						0x0000000000000080	// Indicates that this Target Thread has completed a pass
#define	CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE	0x0000000000000100	// Worker Thread is waiting for the TOT lock in order to update the block number
#define	CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE	0x0000000000000200	// Worker Thread is waiting for the TOT lock in order to release the next I/O
#define	CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS		0x0000000000000400	// Worker Thread is waiting for the TOT lock to set the "wait" time stamp
#define	CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO		0x0000000000000800	// Waiting on the previous I/O op semaphore

};
typedef struct xint_target_state xint_target_state_t;

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
