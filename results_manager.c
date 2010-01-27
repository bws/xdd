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
 *       Steve Hodson, DoE/ORNL
 *       Steve Poole, DoE/ORNL
 *       Bradly Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines necessary to parse the command line
 * arguments  set up all the global and target-specific variables.
 */

#include "xdd.h"

void *xdd_results_dump(results_t *rp, char *dumptype);

/*----------------------------------------------------------------------------*/
// xdd_results_header_display() 
// Display the header and units output lines
// Called by xdd_results_manager() 
//
void *
xdd_results_header_display(results_t *tmprp) 
{
	xgp->heartbeat_holdoff = 1;
	tmprp->format_string = xgp->format_string;
	tmprp->output = xgp->output;
	tmprp->delimiter = ' ';
	tmprp->flags = RESULTS_HEADER_TAG;
	xdd_results_display(tmprp);
	tmprp->flags = RESULTS_UNITS_TAG;
	xdd_results_display(tmprp);
	if (xgp->csvoutput) {
		tmprp->output = xgp->csvoutput;
		tmprp->delimiter = ',';
		tmprp->flags = RESULTS_HEADER_TAG;
		xdd_results_display(tmprp);
		tmprp->flags = RESULTS_UNITS_TAG;
		xdd_results_display(tmprp);
	}
	xgp->heartbeat_holdoff = 0;
	
} // End of xdd_results_header_display()

/*----------------------------------------------------------------------------*/
// xdd_combine_results() 
// Called by xdd_results_manager() to combine results from a qthread pass 
// results struct into a target_average results struct (the "to" pointer)
// This is different than combining results from multiple qthread pass results
// into a qthread_average results structure mainly in the way the timing
// information is processed. In a target_average results structure the
// timing information for a pass is taken as the earliest start time qthread
// and the latest ending time qthread which makes the overall elapsed time for
// that pass.
//
void 
xdd_combine_results(results_t *to, results_t *from)
{


	// Fundamental Variables
	to->format_string = from->format_string;		// Format String for the xdd_results_display_processor() 
	if (to->flags & RESULTS_COMBINED) {				// For the COMBINED output...
		to->my_target_number = xgp->number_of_targets; 	// This is the number of targets
		to->my_qthread_number = xgp->number_of_iothreads;      // This is the number of qthreads
	} else {										// For a queue or target pass then...
		to->my_target_number = from->my_target_number; 	// Target number of instance 
		to->my_qthread_number = from->my_qthread_number;// QThread number of instance 
	}
	to->queue_depth = from->queue_depth;			// Queue Depth for this target
	to->xfer_size_bytes = from->xfer_size_bytes;	// Transfer size in bytes 
	to->xfer_size_blocks = from->xfer_size_blocks;	// Transfer size in blocks 
	to->xfer_size_kbytes = from->xfer_size_kbytes;	// Transfer size in Kbytes 
	to->xfer_size_mbytes = from->xfer_size_mbytes;	// Transfer size in Mbytes 
	to->reqsize = from->reqsize; 					// RequestSize from the ptds 
	to->pass_number = from->pass_number; 			// Pass number of this set of results 
	to->optype = from->optype;			 			// operation type - read, write, or mixed 

	// Incremented Counters
	to->bytes_xfered += from->bytes_xfered;			// Bytes transfered 
	to->bytes_read += from->bytes_read;				// Bytes transfered during read operations
	to->bytes_written += from->bytes_written;		// Bytes transfered during write operations
	to->op_count += from->op_count;	    			// Operations performed 
	to->read_op_count += from->read_op_count;  		// Read operations performed 
	to->write_op_count += from->write_op_count; 	// Write operations performed 
	to->error_count += from->error_count;  			// Number of I/O errors 

	// Timing Information - calculated from time stamps/values of when things hapened 
	to->accumulated_op_time += from->accumulated_op_time;					// Total Accumulated Time in seconds processing I/O ops 
	to->accumulated_read_op_time += from->accumulated_read_op_time;			// Total Accumulated Time in seconds processing read I/O ops 
	to->accumulated_write_op_time += from->accumulated_write_op_time;		// Total Accumulated Time in seconds processing write I/O ops 
	to->accumulated_pattern_fill_time += from->accumulated_pattern_fill_time;	// Total Accumulated Time in seconds doing pattern fills 
	to->accumulated_flush_time += from->accumulated_flush_time;				// Total Accumulated Time in seconds doing buffer flushes 
	to->elapsed_pass_time_from_first_op += from->elapsed_pass_time_from_first_op; // Total time from start of first op in pass to the end of the last operation 
	to->pass_start_lag_time += from->pass_start_lag_time;		// Lag time from start of pass to the start of the first operation 

	if (to->flags & RESULTS_TARGET_PASS) {
		if (to->earliest_start_time_this_pass > from->earliest_start_time_this_pass)
			to->earliest_start_time_this_pass = from->earliest_start_time_this_pass;
		if (to->latest_end_time_this_pass < from->latest_end_time_this_pass)
			to->latest_end_time_this_pass = from->latest_end_time_this_pass;
		to->elapsed_pass_time = (to->latest_end_time_this_pass - to->earliest_start_time_this_pass)/FLOAT_TRILLION;
		to->accumulated_elapsed_time += to->elapsed_pass_time; 	// Accumulated elapsed time is "virtual" time since the qthreads run in parallel
		to->accumulated_latency += from->latency; 				// This is used to calculate the "average" latency of N qthreads
		to->latency = to->accumulated_latency/(from->my_qthread_number + 1); // This is the "average" latency or milliseconds per op for this target

		// Time spent sending or receiving messages for E2E option 
		to->e2e_sr_time_this_run += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_sr_time_this_pass += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_io_time_this_run += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_io_time_this_pass += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_sr_time_percent_this_pass = (to->e2e_sr_time_this_pass/to->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
		to->e2e_sr_time_percent_this_run = (to->e2e_sr_time_this_run/to->e2e_io_time_this_run)*100.0; // Percentage of IO Time spent in SendReceive

		// CPU Utilization Information >> see user_time, system_time, and us_time below
		to->user_time += from->user_time; 		// Amount of CPU time used by the application for this pass 
		to->system_time += from->system_time;	// Amount of CPU time used by the system for this pass 
		to->us_time += from->us_time; 			// Total CPU time used by this process: user+system time for this pass 
		to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
		to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
		to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 

	} else if (to->flags & RESULTS_TARGET_AVG) {
		if (to->earliest_start_time_this_pass > from->earliest_start_time_this_pass)
			to->earliest_start_time_this_pass = from->earliest_start_time_this_pass;

		if (from->pass_number == 1) 
			to->earliest_start_time_this_run = to->earliest_start_time_this_pass; 

		if (to->latest_end_time_this_pass < from->latest_end_time_this_pass)
			to->latest_end_time_this_pass = from->latest_end_time_this_pass;

		to->latest_end_time_this_run = to->latest_end_time_this_pass;
		to->elapsed_pass_time  += from->elapsed_pass_time;
		to->accumulated_elapsed_time += to->elapsed_pass_time; // Accumulated elapsed time is "virtual" time since the qthreads run in parallel
		to->accumulated_latency += from->latency; 				// This is used to calculate the "average" latency of N qthreads
		to->latency = (to->accumulated_latency/to->queue_depth)/to->pass_number; // This is the "average" latency or milliseconds per op for this target over all passes

		// Time spent sending or receiving messages for E2E option 
		to->e2e_sr_time_this_run += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_sr_time_this_pass += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_io_time_this_run += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_io_time_this_pass += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_sr_time_percent_this_pass = (to->e2e_sr_time_this_pass/to->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
		to->e2e_sr_time_percent_this_run = (to->e2e_sr_time_this_run/to->e2e_io_time_this_run)*100.0; // Percentage of IO Time spent in SendReceive

		// CPU Utilization Information >> see user_time, system_time, and us_time below
		to->user_time += from->user_time; 		// Amount of CPU time used by the application for this pass 
		to->system_time += from->system_time;	// Amount of CPU time used by the system for this pass 
		to->us_time += from->us_time; 			// Total CPU time used by this process: user+system time for this pass 
		to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
		to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
		to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 

	} else if (to->flags & RESULTS_COMBINED) {
		if (to->earliest_start_time_this_run > from->earliest_start_time_this_run)
			to->earliest_start_time_this_run = from->earliest_start_time_this_run;
		if (to->latest_end_time_this_run < from->latest_end_time_this_run)
			to->latest_end_time_this_run = from->latest_end_time_this_run;
		to->elapsed_pass_time = (to->latest_end_time_this_run - to->earliest_start_time_this_run)/FLOAT_TRILLION;
		to->accumulated_elapsed_time += to->elapsed_pass_time; // Accumulated elapsed time is "virtual" time since the qthreads run in parallel
		to->accumulated_latency += from->latency; 				// This is used to calculate the "average" latency of N qthreads
		to->latency = to->accumulated_latency/(from->my_target_number + 1); // This is the "average" latency or milliseconds per op for this target

		// Time spent sending or receiving messages for E2E option  
		to->e2e_sr_time_this_pass += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_sr_time_this_run += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_io_time_this_pass += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_io_time_this_run += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_sr_time_percent_this_pass = (to->e2e_sr_time_this_pass/to->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
		to->e2e_sr_time_percent_this_run = (to->e2e_sr_time_this_run/to->e2e_io_time_this_run)*100.0; // Percentage of IO Time spent in SendReceive

		// CPU Utilization Information >> see user_time, system_time, and us_time below
		to->user_time += from->user_time; 		// Amount of CPU time used by the application for this pass 
		to->system_time += from->system_time;	// Amount of CPU time used by the system for this pass 
		to->us_time += from->us_time; 			// Total CPU time used by this process: user+system time for this pass 
		to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
		to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
		to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 

	} else { // This is the QThread Cumulative Results
		to->elapsed_pass_time += from->elapsed_pass_time; // Total time from start of pass to the end of the last operation 
		to->accumulated_elapsed_time += from->accumulated_elapsed_time; // Total virtual time from start of pass to the end of the last operation 
		to->e2e_io_time_this_pass += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_io_time_this_run += from->e2e_io_time_this_pass; // Total I/O Seconds consumed this pass
		to->e2e_sr_time_this_pass += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_sr_time_this_run += from->e2e_sr_time_this_pass; // SR Time in Seconds
		to->e2e_sr_time_percent_this_pass = (to->e2e_sr_time_this_pass/to->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
		to->e2e_sr_time_percent_this_run = (to->e2e_sr_time_this_run/to->e2e_io_time_this_run)*100.0; // Percentage of IO Time spent in SendReceive
		if (to->op_count == 0.0) 
			to->latency = 0.0;
		else to->latency = (to->elapsed_pass_time / to->op_count) * 1000.0 ;

		// CPU Utilization Information >> see user_time, system_time, and us_time below
		to->user_time += from->user_time; 		// Amount of CPU time used by the application for this pass 
		to->system_time += from->system_time;	// Amount of CPU time used by the system for this pass 
		to->us_time += from->us_time; 			// Total CPU time used by this process: user+system time for this pass 
		to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
		to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
		to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 

	}

	// Averages for the following values are calculated over the number of passes 
	if (to->elapsed_pass_time == 0.0) 
		 to->bandwidth = -1.0;
	else to->bandwidth = (to->bytes_xfered / to->elapsed_pass_time) / FLOAT_MILLION;

	if (to->elapsed_pass_time == 0.0) 
		 to->read_bandwidth = -1.0;
	else to->read_bandwidth = (to->bytes_read / to->elapsed_pass_time) / FLOAT_MILLION ;

	if (to->elapsed_pass_time == 0.0) 
		 to->write_bandwidth = -1.0;
	else to->write_bandwidth = (to->bytes_written / to->elapsed_pass_time) / FLOAT_MILLION ;

	if (to->elapsed_pass_time == 0.0) 
		to->iops = -1.0;
	else to->iops = to->op_count / to->elapsed_pass_time ;

	if (to->elapsed_pass_time == 0.0) 
		to->read_iops = -1.0;
	else to->read_iops = to->read_op_count / to->elapsed_pass_time ;

	if (to->elapsed_pass_time == 0.0) 
		to->write_iops = -1.0;
	else to->write_iops = to->write_op_count / to->elapsed_pass_time ;

	// Other information - only valid when certain options are used
	to->compare_errors += from->compare_errors;		// Number of compare errors on a sequenced data pattern check 
	to->e2e_wait_1st_msg += from->e2e_wait_1st_msg; // Time spent waiting on the destination side of an E2E operation for the first msg 
	to->open_time += from->open_time; 				// Time spent openning the target: end_time-start_time

	// Individual Op bandwidths - Only used when -extendedstats option is specified
	if (to->highest_bandwidth < from->highest_bandwidth) 
		to->highest_bandwidth = from->highest_bandwidth;

	if (to->highest_read_bandwidth < from->highest_read_bandwidth)
		to->highest_read_bandwidth = from->highest_read_bandwidth;

	if (to->highest_write_bandwidth < from->highest_write_bandwidth)
		to->highest_write_bandwidth = from->highest_write_bandwidth;

	if (to->lowest_bandwidth > from->highest_write_bandwidth)
		to->lowest_bandwidth = from->highest_write_bandwidth;

	if (to->lowest_read_bandwidth > from->highest_write_bandwidth)
		to->lowest_read_bandwidth = from->highest_write_bandwidth;

	if (to->lowest_write_bandwidth > from->highest_write_bandwidth)
		to->lowest_write_bandwidth = from->highest_write_bandwidth;


	if (to->highest_iops < from->highest_iops)
		to->highest_iops = from->highest_iops;

	if (to->highest_read_iops < from->highest_iops)
		to->highest_read_iops = from->highest_iops;

	if (to->highest_write_iops < from->highest_iops)
		to->highest_write_iops = from->highest_iops;

	if (to->lowest_iops > from->highest_iops)
		to->lowest_iops = from->highest_iops;

	if (to->lowest_read_iops > from->highest_iops)
		to->lowest_read_iops = from->highest_iops;

	if (to->lowest_write_iops > from->highest_iops)
		to->lowest_write_iops = from->highest_iops;


	if (to->longest_op_time < from->longest_op_time)
		to->longest_op_time = from->longest_op_time;

	if (to->longest_read_op_time < from->longest_op_time)
		to->longest_read_op_time = from->longest_op_time;

	if (to->longest_write_op_time < from->longest_op_time)
		to->longest_write_op_time = from->longest_op_time;

	if (to->shortest_op_time > from->longest_op_time)
		to->shortest_op_time = from->longest_op_time;

	if (to->shortest_read_op_time > from->longest_op_time)
		to->shortest_read_op_time = from->longest_op_time;

	if (to->shortest_write_op_time > from->longest_op_time)
		to->shortest_write_op_time = from->longest_op_time;


	if (to->longest_op_bytes < from->longest_op_bytes)
		to->longest_op_bytes = from->longest_op_bytes;

	if (to->longest_read_op_bytes < from->longest_op_bytes)
		to->longest_read_op_bytes = from->longest_op_bytes;

	if (to->longest_write_op_bytes < from->longest_op_bytes)
		to->longest_write_op_bytes = from->longest_op_bytes;

	if (to->shortest_op_bytes > from->longest_op_bytes)
		to->shortest_op_bytes = from->longest_op_bytes;

	if (to->shortest_read_op_bytes > from->longest_op_bytes)
		to->shortest_read_op_bytes = from->longest_op_bytes;

	if (to->shortest_write_op_bytes > from->longest_op_bytes)
		to->shortest_write_op_bytes = from->longest_op_bytes;


	if (to->longest_op_number < from->longest_op_number)
		to->longest_op_number = from->longest_op_number;

	if (to->longest_read_op_number < from->longest_op_number)
		to->longest_read_op_number = from->longest_op_number;

	if (to->longest_write_op_number < from->longest_op_number)
		to->longest_write_op_number = from->longest_op_number;

	if (to->shortest_op_number > from->shortest_op_number)
		to->shortest_op_number = from->shortest_op_number;

	if (to->shortest_read_op_number > from->shortest_op_number)
		to->shortest_read_op_number = from->shortest_op_number;

	if (to->shortest_write_op_number > from->shortest_op_number)
		to->shortest_write_op_number = from->shortest_op_number;


	if (to->longest_op_pass_number < from->longest_op_pass_number)
		to->longest_op_pass_number = from->longest_op_pass_number;

	if (to->longest_read_op_pass_number < from->longest_op_pass_number)
		to->longest_read_op_pass_number = from->longest_op_pass_number;

	if (to->longest_write_op_pass_number < from->longest_op_pass_number)
		to->longest_write_op_pass_number = from->longest_op_pass_number;

	if (to->shortest_op_pass_number > from->longest_op_pass_number)
		to->shortest_op_pass_number = from->longest_op_pass_number;

	if (to->shortest_read_op_pass_number > from->longest_op_pass_number)
		to->shortest_read_op_pass_number = from->longest_op_pass_number;

	if (to->shortest_write_op_pass_number > from->longest_op_pass_number)
		to->shortest_write_op_pass_number = from->longest_op_pass_number;

} // End of xdd_combine_results()

/*----------------------------------------------------------------------------*/
// xdd_extract_pass_results() 
// This routine will extract the pass results from the specified ptds
// and put them into the specified results structure.
// Called by xdd_process_pass_results() with a pointer to the results structure
// to fill in and a pointer to the qthread PTDS that contains the raw data.
//
void *
xdd_extract_pass_results(results_t *rp, ptds_t *p) 
{

	memset(rp, 0, sizeof(results_t));
	rp->pass_number = p->my_current_pass_number;	// int32
	rp->my_target_number = p->my_target_number;		// int32
	rp->my_qthread_number = p->my_qthread_number;	// int32
	rp->queue_depth = p->queue_depth;				// int32
	rp->bytes_xfered = p->my_current_bytes_xfered;	// int64
	rp->bytes_read = p->my_current_bytes_read;		// int64
	rp->bytes_written = p->my_current_bytes_written;// int64
	rp->op_count = p->my_current_op_count;			// int64
	rp->read_op_count = p->my_current_read_op_count;// int64
	rp->write_op_count = p->my_current_write_op_count;// int64
	rp->error_count = p->my_current_error_count;	// int64
	rp->reqsize = p->reqsize;						// int32

	if (p->rwratio == 0.0) 
		rp->optype = "write";
	else if (p->rwratio == 1.0) 
		rp->optype = "read";
	else rp->optype = "mixed";

	// These next values get converted from raw picoseconds to seconds or miliseconds 
	rp->accumulated_op_time = (double)((double)p->my_accumulated_op_time / FLOAT_TRILLION); // pico to seconds
	rp->accumulated_read_op_time = (double)((double)p->my_accumulated_read_op_time / FLOAT_TRILLION); // pico to seconds
	rp->accumulated_write_op_time = (double)((double)p->my_accumulated_write_op_time / FLOAT_TRILLION); // pico to seconds
	rp->accumulated_pattern_fill_time = (double)((double)p->my_accumulated_pattern_fill_time / FLOAT_BILLION); // pico to milli
	rp->accumulated_flush_time = (double)((double)p->my_accumulated_flush_time / FLOAT_BILLION); // pico to milli
	rp->earliest_start_time_this_run = (double)p->first_pass_start_time; // picoseconds
	rp->earliest_start_time_this_pass = (double)p->my_pass_start_time; // picoseconds
	rp->latest_end_time_this_run = (double)p->my_pass_end_time; // picoseconds
	rp->latest_end_time_this_pass = (double)p->my_pass_end_time; // picoseconds
	rp->elapsed_pass_time = (double)((double)(p->my_pass_end_time - p->my_pass_start_time) / FLOAT_TRILLION); // pico to seconds
	if (rp->elapsed_pass_time == 0.0) 
		rp->elapsed_pass_time = -1.0; // This is done to prevent and divide-by-zero problems
	rp->accumulated_elapsed_time += rp->elapsed_pass_time; 

	// These get calculated here
	rp->xfer_size_bytes = p->iosize;				// bytes
	rp->xfer_size_blocks = p->iosize/p->block_size;	// blocks
	rp->xfer_size_kbytes = p->iosize/1024;			// kbytes
	rp->xfer_size_mbytes = p->iosize/(1024*1024);	// mbytes
	rp->bandwidth = (double)(((double)rp->bytes_xfered / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec
	rp->read_bandwidth = (double)(((double)rp->bytes_read / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec
	rp->write_bandwidth = (double)(((double)rp->bytes_written / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec
	rp->iops = (double)((double)rp->op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	rp->read_iops = (double)(rp->read_op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	rp->write_iops = (double)(rp->write_op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	if (rp->iops == 0.0)
		rp->latency = 0.0;
	else rp->latency = (double)((1.0/rp->iops) * 1000.0);  // milliseconds
	rp->user_time =   (double)(p->my_current_cpu_times.tms_utime  - p->my_starting_cpu_times_this_pass.tms_utime)/(double)(xgp->clock_tick); // Seconds
	rp->system_time = (double)(p->my_current_cpu_times.tms_stime  - p->my_starting_cpu_times_this_pass.tms_stime)/(double)(xgp->clock_tick); // Seconds
	rp->us_time = (double)(rp->user_time + rp->system_time); // MilliSeconds
	if (rp->elapsed_pass_time == 0.0) {
		rp->percent_user = 0.0;
		rp->percent_system = 0.0;
		rp->percent_cpu = 0.0;
	} else {
		rp->percent_user = (double)(rp->user_time/rp->elapsed_pass_time) * 100.0; // Percent User Time
		rp->percent_system = (double)(rp->system_time/rp->elapsed_pass_time) * 100.0; // Percent System Time
		rp->percent_cpu = (rp->us_time / rp->elapsed_pass_time) * 100.0;
	}
	rp->e2e_sr_time_this_pass = (double)p->e2e_sr_time/FLOAT_TRILLION; // E2E SendReceive Time in MicroSeconds
	rp->e2e_io_time_this_pass = (double)rp->elapsed_pass_time; // E2E IO  Time in MicroSeconds
	if (rp->e2e_io_time_this_pass == 0.0)
		rp->e2e_sr_time_percent_this_pass = 0.0; // Percentage of IO Time spent in SendReceive
	else rp->e2e_sr_time_percent_this_pass = (rp->e2e_sr_time_this_pass/rp->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
	rp->e2e_wait_1st_msg = (double)p->e2e_wait_1st_msg/FLOAT_TRILLION; // MicroSeconds
	// The Hig/Low values are only updated when the -extendedstats option is specified
	if (xgp->global_options & GO_EXTENDED_STATS) {
		if (rp->shortest_op_time > 0.0) {
			rp->highest_bandwidth = rp->shortest_op_bytes / rp->shortest_op_time;
			rp->highest_iops = 1.0 / rp->shortest_op_time;
		} else {
			rp->highest_bandwidth = -1.0;
			rp->highest_iops = -1.0;
		}
		if (rp->shortest_read_op_time > 0.0) {
			rp->highest_read_bandwidth = rp->shortest_read_op_bytes / rp->shortest_read_op_time;
			rp->highest_read_iops = 1.0 / rp->shortest_read_op_time;
		} else {
			rp->highest_read_bandwidth = -1.0;
			rp->highest_read_iops = -1.0;
		}
		if (rp->shortest_write_op_time > 0.0) {
			rp->highest_write_bandwidth = rp->shortest_write_op_bytes / rp->shortest_write_op_time;
			rp->highest_write_iops = 1.0 / rp->shortest_write_op_time;
		} else {
			rp->highest_write_bandwidth = -1.0;
			rp->highest_write_iops = -1.0;
		}
		if (rp->longest_op_time > 0.0) {
			rp->lowest_bandwidth = rp->longest_op_bytes / rp->longest_op_time;
			rp->lowest_iops = 1.0 / rp->longest_op_time;
		} else {
			rp->lowest_bandwidth = -1.0;
			rp->lowest_iops = -1.0;
		}

		if (rp->longest_read_op_time > 0.0) {
			rp->lowest_read_bandwidth = rp->longest_read_op_bytes / rp->longest_read_op_time;
			rp->lowest_read_iops = 1.0 / rp->longest_read_op_time;
		} else {
			rp->lowest_read_bandwidth = -1.0;
			rp->lowest_read_iops = -1.0;
		}

		if (rp->longest_write_op_time > 0.0) {
			rp->lowest_write_bandwidth = rp->longest_write_op_bytes / rp->longest_write_op_time;
			rp->lowest_write_iops = 1.0 / rp->longest_write_op_time;
		} else {
			rp->lowest_write_bandwidth = -1.0;
			rp->lowest_write_iops = -1.0;
		}
	}
	return;
} // End of xdd_extract_pass_results() 
/*----------------------------------------------------------------------------*/
// xdd_process_pass_results() 
// Calculate results for each qthread
// Called by xdd_results_manager() with a pointer to the qthread PTDS
//
void *
xdd_process_pass_results(void) 
{
	int			i;
	ptds_t		*p;		// PTDS pointer
	results_t	*qarp;	// Pointer to the Qthread Average results structure
	results_t	*tarp;	// Pointer to the Target Average results structure
	results_t	*trp;	// Pointer to the Temporary Target Pass results
	results_t	*qrp;	// Pointer to the temporary qthread pass results
	results_t	qthread_pass_results; // Temporary qthread pass results
	results_t	target_pass_results; // Temporary for target pass results

	xgp->heartbeat_holdoff = 1;

	qrp = &qthread_pass_results;
	trp = &target_pass_results;

	// Next, gather results from all target qthreads and display pass results
	for (i=0; i<xgp->number_of_targets; i++) {
		p = xgp->ptdsp[i]; /* Get the ptds for this target */
		// Init the Target Average Results struct
		tarp = xgp->target_average_resultsp[i];
		tarp->format_string = xgp->format_string;
		tarp->what = "TARGET_AVERAGE";
		if (p->my_current_pass_number == 1) {
			tarp->earliest_start_time_this_run = (double)DOUBLE_MAX;
			tarp->earliest_start_time_this_pass = (double)DOUBLE_MAX;
			tarp->shortest_op_time = (double)DOUBLE_MAX;
			tarp->shortest_read_op_time = (double)DOUBLE_MAX;
			tarp->shortest_write_op_time = (double)DOUBLE_MAX;
		}
		
		// Init the Target Pass Results struct
		trp = &target_pass_results;
		memset(trp, 0, sizeof(results_t));
		trp->format_string = xgp->format_string;
		trp->what = "TARGET_PASS   ";
		trp->earliest_start_time_this_run = (double)DOUBLE_MAX;
		trp->earliest_start_time_this_pass = (double)DOUBLE_MAX;
		trp->shortest_op_time = (double)DOUBLE_MAX;
		trp->shortest_read_op_time = (double)DOUBLE_MAX;
		trp->shortest_write_op_time = (double)DOUBLE_MAX;
		trp->e2e_sr_time_this_pass = 0;
		trp->e2e_io_time_this_pass = 0;
		while (p) { /* Look at all the qthreads for this target */
			qrp = &qthread_pass_results;

			xdd_extract_pass_results(qrp, p);
	
			// Display the QThread pass results
			qrp->flags = (RESULTS_PASS_INFO | RESULTS_QUEUE_PASS);
			qrp->format_string = xgp->format_string;
			qrp->what = "QUEUE_PASS    ";
			qrp->output = xgp->output;
			qrp->delimiter = ' ';
			if (p->target_options & TO_QTHREAD_INFO) {
				xdd_results_display(qrp);
				if (xgp->csvoutput) { // Display to CSV file if requested
					qrp->output = xgp->csvoutput;
					qrp->delimiter = ',';
					xdd_results_display(qrp);
				}
			}
			
			// Combine qthread results with prior QThread Average Results
			qarp = &p->qthread_average_results;
			qarp->format_string = xgp->format_string;
			qarp->what = "QUEUE_AVERAGE";
			xdd_combine_results(qarp, qrp);

			// Combine qthread results for Target Average
			trp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_PASS);
			xdd_combine_results(trp, qrp);

			// get ptds for the next qthread for this target 
			p = p->nextp; 
		} // Done with results for this target

		// Combine the pass results from this target with its average results so far
		tarp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_AVG);
		xdd_combine_results(tarp, trp);

		// At this point all the pass results from each qthread for this particular
		// target have been squished into a single results structure for this target.
		// Display the Pass Results for this target
		trp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_PASS);
		trp->what = "TARGET_PASS   ";
		trp->output = xgp->output;
		trp->delimiter = ' ';
		if (xgp->global_options & GO_VERBOSE) {
			xdd_results_display(trp);
			if (xgp->csvoutput) { // Display to CSV file if requested
				trp->output = xgp->csvoutput;
				trp->delimiter = ',';
				xdd_results_display(trp);
			}
		}
			
	} /* end of FOR loop that looks at all targets */

	xgp->heartbeat_holdoff = 0;

	return;
} // End of xdd_process_pass_results() 

/*----------------------------------------------------------------------------*/
// xdd_process_run_results() 
// Calculate results for each qthread
// Called by xdd_results_manager() with a pointer to the qthread PTDS
//
void *
xdd_process_run_results(void) 
{
	int			i;	
	ptds_t		*p;		// PTDS pointer
	ptds_t		*qp;	// PTDS pointer to a qthread ptds
	results_t	*tarp;	// Pointer to the Target Average results structure
	results_t	*crp;	// Pointer to the temporary combined run results
	results_t	combined_results; // Temporary for target combined results


	// Wait for the other threads to get here...
	xdd_barrier(&xgp->results_run_barrier);

	// At this point all the other threads are waiting for this routine
	// to display the *average* results and combined results for this run.
	xgp->heartbeat_holdoff = 2; // Tell the heartbeat to stop

	crp = &combined_results; // A place to accumulate the combined results
	memset(crp, 0, sizeof(results_t));
	crp->flags = (RESULTS_PASS_INFO | RESULTS_COMBINED);
	crp->format_string = xgp->format_string;
	crp->what = "COMBINED      ";
	crp->earliest_start_time_this_run = (double)DOUBLE_MAX;
	crp->shortest_op_time = (double)DOUBLE_MAX;
	crp->shortest_read_op_time = (double)DOUBLE_MAX;
	crp->shortest_write_op_time = (double)DOUBLE_MAX;

	for (i=0; i<xgp->number_of_targets; i++) {
		p = xgp->ptdsp[i]; /* Get the ptds for this target */
		tarp = xgp->target_average_resultsp[i];
			
		tarp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_AVG);
		tarp->format_string = xgp->format_string;
		tarp->what = "TARGET_AVERAGE";
		tarp->output = xgp->output;
		tarp->delimiter = ' ';
		if (xgp->global_options & GO_VERBOSE) {
			xdd_results_display(tarp);
			if (xgp->csvoutput) { // Display to CSV file if requested
				tarp->output = xgp->csvoutput;
				tarp->delimiter = ',';
				xdd_results_display(tarp);
			}
		}

		xdd_combine_results(crp, tarp);

	} // End of FOR loop that processes all targets for the run
	crp->output = xgp->output;
	crp->delimiter = ' ';
	xdd_results_display(crp);
	if (xgp->csvoutput) { // Display to CSV file if requested
		crp->output = xgp->csvoutput;
		crp->delimiter = ',';
		xdd_results_display(crp);
	}

	for (i=0; i<xgp->number_of_targets; i++) { // Processing TimeStamp reports
		p = xgp->ptdsp[i]; /* Get the ptds for this target */
		qp = p;
		while (qp) {
			/* Display and write the time stamping information if requested */
			if (qp->ts_options & (TS_ON | TS_TRIGGERED)) {
				if (qp->timestamps > qp->ttp->tt_size) 
					qp->ttp->numents = qp->ttp->tt_size;
				else qp->ttp->numents = qp->timestamps;
				if (qp->run_complete == 0) {
					fprintf(xgp->errout,"%s: Houston, we have a problem... thread for Target %d Q %d is not complete and we are trying to dump the time stamp file!\n", 
						xgp->progname, 
						qp->my_target_number, 
						qp->my_qthread_number);
				}
				xdd_ts_reports(qp);  /* generate reports if requested */
				xdd_ts_write(qp); 
				xdd_ts_cleanup(qp->ttp); /* call this to free the TS table in memory */
			}
			qp = qp->nextp;
		}
	} // End of processing TimeStamp reports
	// Release all the other qthreads so that they can do their cleanup and exit
	xdd_barrier(&xgp->results_display_final_barrier);
	return;
} // End of xdd_process_run_results() 

/*----------------------------------------------------------------------------*/
// xdd_results_manager runs as a separate thread.
// When started, this thread will initialize itself and enter the 
// "results_barrier" and wait for all the other target qthreads to enter
// the "results_barrier". 
// When the other threads enter the "results_barrier", this thread will be
// released and will gather results information from all the qthreads
// and will display the appropriate information.
// All of the target threads will have re-enterd the "results_barrier" 
// waiting for the results_manager to process and display the results.
// After the results have been processed and displayed, the results_manager
// will re-enter the "results_barrier" and release all the qthreads to continue
// with the next pass or termination. Re-entering the "results_barrier" will
// cause the results_manager to fall through as well at which time it will
// re-enter the primary waiting barrier.
//
void *
xdd_results_manager(void *n)
{
	int32_t  	i;  							/* Random variables */
	int32_t  	results_pass_barrier_index;		/* index for results pass barrier */
	int32_t  	results_display_barrier_index;  /* index for results display barrier */
	ptds_t  	*p;   							/* Pointer to a qthread ptds */
	results_t	*rp;							// Pointer to the results struct in each qthread PTDS
	results_t	*tmprp;							// Pointer to the results struct in each qthread PTDS


	results_pass_barrier_index = 0;
	results_display_barrier_index = 0;

	tmprp = (results_t *)valloc(sizeof(results_t));
	if (tmprp == NULL) {
			fprintf(xgp->errout, "%s: results_manager: Error: Cannot allocate memory for a temp results structure!\n", xgp->progname);
			perror("results_manager: Reason");
			return;
	}

	// Enter this barrier and wait for the results monitor to initialize
	xdd_barrier(&xgp->results_initialization_barrier);

	// This is the loop that runs continuously throughout the xdd run
	while (1) {
		/* Enter the results barrier and wait for the target threads to get here */
		xdd_barrier(&xgp->results_pass_barrier[results_pass_barrier_index]);
		results_pass_barrier_index ^= 1; 

		if ( xgp->abort_io == 1) { // Something went wrong during thread initialization so let's just leave
			xdd_process_run_results();
			return;
		}

		// Display the header and nuits line if necessary
		if (xgp->results_header_displayed == 0) {
			xdd_results_header_display(tmprp);
			xgp->results_header_displayed = 1;
		}

		// At this point all the target qthreads should have entered the results_display_barrier
		// or the results_display_fnial_barrier and will wait for the pass or run results to be displayed

		if (xgp->run_complete) {
			xdd_process_run_results();
			return;
		} else { 
			 xdd_process_pass_results();
		}

		xdd_barrier(&xgp->results_display_barrier[results_display_barrier_index]);
		results_display_barrier_index ^= 1; 

	} // End of main WHILE loop for the results_manager()
}

/*----------------------------------------------------------------------------*/
// xdd_results_dump 
void *
xdd_results_dump(results_t *rp, char *dumptype)
{

	fprintf(xgp->output,"\n\n------------------------ Dumping %s -------------------------------\n\n", dumptype);
	fprintf(xgp->output,"	flags = 0x%016x\n",rp->flags);				// Flags that tell the display function what to display
	fprintf(xgp->output,"	*what = '%s'\n",rp->what);					// The type of information line to display - Queue Pass, Target Pass, Queue Avg, Target Avg, Combined
	fprintf(xgp->output,"	*output = 0x%016x\n",rp->output);			// This points to the output file 
	fprintf(xgp->output,"	delimiter = 0x%1x\n",rp->delimiter);		// The delimiter to use between fields - i.e. a space or tab or comma

	// Fundamental Variables
	fprintf(xgp->output,"	*format_string = '%s'\n",rp->format_string);		// Format String for the xdd_results_display_processor() 
	fprintf(xgp->output,"	my_target_number = %d\n",rp->my_target_number); 	// Target number of instance 
	fprintf(xgp->output,"	my_qthread_number = %d\n",rp->my_qthread_number);	// Qthread number of this instance 
	fprintf(xgp->output,"	queue_depth = %d\n",rp->queue_depth);		 	// The queue depth of this target
	fprintf(xgp->output,"	xfer_size_bytes = %12.0f\n",rp->xfer_size_bytes);		// Transfer size in bytes 
	fprintf(xgp->output,"	xfer_size_blocks = %12.3f\n",rp->xfer_size_blocks);		// Transfer size in blocks 
	fprintf(xgp->output,"	xfer_size_kbytes = %12.3f\n",rp->xfer_size_kbytes);		// Transfer size in Kbytes 
	fprintf(xgp->output,"	xfer_size_mbytes = %12.3f\n",rp->xfer_size_mbytes);		// Transfer size in Mbytes 
	fprintf(xgp->output,"	reqsize = %d\n",rp->reqsize); 			// RequestSize from the ptds 
	fprintf(xgp->output,"	pass_number = %d\n",rp->pass_number); 	// Pass number of this set of results 
	fprintf(xgp->output,"	*optype = '%s'\n",rp->optype);			// Operation type - read, write, or mixed

	// Incremented Counters
	fprintf(xgp->output,"	bytes_xfered = %lld\n",rp->bytes_xfered);		// Bytes transfered 
	fprintf(xgp->output,"	bytes_read = %lld\n",rp->bytes_read);			// Bytes transfered during read operations
	fprintf(xgp->output,"	bytes_written = %lld\n",rp->bytes_written);		// Bytes transfered during write operations
	fprintf(xgp->output,"	op_count = %lld\n",rp->op_count);    			// Operations performed 
	fprintf(xgp->output,"	read_op_count = %lld\n",rp->read_op_count);		// Read operations performed 
	fprintf(xgp->output,"	write_op_count = %lld\n",rp->write_op_count); 	// Write operations performed 
	fprintf(xgp->output,"	error_count = %lld\n",rp->error_count);  		// Number of I/O errors 

	// Timing Information - calculated from time stamps/values of when things hapened 
	fprintf(xgp->output,"	accumulated_op_time = %8.3f\n",rp->accumulated_op_time);		// Total Accumulated Time in seconds processing I/O ops 
	fprintf(xgp->output,"	accumulated_read_op_time = %8.3f\n",rp->accumulated_read_op_time);	// Total Accumulated Time in seconds processing read I/O ops 
	fprintf(xgp->output,"	accumulated_write_op_time = %8.3f\n",rp->accumulated_write_op_time);	// Total Accumulated Time in seconds processing write I/O ops 
	fprintf(xgp->output,"	accumulated_pattern_fill_time = %8.3f\n",rp->accumulated_pattern_fill_time);	// Total Accumulated Time in seconds doing pattern fills 
	fprintf(xgp->output,"	accumulated_flush_time = %8.3f\n",rp->accumulated_flush_time);	// Total Accumulated Time in seconds doing buffer flushes 
	fprintf(xgp->output,"	earliest_start_time_this_run = %8.3f\n",rp->earliest_start_time_this_run);	// usec, For a qthread this is simply the start time of pass 1, for a target it is the earliest start time of all threads
	fprintf(xgp->output,"	earliest_start_time_this_pass = %8.3f\n",rp->earliest_start_time_this_pass);	// usec, For a qthread this is simply the start time of pass 1, for a target it is the earliest start time of all threads
	fprintf(xgp->output,"	latest_end_time_this_run = %8.3f\n",rp->latest_end_time_this_run);			// usec, For a qthread this is the end time of the last pass, for a target it is the latest end time of all qthreads
	fprintf(xgp->output,"	latest_end_time_this_pass = %8.3f\n",rp->latest_end_time_this_pass);			// usec, For a qthread this is the end time of the last pass, for a target it is the latest end time of all qthreads
	fprintf(xgp->output,"	elapsed_pass_time = %8.3f\n",rp->elapsed_pass_time);		// usec, Total time from start of pass to the end of the last operation 
	fprintf(xgp->output,"	elapsed_pass_time_from_first_op = %8.3f\n",rp->elapsed_pass_time_from_first_op); // usec, Total time from start of first op in pass to the end of the last operation 
	fprintf(xgp->output,"	pass_start_lag_time = %8.3f\n",rp->pass_start_lag_time); 	// usec, Lag time from start of pass to the start of the first operation 
	fprintf(xgp->output,"	bandwidth = %8.3f\n",rp->bandwidth);				// Measured data rate in MB/sec from start of first op to end of last op
	fprintf(xgp->output,"	read_bandwidth = %8.3f\n",rp->read_bandwidth);			// Measured read data rate in MB/sec  from start of first op to end of last op
	fprintf(xgp->output,"	write_bandwidth = %8.3f\n",rp->write_bandwidth);		// Measured write data rate in MB/sec from start of first op to end of last op 
	fprintf(xgp->output,"	iops = %8.3f\n",rp->iops);					// Measured I/O Operations per second from start of first op to end of last op 
	fprintf(xgp->output,"	read_iops = %8.3f\n",rp->read_iops);				// Measured read I/O Operations per second from start of first op to end of last op 
	fprintf(xgp->output,"	write_iops = %8.3f\n",rp->write_iops);				// Measured write I/O Operations per second from start of first op to end of last op 
	fprintf(xgp->output,"	latency = %8.3f\n",rp->latency); 				// Latency in milliseconds 

	// CPU Utilization Information >> see user_time, system_time, and us_time below
	fprintf(xgp->output,"	user_time = %8.3f\n",rp->user_time); 				// Amount of CPU time used by the application for this pass 
	fprintf(xgp->output,"	system_time = %8.3f\n",rp->system_time); 			// Amount of CPU time used by the system for this pass 
	fprintf(xgp->output,"	us_time = %8.3f\n",rp->us_time); 				// Total CPU time used by this process: user+system time for this pass 
	fprintf(xgp->output,"	percent_user = %8.3f\n",rp->percent_user); 			// Percent of User CPU used by this process 
	fprintf(xgp->output,"	percent_system = %8.3f\n",rp->percent_system); 		// Percent of SYSTEM CPU used by this process 
	fprintf(xgp->output,"	percent_cpu = %8.3f\n",rp->percent_cpu); 		// Percent of CPU used by this process 

	// Other information - only valid when certain options are used
	fprintf(xgp->output,"	compare_errors = %lld\n",rp->compare_errors);			// Number of compare errors on a sequenced data pattern check 
	fprintf(xgp->output,"	e2e_io_time_this_pass = %8.3f\n",rp->e2e_io_time_this_pass); 			// Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_sr_time_this_pass = %8.3f\n",rp->e2e_sr_time_this_pass); 			// Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_sr_time_percent_this_pass = %8.3f\n",rp->e2e_sr_time_percent_this_pass); 	// Percentage of total Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_io_time_this_pass = %8.3f\n",rp->e2e_io_time_this_run); 			// Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_sr_time_this_pass = %8.3f\n",rp->e2e_sr_time_this_run); 			// Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_sr_time_percent_this_pass = %8.3f\n",rp->e2e_sr_time_percent_this_run); 	// Percentage of total Time spent sending or receiving messages for E2E option 
	fprintf(xgp->output,"	e2e_wait_1st_msg = %8.3f\n",rp->e2e_wait_1st_msg);    	// Time spent waiting on the destination side of an E2E operation for the first msg 
	fprintf(xgp->output,"	open_start_time = %8.3f\n",rp->open_start_time); 		// Time stamp at the beginning of openning the target
	fprintf(xgp->output,"	open_end_time = %8.3f\n",rp->open_end_time); 			// Time stamp after openning the target 
	fprintf(xgp->output,"	open_time = %8.3f\n",rp->open_time); 				// Time spent openning the target: end_time-start_time

	// Individual Op bandwidths - Only used when -extendedstats option is specified
	fprintf(xgp->output,"	longest_op_time = %12.3f\n",rp->longest_op_time); 		// Longest op time that occured during this pass
	fprintf(xgp->output,"	longest_read_op_time = %12.3f\n",rp->longest_read_op_time); 	// Longest read op time that occured during this pass
	fprintf(xgp->output,"	longest_write_op_time = %12.3f\n",rp->longest_write_op_time); 	// Longest write op time that occured during this pass
	fprintf(xgp->output,"	shortest_op_time = %12.3f\n",rp->shortest_op_time); 		// Shortest op time that occurred during this pass
	fprintf(xgp->output,"	shortest_read_op_time = %12.3f\n",rp->shortest_read_op_time); 	// Shortest read op time that occured during this pass
	fprintf(xgp->output,"	shortest_write_op_time = %12.3f\n",rp->shortest_write_op_time); // Shortest write op time that occured during this pass

	fprintf(xgp->output,"	longest_op_bytes = %lld\n",rp->longest_op_bytes); 			// Bytes xfered when the longest op time occured during this pass
	fprintf(xgp->output," 	longest_read_op_bytes = %lld\n",rp->longest_read_op_bytes);	 	// Bytes xfered when the longest read op time occured during this pass
	fprintf(xgp->output," 	longest_write_op_bytes = %lld\n",rp->longest_write_op_bytes); 	// Bytes xfered when the longest write op time occured during this pass
	fprintf(xgp->output," 	shortest_op_bytes = %lld\n",rp->shortest_op_bytes); 			// Bytes xfered when the shortest op time occured during this pass
	fprintf(xgp->output," 	shortest_read_op_bytes = %lld\n",rp->shortest_read_op_bytes); 	// Bytes xfered when the shortest read op time occured during this pass
	fprintf(xgp->output," 	shortest_write_op_bytes = %lld\n",rp->shortest_write_op_bytes);	// Bytes xfered when the shortest write op time occured during this pass

	fprintf(xgp->output,"	longest_op_number = %lld\n",rp->longest_op_number); 			// Operation Number when the longest op time occured during this pass
	fprintf(xgp->output," 	longest_read_op_number = %lld\n",rp->longest_read_op_number); 	// Operation Number when the longest read op time occured during this pass
	fprintf(xgp->output," 	longest_write_op_number = %lld\n",rp->longest_write_op_number); 	// Operation Number when the longest write op time occured during this pass
	fprintf(xgp->output," 	shortest_op_number = %lld\n",rp->shortest_op_number); 		// Operation Number when the shortest op time occured during this pass
	fprintf(xgp->output," 	shortest_read_op_number = %lld\n",rp->shortest_read_op_number); 	// Operation Number when the shortest read op time occured during this pass
	fprintf(xgp->output," 	shortest_write_op_number = %lld\n",rp->shortest_write_op_number);	// Operation Number when the shortest write op time occured during this pass

	fprintf(xgp->output,"	longest_op_pass_number = %lld\n",rp->longest_op_pass_number);		// Pass Number when the longest op time occured during this pass
	fprintf(xgp->output,"	longest_read_op_pass_number = %lld\n",rp->longest_read_op_pass_number);// Pass Number when the longest read op time occured
	fprintf(xgp->output,"	longest_write_op_pass_number = %lld\n",rp->longest_write_op_pass_number);// Pass Number when the longest write op time occured 
	fprintf(xgp->output,"	shortest_op_pass_number = %lld\n",rp->shortest_op_pass_number);	// Pass Number when the shortest op time occured 
	fprintf(xgp->output,"	shortest_read_op_pass_number = %lld\n",rp->shortest_read_op_pass_number);// Pass Number when the shortest read op time occured 
	fprintf(xgp->output,"	shortest_write_op_pass_number = %lld\n",rp->shortest_write_op_pass_number);// Pass Number when the shortest write op time occured 

	fprintf(xgp->output,"	highest_bandwidth = %12.3f\n",rp->highest_bandwidth);		// Highest individual op data rate in MB/sec
	fprintf(xgp->output,"	highest_read_bandwidth = %12.3f\n",rp->highest_read_bandwidth);	// Highest individual op read data rate in MB/sec 
	fprintf(xgp->output,"	highest_write_bandwidth = %12.3f\n",rp->highest_write_bandwidth);// Highest individual op write data rate in MB/sec 
	fprintf(xgp->output,"	lowest_bandwidth = %12.3f\n",rp->lowest_bandwidth);		// Lowest individual op data rate in MB/sec
	fprintf(xgp->output,"	lowest_read_bandwidth = %12.3f\n",rp->lowest_read_bandwidth);	// Lowest individual op read data rate in MB/sec 
	fprintf(xgp->output,"	lowest_write_bandwidth = %12.3f\n",rp->lowest_write_bandwidth);	// Lowest individual op write data rate in MB/sec 

	fprintf(xgp->output,"	highest_iops = %12.3f\n",rp->highest_iops);			// Highest individual op I/O Operations per second 
	fprintf(xgp->output,"	highest_read_iops = %12.3f\n",rp->highest_read_iops);		// Highest individual op read I/O Operations per second 
	fprintf(xgp->output,"	highest_write_iops = %12.3f\n",rp->highest_write_iops);		// Highest individual op write I/O Operations per second 
	fprintf(xgp->output,"	lowest_iops = %12.3f\n",rp->lowest_iops);			// Lowest individual op I/O Operations per second 
	fprintf(xgp->output,"	lowest_read_iops = %12.3f\n",rp->lowest_read_iops);		// Lowest individual op read I/O Operations per second 
	fprintf(xgp->output,"	lowest_write_iops = %12.3f\n",rp->lowest_write_iops);		// Lowest individual op write I/O Operations per second 

	fprintf(xgp->output,"==================================================================\n");
}
