/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines that perform various initialization 
 * functions when xdd is started.
 */
#include "xint.h"
#include <stdint.h>
/*----------------------------------------------------------------------------*/
/* xdd_heartbeat() 
 * Heartbeat runs as a separate thread that will periodically interogate the
 * Worker Data Struct of each running Worker Thread and display the:
 *    - Current pass number
 *    - Ops completed or issued so far across all Worker Threads  
 *    - Estimated BW
 * 
 * A global variable called "heartbeat_flags" is used by the results_manager
 * to prevent heartbeat from displaying information while it is trying to
 * display information (set HEARTBEAT_HOLDOFF). 
 * It is also used to tell heartbeat to exit (set HEARTBEAT_EXIT)
 * When the -heartbeat option is specified then the HEARTBEAT_ACTIVE flag is set
 */
static	char	activity_indicators[8] = {'|','/','-','\\','*'};
void *
xdd_heartbeat(void *data) {
	int32_t 	i;						// If it is not obvious what "i" is then you should not be reading this
	nclk_t 		now;					// Current time
	nclk_t 		earliest_start_time;	// The earliest start time of the Worker Threads for a target
	int64_t		total_bytes_xferred;	// The total number of bytes xferred for all Worker Threads for a target
	int64_t		total_ops_issued;		// This is the total number of ops issued/completed up til now
	double		elapsed;				// Elapsed time for this Worker Thread
	target_data_t 		*tdp;			// Pointer to the Target's Data Struct
	int			activity_index;			// A number from 0 to 4 to index into the activity indicators table
	int			prior_activity_index;	// Used to save the state of the activity index 
	int			scattered_output;		// When set to something other than 0 it means that the output is directed to mutliple files
	uint32_t	interval;				// The shortest heartbeat interval that is greater than 0
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier
	xdd_plan_t* planp = (xdd_plan_t*)data;

	// Init indices to 0
	activity_index = 0; 
	prior_activity_index = 0;
	xdd_init_barrier_occupant(&barrier_occupant, "HEARTBEAT", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);

	// Open all the Heartbeat output Files if they are not the default (stderr)
	scattered_output = 0;
	interval = 31536000; // a year's worth of seconds
	for (i = 0; i < planp->number_of_targets; i++) {
		tdp = planp->target_datap[i];
		if (tdp->td_hb.hb_filename) {
			tdp->td_hb.hb_file_pointer = fopen(tdp->td_hb.hb_filename,"a");
			scattered_output++;
		} else {
			tdp->td_hb.hb_file_pointer = stderr;
		}

		// Check to see if this is the shortest non-zero interval and save it if it is
		if (tdp->td_hb.hb_interval) {
			if (tdp->td_hb.hb_interval < interval) 
				interval = tdp->td_hb.hb_interval;
		}
	}
	planp->heartbeat_flags |= HEARTBEAT_ACTIVE;
	// Enter this barrier and wait for the heartbeat monitor to initialize
	xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant, 0);

	while (1) {
		sleep(interval);
		if (xgp->canceled) {
			fprintf(xgp->errout,"\nHEARTBEAT: Canceled\n");
			return(0);
		}
		if (xgp->abort) {
			fprintf(xgp->errout,"\nHEARTBEAT: Abort\n");
			return(0);
		}
		if (planp->heartbeat_flags & HEARTBEAT_HOLDOFF) 
			continue;
		if (planp->heartbeat_flags & HEARTBEAT_EXIT) 
			return(0);

		// Display all the requested items for each target
		for (i = 0; i < planp->number_of_targets; i++) {
			tdp = planp->target_datap[i];

			// Check to see if this Target wants a heartbeat display, if not, just continue
			if (tdp->td_hb.hb_interval == 0) 
				continue;

			// Display the "legend" string
			// If the "scattered_output" is greater than 0 then display the legend for each target
			// Otherwise just display it for target 0
			if ((scattered_output)  || (i == 0))
			    xdd_heartbeat_legend(planp, tdp);
			total_bytes_xferred = 0;
			earliest_start_time = NCLK_MAX;
			total_ops_issued = 0;
			if (tdp->td_counters.tc_pass_start_time == NCLK_MAX) { // Haven't started yet...
				fprintf(tdp->td_hb.hb_file_pointer," + WAITING");
				continue; 
			}
			// Get the number of bytes xferred by all Worker Threads for this Target
			total_bytes_xferred = tdp->td_counters.tc_accumulated_bytes_xfered;
			total_ops_issued = tdp->td_counters.tc_accumulated_op_count;
			// Determine if this is the earliest start time
			if (earliest_start_time > tdp->td_counters.tc_pass_start_time) 
				earliest_start_time = tdp->td_counters.tc_pass_start_time;

			// At this point we have the total number of bytes transferred for this target
			// as well as the time the first Worker Thread started and the most recent op number issued.
			// From that we can calculate the estimated BW for the target as a whole 

			tdp = planp->target_datap[i];
			if (tdp->td_current_state & TARGET_CURRENT_STATE_PASS_COMPLETE) {
				now = tdp->td_counters.tc_pass_end_time;
				prior_activity_index = activity_index;
				activity_index = 4;
			} else nclk_now(&now);

			// Calculate the elapsed time so far
			elapsed = ((double)((double)now - (double)earliest_start_time)) / FLOAT_BILLION;

			// Display the activity indicator 
			fprintf(tdp->td_hb.hb_file_pointer," + [%c]", activity_indicators[activity_index]);

			// Display the data for this target
			xdd_heartbeat_values(tdp, total_bytes_xferred, total_ops_issued, elapsed);
			
			// If this target has completed its pass then the activity indicator is static
			if (activity_index == 4)
				activity_index = prior_activity_index;

		} // End of FOR loop that processes all targets
		if (activity_index >= 3)
			activity_index = 0;
		else activity_index++;
	}
DFLOW("\n----------------------heartbeat: Exit-------------------------\n");
} /* end of xdd_heartbeat() */

/*----------------------------------------------------------------------------*/
/* xdd_heartbeat_legend() 
 * This will display the 'legend' for the heartbeat line 
 * The legend appears at the start of the heartbeat line and indicates
 * the pass number followed by the names of the values being displayed.
 */
void
xdd_heartbeat_legend(xdd_plan_t* planp, target_data_t *tdp) {
	nclk_t 		now;					// Current time
	double		elapsed_seconds;		// Elapsed time in seconds
	time_t		current_time;			// For the Time of Day option
	char		current_time_string[32];	// For the Time of Day option
	int			len;


	if (tdp->td_hb.hb_options & HB_LF)  
		 fprintf(tdp->td_hb.hb_file_pointer,"\n"); // Put a LineFeed character at the end of this line
	else fprintf(tdp->td_hb.hb_file_pointer,"\r"); // Otherwise just a carriage return

	fprintf(tdp->td_hb.hb_file_pointer,"Pass,%04d,",tdp->td_counters.tc_pass_number);
	if (tdp->td_hb.hb_options & HB_HOST)  // Display Current number of OPS performed 
		fprintf(tdp->td_hb.hb_file_pointer,"/HOST,%s,",planp->hostname.nodename);

	if (tdp->td_hb.hb_options & HB_TOD) {  // Display the current Time of Day
		current_time = time(NULL);
		ctime_r(&current_time,(char *)current_time_string);
		len = strlen((char *)current_time_string);
		if (len > 0)
	 		current_time_string[(len-1)] = '\0';
		
		fprintf(tdp->td_hb.hb_file_pointer,"/TOD,%s,",current_time_string);
	}

	if (tdp->td_hb.hb_options & HB_ELAPSED) {  // Display the elapsed number of seconds this has been running
		nclk_now(&now);
		elapsed_seconds = ((double)((double)now - (double)planp->run_start_time)) / FLOAT_BILLION;
		fprintf(tdp->td_hb.hb_file_pointer,"/ELAPSED,%.0f,",elapsed_seconds);
	}

	if (tdp->td_hb.hb_options & HB_TARGET)  // Display Target Number 
		fprintf(tdp->td_hb.hb_file_pointer,"/TARGET");
	if (tdp->td_hb.hb_options & HB_OPS)  // Display Current number of OPS performed 
		fprintf(tdp->td_hb.hb_file_pointer,"/OPS");
	if (tdp->td_hb.hb_options & HB_BYTES)  // Display Current number of BYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,"/BYTES");
	if (tdp->td_hb.hb_options & HB_KBYTES)  // Display Current number of KILOBYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,"/KB");
	if (tdp->td_hb.hb_options & HB_MBYTES)  // Display Current number of MEGABYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,"/MB");
	if (tdp->td_hb.hb_options & HB_GBYTES)  // Display Current number of GIGABYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,"/GB");
	if (tdp->td_hb.hb_options & HB_BANDWIDTH)  // Display Current Aggregate BANDWIDTH 
		fprintf(tdp->td_hb.hb_file_pointer,"/BW");
	if (tdp->td_hb.hb_options & HB_IOPS)  // Display Current Aggregate IOPS 
		fprintf(tdp->td_hb.hb_file_pointer,"/IOPS");
	if (tdp->td_hb.hb_options & HB_PERCENT)  // Display Percent Complete 
		fprintf(tdp->td_hb.hb_file_pointer,"/PCT");
	if (tdp->td_hb.hb_options & HB_ET)  // Display Estimated Time to Completion
		fprintf(tdp->td_hb.hb_file_pointer,"/ET");
} // End of xdd_heartbeat_legend()
/*----------------------------------------------------------------------------*/
/* xdd_heartbeat_values() 
 * This will calculate and display the the actual values for a specific target
 * Arguments to this subroutine are:
 *    - pointer to the Target Data Struct of this target
 *    - Number of bytes transfered by this target
 *    - Number of ops completed by this target
 *    - Elapsed time in seconds used by this target
 *
 * There is a case when this target is in the middle of a restart operation 
 * in which case the Percent_Complete should represent the percentage of the
 * entire copy operation that has completed - not just the percentage of the
 * current restart operation that is in progress. 
 * For example, when copying a 10GB file, if the copy was stopped after 
 * the first 3GB had transferred and then restarted at this point (+3GB), 
 * the restarted XDD operation will only need to copy 7GBytes of data.
 * Hence, when XDD starts, the heartbeat should report that it is already 30%
 * complete and start counting from there. If the "-heartbeat ignorerestart" 
 * option is specified then heartbeat will report that it is 0% complete
 * at the beginning of a restart. 
 * The number of bytes transferred and ops completed will  also be adjusted 
 * according to the restart operation and whether or not the "ignorerestart"
 * was specified. 
 * The number of "target_ops" during a resume_copy operation is just the 
 * number of ops required to transfer the remaining data. In order to 
 * correctly calculate the percent complete we need to know the total ops
 * completed for all the data for the entire copy operation. The total
 * data for the copy operation is the start offset (in bytes) plus the
 * amount of data for this instance of XDD. The target ops to move that amount
 * of data is (total_data)/(iosize). 
 * The Estimated Time to Completion is *not* adjusted and should be an 
 * accurate representation of when the current XDD copy operation will complete. 
 * Similarly, IOPS and Bandwidth are also not affected by the restart status. 
 */
void
xdd_heartbeat_values(target_data_t *tdp, int64_t bytes, int64_t ops, double elapsed) {
	double	d;
	int64_t	adjusted_bytes;
	int64_t	adjusted_ops;
	int64_t	adjusted_target_ops;


	adjusted_bytes = bytes;
	adjusted_ops = ops;
	adjusted_target_ops = tdp->td_target_ops;
	// Check to see if we are in a "restart" operation and make appropriate adjustments
	if (tdp->td_restartp) {
		if ((tdp->td_restartp->flags & RESTART_FLAG_RESUME_COPY) &&
			!(tdp->td_hb.hb_options & HB_IGNORE_RESTART)) { // We were not asked to ignore the restart adjustments
			// Adjust the number of bytes completed so far - add in the start offset
			adjusted_bytes = bytes + (tdp->td_start_offset * tdp->td_block_size);
			// Adjust the number of ops completed so far
			adjusted_ops = adjusted_bytes / tdp->td_xfer_size;
			// Adjust the number of target_ops needed to complete the entire copy
			adjusted_target_ops = (tdp->td_target_bytes_to_xfer_per_pass + (tdp->td_start_offset*tdp->td_block_size))/tdp->td_xfer_size; 
		} // End of making adjustments for a restart operation
	}
	if (tdp->td_hb.hb_options & HB_TARGET)  // Display Target Number of this information 
		fprintf(tdp->td_hb.hb_file_pointer,",%04d,tgt",tdp->td_target_number);
	if (tdp->td_hb.hb_options & HB_OPS)  // Display current number of OPS performed 
		fprintf(tdp->td_hb.hb_file_pointer,",%010lld,ops",(long long int)adjusted_ops);
	if (tdp->td_hb.hb_options & HB_BYTES)  // Display current number of BYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,",%015lld,B",(long long int)adjusted_bytes);
	if (tdp->td_hb.hb_options & HB_KBYTES)  // Display current number of KILOBYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,",%013.1f,KiB",(double)((double)adjusted_bytes / 1024.0) );
	if (tdp->td_hb.hb_options & HB_MBYTES)  // Display current number of MEGABYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,",%010.1f,MiB",(double)((double)adjusted_bytes / (1024.0*1024.0)) );
	if (tdp->td_hb.hb_options & HB_GBYTES)  // Display current number of GIGABYTES transferred 
		fprintf(tdp->td_hb.hb_file_pointer,",%07.1f,GiB",(double)((double)adjusted_bytes / (1024.0*1024.0*1024.0)) );
	// Bandwidth is calculated on unadjusted bytes regardless of whether or not we are doing a restart
	if (tdp->td_hb.hb_options & HB_BANDWIDTH) {  // Display current Aggregate BANDWIDTH 
		if (elapsed != 0.0) {
			d = (bytes/elapsed)/FLOAT_MILLION;
		} else {
			d = -1.0;
		}
		fprintf(tdp->td_hb.hb_file_pointer,",%06.2f,MBps",d);
	}
	// IOPS is calculated on unadjusted ops regardless of whether or not we are doing a restart
	if (tdp->td_hb.hb_options & HB_IOPS){  // Display current Aggregate IOPS 
		if (elapsed != 0.0) {
			d = ((double)ops)/elapsed;
		} else {
			d = -1.0;
		}
		fprintf(tdp->td_hb.hb_file_pointer,",%06.2f,iops",d);
	}

	// Percent complete is based on "adjusted bytes" 
	if (tdp->td_hb.hb_options & HB_PERCENT) {  // Display Percent Complete 
		if (tdp->td_target_ops > 0) 
			d = (double)((double)adjusted_ops / (double)adjusted_target_ops) * 100.0;
		else
			d = -1.0;
		fprintf(tdp->td_hb.hb_file_pointer,",%4.2f%%",d);
	}
	// Estimated time is based on "unadjusted bytes" otherwise the ETC would be skewed
	if (tdp->td_hb.hb_options & HB_ET) {  // Display Estimated Time to Completion
		if (tdp->td_current_state & TARGET_CURRENT_STATE_PASS_COMPLETE) 
			d = 0.0;
		else if (ops > 0) {
			// Estimate the time to completion -> ((total_ops/ops_completed)*elapsed_time - elapsed)
			d = (double)(((double)tdp->td_target_ops / (double)ops) * elapsed) - elapsed;
		} else d = -1.0;
		fprintf(tdp->td_hb.hb_file_pointer,",%07.0f,s",d);
	}
} // End of xdd_heartbeat_values()
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
