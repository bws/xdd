/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
#ifndef RESULTS_H
#define RESULTS_H

/* results structure for multiple processes */
// Results Flags definitions
#define RESULTS_HEADER_TAG	0x0000000000000001		// Display the Header Tag
#define RESULTS_UNITS_TAG	0x0000000000000002		// Display the Units Tag
#define RESULTS_AVAILABLE	0x0000000000000004		// Display the Value of the variable
#define RESULTS_PASS_INFO	0x0000000000000008		// Pass Information
#define RESULTS_RUN_INFO	0x0000000000000010		// Run information
#define RESULTS_QUEUE_PASS	0x0000000000000020		// Pass information for a Queue Thread
#define RESULTS_TARGET_PASS	0x0000000000000040		// Pass information for a Target
#define RESULTS_QUEUE_AVG	0x0000000000000080		// Average information for each queue thread over all passes
#define RESULTS_TARGET_AVG	0x0000000000000100		// Average information for each target over all passes
#define RESULTS_COMBINED	0x0000000000000200		// Run information for all Targets over all passes

struct results {
	uint64_t	flags;					// Flags that tell the display function what to display
	char		*what;					// The type of information line to display - Queue Pass, Target Pass, Queue Avg, Target Avg, Combined
	FILE		*output;				// This points to the output file 
	char		delimiter;				// The delimiter to use between fields - i.e. a space or tab or comma

	// Fundamental Variables
	char		*format_string;			// Format String for the xdd_results_display_processor() 
	int32_t		my_target_number; 		// Target number of instance 
	int32_t		my_worker_thread_number; 		// Qthread number of this instance 
	int32_t		queue_depth;		 	// The queue depth of this target
	double		xfer_size_bytes;		// Transfer size in bytes 
	double		xfer_size_blocks;		// Transfer size in blocks 
	double		xfer_size_kbytes;		// Transfer size in Kbytes 
	double		xfer_size_mbytes;		// Transfer size in Mbytes 
	int32_t		reqsize; 				// RequestSize from the Target Data Struct 
	int32_t		pass_number; 			// Pass number of this set of results 
	char		*optype;	 			// operation type - read, write, or mixed 

	// Incremented Counters
	int64_t		bytes_xfered;			// Bytes transfered 
	int64_t		bytes_read;				// Bytes transfered during read operations
	int64_t		bytes_written;			// Bytes transfered during write operations
	int64_t		op_count;    			// Operations performed 
	int64_t		read_op_count;  		// Read operations performed 
	int64_t		write_op_count; 		// Write operations performed 
	int64_t		error_count;  			// Number of I/O errors 

	// Timing Information - calculated from time stamps/values of when things hapened 
	double		accumulated_op_time;		// Total Accumulated Time in seconds processing I/O ops 
	double		accumulated_read_op_time;	// Total Accumulated Time in seconds processing read I/O ops 
	double		accumulated_write_op_time;	// Total Accumulated Time in seconds processing write I/O ops 
	double		accumulated_pattern_fill_time;// Total Accumulated Time in seconds doing pattern fills 
	double		accumulated_flush_time;		// Total Accumulated Time in seconds doing buffer flushes 
	double		accumulated_elapsed_time;	// Total Accumulated Time in seconds for all "elapsed" times 
	double		accumulated_latency;		// Total Accumulated Latency Used to calculate average latency
	double		earliest_start_time_this_run;	// The earliest recorded start time for any worker_thread in pass 1 of the run
	double		earliest_start_time_this_pass;	// The earliest recorded start time for any worker_thread in a given pass
	double		pass_delay_accumulated_time;	// The accumulated time from inter-pass delay (from the -passdelay option)
	double		latest_end_time_this_run;		// For a worker_thread this is the end time of the last pass, for a target it is the latest end time of all worker_threads
	double		latest_end_time_this_pass;		// For a worker_thread this is the end time of the last pass, for a target it is the latest end time of all worker_threads
	double		elapsed_pass_time;		// usec, Total time from start of pass to the end of the last operation 
	double		elapsed_pass_time_from_first_op; // usec, Total time from start of first op in pass to the end of the last operation 
	double		pass_start_lag_time; 	// usec, Lag time from start of pass to the start of the first operation 
	double		bandwidth;				// Measured data rate in MB/sec from start of first op to end of last op
	double		read_bandwidth;			// Measured read data rate in MB/sec  from start of first op to end of last op
	double		write_bandwidth;		// Measured write data rate in MB/sec from start of first op to end of last op 
	double		iops;					// Measured I/O Operations per second from start of first op to end of last op 
	double		read_iops;				// Measured read I/O Operations per second from start of first op to end of last op 
	double		write_iops;				// Measured write I/O Operations per second from start of first op to end of last op 
	double		latency; 				// Latency in milliseconds 

	// CPU Utilization Information >> see user_time, system_time, and us_time below
	double		user_time; 				// Amount of CPU time used by the application for this pass 
	double		system_time; 			// Amount of CPU time used by the system for this pass 
	double		us_time; 				// Total CPU time used by this process: user+system time for this pass 
	double		percent_user; 			// Percent of User CPU used by this process 
	double		percent_system; 		// Percent of SYSTEM CPU used by this process 
	double		percent_cpu; 			// Percent of CPU used by this process  -> us_time / elapsed_time

	// Other information - only valid when certain options are used
	int64_t		compare_errors;			// Number of compare errors on a sequenced data pattern check 
	double		e2e_wait_1st_msg;    		// Time spent waiting on the destination side of an E2E operation for first msg 
	double		e2e_io_time_this_pass; 		// Total I/O time for this pass
	double		e2e_sr_time_this_pass; 		// Time spent sending or receiving messages for E2E option 
	double		e2e_sr_time_percent_this_pass; 	// Percentage of total Time spent sending or receiving messages for E2E option 
	double		e2e_io_time_this_run; 		// Total I/O time for this run
	double		e2e_sr_time_this_run; 		// Time spent sending or receiving messages for E2E option 
	double		e2e_sr_time_percent_this_run; 	// Percentage of total Time spent sending or receiving messages for E2E option 
	double		open_start_time; 		// Time stamp at the beginning of openning the target
	double		open_end_time; 			// Time stamp after openning the target 
	double		open_time; 				// Time spent openning the target: end_time-start_time

	// Individual Op bandwidths - Only used when -extendedstats option is specified
	double		longest_op_time; 		// Longest op time that occured during this pass
	double		longest_read_op_time; 	// Longest read op time that occured during this pass
	double		longest_write_op_time; 	// Longest write op time that occured during this pass
	double		shortest_op_time; 		// Shortest op time that occurred during this pass
	double		shortest_read_op_time; 	// Shortest read op time that occured during this pass
	double		shortest_write_op_time; // Shortest write op time that occured during this pass

	int64_t		longest_op_bytes; 			// Bytes xfered when the longest op time occured during this pass
	int64_t	 	longest_read_op_bytes;	 	// Bytes xfered when the longest read op time occured during this pass
	int64_t 	longest_write_op_bytes; 	// Bytes xfered when the longest write op time occured during this pass
	int64_t 	shortest_op_bytes; 			// Bytes xfered when the shortest op time occured during this pass
	int64_t 	shortest_read_op_bytes; 	// Bytes xfered when the shortest read op time occured during this pass
	int64_t 	shortest_write_op_bytes;	// Bytes xfered when the shortest write op time occured during this pass

	int64_t		longest_op_number; 			// Operation Number when the longest op time occured during this pass
	int64_t	 	longest_read_op_number; 	// Operation Number when the longest read op time occured during this pass
	int64_t 	longest_write_op_number; 	// Operation Number when the longest write op time occured during this pass
	int64_t 	shortest_op_number; 		// Operation Number when the shortest op time occured during this pass
	int64_t 	shortest_read_op_number; 	// Operation Number when the shortest read op time occured during this pass
	int64_t 	shortest_write_op_number;	// Operation Number when the shortest write op time occured during this pass

	int32_t		longest_op_pass_number;		// Pass Number when the longest op time occured during this pass
	int32_t		longest_read_op_pass_number;// Pass Number when the longest read op time occured
	int32_t		longest_write_op_pass_number;// Pass Number when the longest write op time occured 
	int32_t		shortest_op_pass_number;	// Pass Number when the shortest op time occured 
	int32_t		shortest_read_op_pass_number;// Pass Number when the shortest read op time occured 
	int32_t		shortest_write_op_pass_number;// Pass Number when the shortest write op time occured 

	double		highest_bandwidth;		// Highest individual op data rate in MB/sec
	double		highest_read_bandwidth;	// Highest individual op read data rate in MB/sec 
	double		highest_write_bandwidth;// Highest individual op write data rate in MB/sec 
	double		lowest_bandwidth;		// Lowest individual op data rate in MB/sec
	double		lowest_read_bandwidth;	// Lowest individual op read data rate in MB/sec 
	double		lowest_write_bandwidth;	// Lowest individual op write data rate in MB/sec 

	double		highest_iops;			// Highest individual op I/O Operations per second 
	double		highest_read_iops;		// Highest individual op read I/O Operations per second 
	double		highest_write_iops;		// Highest individual op write I/O Operations per second 
	double		lowest_iops;			// Lowest individual op I/O Operations per second 
	double		lowest_read_iops;		// Lowest individual op read I/O Operations per second 
	double		lowest_write_iops;		// Lowest individual op write I/O Operations per second 
}; 
typedef struct results results_t; 

void xdd_results_fmt_what(results_t *rp);
void xdd_results_fmt_pass_number(results_t *rp);
void xdd_results_fmt_target_number(results_t *rp);
void xdd_results_fmt_queue_number(results_t *rp);
void xdd_results_fmt_bytes_transferred(results_t *rp);
void xdd_results_fmt_bytes_read(results_t *rp);
void xdd_results_fmt_bytes_written(results_t *rp);
void xdd_results_fmt_ops(results_t *rp);
void xdd_results_fmt_read_ops(results_t *rp);
void xdd_results_fmt_write_ops(results_t *rp);
void xdd_results_fmt_bandwidth(results_t *rp);
void xdd_results_fmt_read_bandwidth(results_t *rp);
void xdd_results_fmt_write_bandwidth(results_t *rp);
void xdd_results_fmt_iops(results_t *rp);
void xdd_results_fmt_read_iops(results_t *rp);
void xdd_results_fmt_write_iops(results_t *rp);
void xdd_results_fmt_latency(results_t *rp);
void xdd_results_fmt_elapsed_time_from_1st_op(results_t *rp);
void xdd_results_fmt_elapsed_time_from_pass_start(results_t *rp);
void xdd_results_fmt_elapsed_over_head_time(results_t *rp);
void xdd_results_fmt_elapsed_pattern_fill_time(results_t *rp);
void xdd_results_fmt_elapsed_buffer_flush_time(results_t *rp);
void xdd_results_fmt_cpu_time(results_t *rp);
void xdd_results_fmt_user_time(results_t *rp);
void xdd_results_fmt_system_time(results_t *rp);
void xdd_results_fmt_percent_cpu_time(results_t *rp);
void xdd_results_fmt_percent_user_time(results_t *rp);
void xdd_results_fmt_percent_system_time(results_t *rp);
void xdd_results_fmt_op_type(results_t *rp);
void xdd_results_fmt_transfer_size_bytes(results_t *rp);
void xdd_results_fmt_transfer_size_blocks(results_t *rp);
void xdd_results_fmt_transfer_size_kbytes(results_t *rp);
void xdd_results_fmt_transfer_size_mbytes(results_t *rp);
void xdd_results_fmt_e2e_send_receive_time(results_t *rp);
void xdd_results_fmt_e2e_percent_send_receive_time(results_t *rp);
void xdd_results_fmt_e2e_lag_time(results_t *rp);
void xdd_results_fmt_e2e_percent_lag_time(results_t *rp);
void xdd_results_fmt_e2e_first_read_time(results_t *rp);
void xdd_results_fmt_e2e_last_write_time(results_t *rp);
void xdd_results_fmt_delimiter(results_t *rp);
void xdd_results_fmt_text(results_t *rp);

#endif
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
