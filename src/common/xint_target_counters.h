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
struct xint_target_counters {
	// Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
	int32_t		tc_pass_number; 				// Current pass number 
	nclk_t		tc_pass_start_time; 			// The time this pass started but before the first operation is issued
	nclk_t		tc_pass_end_time; 				// The time stamp that this pass ended 
	nclk_t		tc_pass_elapsed_time; 			// Time between the start and end of this pass
	nclk_t		tc_first_op_start_time;			// Time this Worker Thread was able to issue its first operation for this pass
	struct	tms	tc_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	struct	tms	tc_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	struct	tms	tc_current_cpu_times;			// CPU times from times()

	// Updated by the Worker Thread in Worker Data during an I/O operation
	// and then copied to the Target Data strcuture at the completion of an I/O operation
	int64_t		tc_current_byte_offset; 		// Current byte location for this I/O operation 
	int64_t		tc_current_op_number;			// Current I/O operation number 
	int32_t		tc_current_io_status; 			// I/O Status of the last I/O operation for this Worker Thread
	int32_t		tc_current_io_errno; 			// The errno associated with the status of this I/O for this thread
	int64_t		tc_current_error_count;			// The number of I/O errors for this Worker Thread
	nclk_t		tc_current_op_start_time; 		// Start time of the current op
	nclk_t		tc_current_op_end_time; 		// End time of the current op
	nclk_t		tc_current_op_elapsed_time;		// Elapsed time of the current op
	nclk_t		tc_current_net_start_time; 		// Start time of the current network op (e2e only)
	nclk_t		tc_current_net_end_time; 		// End time of the current network op (e2e only)
	nclk_t		tc_current_net_elapsed_time;	// Elapsed time of the current network op (e2e only)

	// Accumulated Counters 
	// Updated in the Target Data structure by each Worker Thread at the completion of an I/O operation
	int64_t		tc_accumulated_op_count; 		// The number of read+write operations that have completed so far
	int64_t		tc_accumulated_read_op_count;	// The number of read operations that have completed so far 
	int64_t		tc_accumulated_write_op_count;	// The number of write operations that have completed so far 
	int64_t		tc_accumulated_noop_op_count;	// The number of noops that have completed so far 
	int64_t		tc_accumulated_bytes_xfered;	// Total number of bytes transferred so far (to storage device, not network)
	int64_t		tc_accumulated_bytes_read;		// Total number of bytes read so far (from storage device, not network)
	int64_t		tc_accumulated_bytes_written;	// Total number of bytes written so far (to storage device, not network)
	int64_t		tc_accumulated_bytes_noop;		// Total number of bytes processed by noops so far
	nclk_t		tc_accumulated_op_time; 		// Accumulated time spent in I/O 
	nclk_t		tc_accumulated_read_op_time; 	// Accumulated time spent in read 
	nclk_t		tc_accumulated_write_op_time;	// Accumulated time spent in write 
	nclk_t		tc_accumulated_noop_op_time;	// Accumulated time spent in noops 
	nclk_t		tc_accumulated_pattern_fill_time; // Accumulated time spent in data pattern fill before all I/O operations 
	nclk_t		tc_accumulated_flush_time; 		// Accumulated time spent doing flush (fsync) operations
};
typedef struct xint_target_counters xint_target_counters_t;

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
