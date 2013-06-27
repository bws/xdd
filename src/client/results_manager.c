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

#include "xint.h"

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
xdd_results_manager(void *data) {
	results_t	*tmprp;				// Pointer to the results struct in each qthread PTDS
	int32_t		status;				// Status of the initialization subroutine
	int			tmp_errno;			// Save the errno
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier
	nclk_t		pass_delay_start;	// Start of a pass delay if it is specified
	nclk_t		pass_delay_end;		// End of a pass delay if it is specified
	xdd_plan_t* planp = (xdd_plan_t*)data;



	tmprp = (results_t *)valloc(sizeof(results_t));
	tmp_errno = errno;
	if (tmprp == NULL) {
			fprintf(xgp->errout, "%s: xdd_results_manager: ERROR: Cannot allocate memory for a temporary results structure!\n", xgp->progname);
			errno = tmp_errno;  // Restore the errno
			perror("xdd_results_manager: Reason");
			return(0);
	}

	// Initialize the barriers and such
	status = xdd_results_manager_init(planp);
	if (status < 0)  {
		fprintf(stderr,"%s: xdd_results_manager: ERROR: Exiting due to previous initialization failure.\n",xgp->progname);
		return(0);
	}
	
	// Enter this barrier to release main indicating that results manager has initialized
	xdd_init_barrier_occupant(&barrier_occupant, "RESULTS_MANAGER", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);
	xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,0);

	// This is the loop that runs continuously throughout the xdd run
	while (1) {
		// This barrier will release all the targets at the start of a pass so they all start at the same time
		xdd_barrier(&planp->results_targets_startpass_barrier,&barrier_occupant,1);

		// At this point all the targets are running 

		// While the targets are running, we enter this barrier to wait for them to all complete a single pass
		xdd_barrier(&planp->results_targets_endpass_barrier,&barrier_occupant,1);

		if ( xgp->abort == 1) { // Something went wrong during thread initialization so let's just leave
			xdd_process_run_results(planp);
			// Release all the Target Threads so that they can do their cleanup and exit
			xdd_barrier(&planp->results_targets_display_barrier,&barrier_occupant,1);
			return(0);
		}

		// Display the header and units line if necessary
		if (planp->results_header_displayed == 0) {
			xdd_results_header_display(tmprp, planp);
			planp->results_header_displayed = 1;
		}

		// At this point all the target qthreads should have entered the results_display_barrier
		// or the results_display_fnial_barrier and will wait for the pass or run results to be displayed

		if (xgp->run_complete) {
			xdd_process_run_results(planp);
			// Release all the Target Threads so that they can do their cleanup and exit
			xdd_barrier(&planp->results_targets_display_barrier,&barrier_occupant,1);
			// Wait for all the Target Threads do their cleanup...
			xdd_barrier(&planp->results_targets_waitforcleanup_barrier,&barrier_occupant,1);
			// Enter the FINAL barrier to tell XDD MAIN that everything is complete
			xdd_barrier(&planp->main_results_final_barrier,&barrier_occupant,0);
			return(0);
		} else { 
			xdd_process_pass_results(planp);
			planp->current_pass_number++;
			/* Insert a delay of "pass_delay" seconds if requested */
			if ((planp->pass_delay_usec > 0) && (planp->current_pass_number < planp->passes)) {
				nclk_now(&pass_delay_start);
				planp->heartbeat_holdoff = 1;
				fprintf(xgp->output,"\nStarting Pass Delay of %f seconds...",planp->pass_delay);
				fflush(xgp->output);
				usleep(planp->pass_delay_usec);
				fprintf(xgp->output,"Done\n");
				fflush(xgp->output);
				planp->heartbeat_holdoff = 0;
				nclk_now(&pass_delay_end);
				// Figure out the accumulated pass delay time so that it can be subtracted later
				planp->pass_delay_accumulated_time += (pass_delay_end - pass_delay_start);
			}

		}

		xdd_barrier(&planp->results_targets_display_barrier,&barrier_occupant,1);

	} // End of main WHILE loop for the results_manager()
	return(0);
} // End of xdd_results_manager() 

/*----------------------------------------------------------------------------*/
// xdd_results_manager_init() - Initialize barriers for the results manager
// Called by xdd_results_manager() 
// Return value of 0 is good, return of something less than zero is bad
//
int32_t
xdd_results_manager_init(xdd_plan_t *planp) {
	int32_t	status;

	status = xdd_init_barrier(planp, &planp->results_targets_display_barrier, planp->number_of_targets+1, "results_targets_display_barrier");
	status += xdd_init_barrier(planp, &planp->results_targets_startpass_barrier,planp->number_of_targets+1, "results_targets_startpass_barrier");
	status += xdd_init_barrier(planp, &planp->results_targets_endpass_barrier,planp->number_of_targets+1, "results_targets_endpass_barrier");
	status += xdd_init_barrier(planp, &planp->results_targets_waitforcleanup_barrier, planp->number_of_targets+1, "results_targets_waitforcleanup_barrier");
	
	if (status < 0)  {
		fprintf(stderr,"%s: xdd_results_manager_init: ERROR: Cannot init barriers.\n",xgp->progname);
		return(status);
	}
	return(0);

} // End of xdd_results_manager_init() 
/*----------------------------------------------------------------------------*/
// xdd_results_header_display() 
// Display the header and units output lines
// Called by xdd_results_manager() 
//
void *
xdd_results_header_display(results_t *tmprp, xdd_plan_t *planp) {
	planp->heartbeat_holdoff = 1;
	tmprp->format_string = planp->format_string;
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
	planp->heartbeat_holdoff = 0;
	return(0);
	
} // End of xdd_results_header_display()

/*----------------------------------------------------------------------------*/
// xdd_process_pass_results() 
// Called by xdd_results_manager() 
//
void *
xdd_process_pass_results(xdd_plan_t *planp) {
    int			target_number;
    target_data_t		*tdp;		// PTDS pointer of the Target PTDS
    results_t	*tarp;	// Pointer to the Target Average results structure
    results_t	*trp;	// Pointer to the Temporary Target Pass results
    results_t	targetpass_results = {0}; // Temporary for target pass results

    planp->heartbeat_holdoff = 1;

    trp = &targetpass_results;

    // Next, display pass results for each Target
    for (target_number=0; target_number<planp->number_of_targets; target_number++) {
        /* Get the Tartget_Data for this target */
        tdp = planp->target_datap[target_number]; 
        
        // Init the Target Average Results struct
        tarp = planp->target_average_resultsp[target_number];
        memset(tarp, 0, sizeof(*tarp));
        tarp->format_string = planp->format_string;
        tarp->what = "TARGET_AVERAGE";
        if (tdp->td_counters.tc_pass_number == 1) {
            tarp->earliest_start_time_this_run = (double)DOUBLE_MAX;
            tarp->earliest_start_time_this_pass = (double)DOUBLE_MAX;
            tarp->shortest_op_time = (double)DOUBLE_MAX;
            tarp->shortest_read_op_time = (double)DOUBLE_MAX;
            tarp->shortest_write_op_time = (double)DOUBLE_MAX;
        }
		
        // Init the Target Pass Results struct
        trp = &targetpass_results;

        // Get the results from this Target's PTDS and
        // stuff them into a results struct for display
        xdd_extract_pass_results(trp, tdp, planp);

        // Combine the pass results from this target with its AVERAGE results
        // from all previous passes
        tarp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_AVG);
        xdd_combine_results(tarp, trp, planp);

        // Display the Pass Results for this target
        trp->format_string = planp->format_string;
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
    
    planp->heartbeat_holdoff = 0;

    return(0);
} // End of xdd_process_pass_results() 

/*----------------------------------------------------------------------------*/
// xdd_process_run_results() 
// Calculate results for each qthread
// Called by xdd_results_manager() with a pointer to the qthread PTDS
//
void *
xdd_process_run_results(xdd_plan_t *planp) {
	int			target_number;	// Current target number
	target_data_t		*tdp;		// PTDS pointer to a Target PTDS
	results_t	*tarp;	// Pointer to the Target Average results structure
	results_t	*crp;	// Pointer to the temporary combined run results
	results_t	combined_results; // Temporary for target combined results


	// At this point all the Target Threads are waiting for this routine
	// to display the AVERAGE and COMBINED results for this run.

	// Tell the heartbeat to stop
	planp->heartbeat_holdoff = 2; 

	// Initialize the place where the COMBINED results are accumulated
	crp = &combined_results; 
	memset(crp, 0, sizeof(results_t));
	crp->flags = (RESULTS_PASS_INFO | RESULTS_COMBINED);
	crp->format_string = planp->format_string;
	crp->what = "COMBINED      ";
	crp->earliest_start_time_this_run = (double)DOUBLE_MAX;
	crp->earliest_start_time_this_pass = (double)DOUBLE_MAX;
	crp->shortest_op_time = (double)DOUBLE_MAX;
	crp->shortest_read_op_time = (double)DOUBLE_MAX;
	crp->shortest_write_op_time = (double)DOUBLE_MAX;

	for (target_number=0; target_number<planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number]; /* Get the target_datap for this target */
		tarp = planp->target_average_resultsp[target_number];

		if (tdp->td_abort) 
			xgp->abort = 1; // Indicate that this run ended with errors of some sort
			
		tarp->flags = (RESULTS_PASS_INFO | RESULTS_TARGET_AVG);
		tarp->format_string = planp->format_string;
		tarp->what = "TARGET_AVERAGE";
		tarp->output = xgp->output;
		tarp->delimiter = ' ';
		if (xgp->global_options & GO_VERBOSE) {
			// Display the Target AVERAGE results if -verbose was specified
			xdd_results_display(tarp);
			if (xgp->csvoutput) { // Display to CSV file if requested
				tarp->output = xgp->csvoutput;
				tarp->delimiter = ',';
				xdd_results_display(tarp);
			}
		}

		// Combined this Target's results with the other Targets
		xdd_combine_results(crp, tarp, planp);

	} // End of FOR loop that processes all targets for the run

	// Now lets display the COMBINED results
	crp->output = xgp->output;
	crp->delimiter = ' ';
	xdd_results_display(crp);
	if (xgp->csvoutput) { // Display to CSV file if requested
		crp->output = xgp->csvoutput;
		crp->delimiter = ',';
		xdd_results_display(crp);
	}
	if (xgp->combined_output) { // Display to CSV file if requested
		crp->output = xgp->combined_output;
		crp->delimiter = ',';
		xdd_results_display(crp);
	}

	// Process TimeStamp reports for the -ts option
//TMR FIXME
#ifdef ndef
	for (target_number=0; target_number<planp->number_of_targets; target_number++) { 
		tdp = planp->target_datap[target_number]; /* Get the target_datap for this target */
		/* Display and write the time stamping information if requested */
		if (tdp->td_tsp->ts_options & (TS_ON | TS_TRIGGERED)) {
			if (tdp->td_tsp->ts_current_entry > tdp->td_tsp->ts_size) 
				tdp->td_ttp->numents = tdp->td_tsp->ts_size;
			else tdp->td_ttp->numents = tdp->td_tsp->ts_current_entry;
			xdd_ts_reports(p);  /* generate reports if requested */
			xdd_ts_write(p); 
			xdd_ts_cleanup(tdp->td_ttp); /* call this to free the TS table in memory */
		}
	} // End of processing TimeStamp reports
#endif

	return(0);
} // End of xdd_process_run_results() 

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
xdd_combine_results(results_t *to, results_t *from, xdd_plan_t *planp) {


	// Fundamental Variables
	to->format_string = from->format_string;		// Format String for the xdd_results_display_processor() 
	if (to->flags & RESULTS_COMBINED) {				// For the COMBINED output...
		to->my_target_number = planp->number_of_targets; 	// This is the number of targets
		to->my_qthread_number = from->queue_depth; 	// This is the number of QThreads for this Target
	} else {										// For a queue or target pass then...
		to->my_target_number = from->my_target_number; 	// Target number of instance 
		to->my_qthread_number = from->my_qthread_number;// QThread number of instance 
	}
	to->queue_depth = from->queue_depth;			// Queue Depth for this target
	to->xfer_size_bytes = from->xfer_size_bytes;	// Transfer size in bytes 
	to->xfer_size_blocks = from->xfer_size_blocks;	// Transfer size in blocks 
	to->xfer_size_kbytes = from->xfer_size_kbytes;	// Transfer size in Kbytes 
	to->xfer_size_mbytes = from->xfer_size_mbytes;	// Transfer size in Mbytes 
	to->reqsize = from->reqsize; 					// RequestSize from the target_data 
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
		if (to->earliest_start_time_this_pass >= from->earliest_start_time_this_pass)
			to->earliest_start_time_this_pass = from->earliest_start_time_this_pass;
		if (to->latest_end_time_this_pass <= from->latest_end_time_this_pass)
			to->latest_end_time_this_pass = from->latest_end_time_this_pass;
		to->elapsed_pass_time = (to->latest_end_time_this_pass - to->earliest_start_time_this_pass)/FLOAT_BILLION;
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
		if (to->elapsed_pass_time > 0.0) {
			to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
			to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
			to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 
		} else { // Cannot calculate
			to->percent_user = -1.0;
			to->percent_system = -1.0;
			to->percent_cpu = -1.0; 
		}
	} else if (to->flags & RESULTS_TARGET_AVG) {
		if (to->earliest_start_time_this_pass >= from->earliest_start_time_this_pass)
			to->earliest_start_time_this_pass = from->earliest_start_time_this_pass;

		if (from->pass_number == 1) 
			to->earliest_start_time_this_run = to->earliest_start_time_this_pass; 

		if (to->latest_end_time_this_pass <= from->latest_end_time_this_pass)
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
		if (to->elapsed_pass_time > 0.0) {
			to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
			to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
			to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 
		} else { // Cannot calculate
			to->percent_user = -1.0;
			to->percent_system = -1.0;
			to->percent_cpu = -1.0; 
		}

	} else if (to->flags & RESULTS_COMBINED) {
		if (to->earliest_start_time_this_run >= from->earliest_start_time_this_run)
			to->earliest_start_time_this_run = from->earliest_start_time_this_run;
		if (to->latest_end_time_this_run <= from->latest_end_time_this_run)
			to->latest_end_time_this_run = from->latest_end_time_this_run;
		to->pass_delay_accumulated_time = planp->pass_delay_accumulated_time;
		if (to->latest_end_time_this_run <= to->earliest_start_time_this_run) 
			to->elapsed_pass_time = -1.0;
		else to->elapsed_pass_time = (to->latest_end_time_this_run - to->earliest_start_time_this_run - to->pass_delay_accumulated_time)/FLOAT_BILLION;
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
		if (to->elapsed_pass_time > 0.0) {
			to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
			to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
			to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 
		} else { // Cannot calculate
			to->percent_user = -1.0;	
			to->percent_system = -1.0;
			to->percent_cpu = -1.0; 
		}

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
		if (to->elapsed_pass_time > 0.0) {
			to->percent_user = (to->user_time / to->elapsed_pass_time) * 100.0;	// Percent of CPU time used by the application for this pass 
			to->percent_system = (to->system_time / to->elapsed_pass_time) * 100.0;		// Percent of CPU time used by the system for this pass 
			to->percent_cpu = (to->us_time / to->elapsed_pass_time) * 100.0; 	// Percent CPU time used by this process: user+system time for this pass 
		} else { // Cannot calculate
			to->percent_user = -1.0;
			to->percent_system = -1.0;
			to->percent_cpu = -1.0; 
		}

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
// This routine will extract the pass results from the specified target_data
// and put them into the specified results structure.
// Called by xdd_process_pass_results() with a pointer to the results structure
// to fill in and a pointer to the qthread PTDS that contains the raw data.
//
void *
xdd_extract_pass_results(results_t *rp, target_data_t *tdp, xdd_plan_t *planp) {

	memset(rp, 0, sizeof(results_t));

	// Basic parameters
	rp->pass_number = tdp->td_counters.tc_pass_number;	// int32
	rp->my_target_number = tdp->td_target_number;		// int32
	rp->queue_depth = tdp->td_queue_depth;				// int32
	rp->bytes_xfered = tdp->td_counters.tc_accumulated_bytes_xfered;	// int64
	rp->bytes_read = tdp->td_counters.tc_accumulated_bytes_read;		// int64
	rp->bytes_written = tdp->td_counters.tc_accumulated_bytes_written;// int64
	rp->op_count = tdp->td_counters.tc_accumulated_op_count;			// int64
	rp->read_op_count = tdp->td_counters.tc_accumulated_read_op_count;// int64
	rp->write_op_count = tdp->td_counters.tc_accumulated_write_op_count;// int64
	rp->error_count = tdp->td_counters.tc_current_error_count;	// int64
	rp->reqsize = tdp->td_reqsize;						// int32

	// Operation type
	if (tdp->td_rwratio == 0.0) 
		rp->optype = "write";
	else if (tdp->td_rwratio == 1.0) 
		rp->optype = "read";
	else rp->optype = "mixed";

	// These next values get converted from raw nanoseconds to seconds or miliseconds 
	rp->accumulated_op_time = (double)((double)tdp->td_counters.tc_accumulated_op_time / FLOAT_BILLION); // nano to seconds
	rp->accumulated_read_op_time = (double)((double)tdp->td_counters.tc_accumulated_read_op_time / FLOAT_BILLION); // nano to seconds
	rp->accumulated_write_op_time = (double)((double)tdp->td_counters.tc_accumulated_write_op_time / FLOAT_BILLION); // nano to seconds
	rp->accumulated_pattern_fill_time = (double)((double)tdp->td_counters.tc_accumulated_pattern_fill_time / FLOAT_BILLION); // nano to milli
	rp->accumulated_flush_time = (double)((double)tdp->td_counters.tc_accumulated_flush_time / FLOAT_BILLION); // nano to milli
	rp->earliest_start_time_this_run = (double)(tdp->td_first_pass_start_time); // nanoseconds
	rp->earliest_start_time_this_pass = (double)(tdp->td_counters.tc_pass_start_time); // nanoseconds
	rp->latest_end_time_this_run = (double)(tdp->td_counters.tc_pass_end_time); // nanoseconds
	rp->latest_end_time_this_pass = (double)(tdp->td_counters.tc_pass_end_time); // nanoseconds
	rp->elapsed_pass_time = (double)((double)(tdp->td_counters.tc_pass_end_time - tdp->td_counters.tc_pass_start_time) / FLOAT_BILLION); // nano to seconds
	if (rp->elapsed_pass_time == 0.0) 
		rp->elapsed_pass_time = -1.0; // This is done to prevent and divide-by-zero problems
	rp->accumulated_elapsed_time += rp->elapsed_pass_time; 

	// These get calculated here
	rp->xfer_size_bytes = tdp->td_xfer_size;				// bytes
	rp->xfer_size_blocks = tdp->td_xfer_size/tdp->td_block_size;	// blocks
	rp->xfer_size_kbytes = tdp->td_xfer_size/1024;			// kbytes
	rp->xfer_size_mbytes = tdp->td_xfer_size/(1024*1024);	// mbytes

	// Bandwidth
	rp->bandwidth = (double)(((double)rp->bytes_xfered / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec
	rp->read_bandwidth = (double)(((double)rp->bytes_read / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec
	rp->write_bandwidth = (double)(((double)rp->bytes_written / (double)rp->elapsed_pass_time) / FLOAT_MILLION);  // MB/sec

	// IOPS and Latency
	rp->iops = (double)((double)rp->op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	rp->read_iops = (double)(rp->read_op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	rp->write_iops = (double)(rp->write_op_count / (double)rp->elapsed_pass_time);  // OPs/sec
	if (rp->iops == 0.0)
		rp->latency = 0.0;
	else rp->latency = (double)((1.0/rp->iops) * 1000.0);  // milliseconds

	// Times
	rp->user_time =   (double)(tdp->td_counters.tc_current_cpu_times.tms_utime  - tdp->td_counters.tc_starting_cpu_times_this_pass.tms_utime)/(double)(xgp->clock_tick); // Seconds
	rp->system_time = (double)(tdp->td_counters.tc_current_cpu_times.tms_stime  - tdp->td_counters.tc_starting_cpu_times_this_pass.tms_stime)/(double)(xgp->clock_tick); // Seconds
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

	// E2E Times
	if (tdp->td_e2ep) {
		rp->e2e_sr_time_this_pass = (double)tdp->td_e2ep->e2e_sr_time/FLOAT_BILLION; // E2E SendReceive Time in MicroSeconds
		rp->e2e_io_time_this_pass = (double)rp->elapsed_pass_time; // E2E IO  Time in MicroSeconds
		if (rp->e2e_io_time_this_pass == 0.0)
			rp->e2e_sr_time_percent_this_pass = 0.0; // Percentage of IO Time spent in SendReceive
		else rp->e2e_sr_time_percent_this_pass = (rp->e2e_sr_time_this_pass/rp->e2e_io_time_this_pass)*100.0; // Percentage of IO Time spent in SendReceive
		rp->e2e_wait_1st_msg = (double)tdp->td_e2ep->e2e_wait_1st_msg/FLOAT_BILLION; // MicroSeconds
	}

	// Extended Statistics
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
	return(0);
} // End of xdd_extract_pass_results() 

#ifdef ndef
/*----------------------------------------------------------------------------*/
// xdd_results_dump 
void *
xdd_results_dump(results_t *rp, char *dumptype, xdd_plan_t *planp) {

	fprintf(xgp->output,"\n\n------------------------ Dumping %s -------------------------------\n\n", dumptype);
	fprintf(xgp->output,"	flags = 0x%016x\n",(unsigned int)rp->flags);				// Flags that tell the display function what to display
	fprintf(xgp->output,"	*what = '%s'\n",rp->what);					// The type of information line to display - Queue Pass, Target Pass, Queue Avg, Target Avg, Combined
	fprintf(xgp->output,"	*output = 0x%016x\n",(unsigned int)rp->output);			// This points to the output file 
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
	fprintf(xgp->output,"	reqsize = %d\n",rp->reqsize); 			// RequestSize from the target_data 
	fprintf(xgp->output,"	pass_number = %d\n",rp->pass_number); 	// Pass number of this set of results 
	fprintf(xgp->output,"	*optype = '%s'\n",rp->optype);			// Operation type - read, write, or mixed

	// Incremented Counters
	fprintf(xgp->output,"	bytes_xfered = %lld\n",(long long)rp->bytes_xfered);		// Bytes transfered 
	fprintf(xgp->output,"	bytes_read = %lld\n",(long long)rp->bytes_read);			// Bytes transfered during read operations
	fprintf(xgp->output,"	bytes_written = %lld\n",(long long)rp->bytes_written);		// Bytes transfered during write operations
	fprintf(xgp->output,"	op_count = %lld\n",(long long)rp->op_count);    			// Operations performed 
	fprintf(xgp->output,"	read_op_count = %lld\n",(long long)rp->read_op_count);		// Read operations performed 
	fprintf(xgp->output,"	write_op_count = %lld\n",(long long)rp->write_op_count); 	// Write operations performed 
	fprintf(xgp->output,"	error_count = %lld\n",(long long)rp->error_count);  		// Number of I/O errors 

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
	fprintf(xgp->output,"	compare_errors = %lld\n",(long long)rp->compare_errors);			// Number of compare errors on a sequenced data pattern check 
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

	fprintf(xgp->output,"	longest_op_bytes = %lld\n",(long long)rp->longest_op_bytes); 			// Bytes xfered when the longest op time occured during this pass
	fprintf(xgp->output," 	longest_read_op_bytes = %lld\n",(long long)rp->longest_read_op_bytes);	 	// Bytes xfered when the longest read op time occured during this pass
	fprintf(xgp->output," 	longest_write_op_bytes = %lld\n",(long long)rp->longest_write_op_bytes); 	// Bytes xfered when the longest write op time occured during this pass
	fprintf(xgp->output," 	shortest_op_bytes = %lld\n",(long long)rp->shortest_op_bytes); 			// Bytes xfered when the shortest op time occured during this pass
	fprintf(xgp->output," 	shortest_read_op_bytes = %lld\n",(long long)rp->shortest_read_op_bytes); 	// Bytes xfered when the shortest read op time occured during this pass
	fprintf(xgp->output," 	shortest_write_op_bytes = %lld\n",(long long)rp->shortest_write_op_bytes);	// Bytes xfered when the shortest write op time occured during this pass

	fprintf(xgp->output,"	longest_op_number = %lld\n",(long long)rp->longest_op_number); 			// Operation Number when the longest op time occured during this pass
	fprintf(xgp->output," 	longest_read_op_number = %lld\n",(long long)rp->longest_read_op_number); 	// Operation Number when the longest read op time occured during this pass
	fprintf(xgp->output," 	longest_write_op_number = %lld\n",(long long)rp->longest_write_op_number); 	// Operation Number when the longest write op time occured during this pass
	fprintf(xgp->output," 	shortest_op_number = %lld\n",(long long)rp->shortest_op_number); 		// Operation Number when the shortest op time occured during this pass
	fprintf(xgp->output," 	shortest_read_op_number = %lld\n",(long long)rp->shortest_read_op_number); 	// Operation Number when the shortest read op time occured during this pass
	fprintf(xgp->output," 	shortest_write_op_number = %lld\n",(long long)rp->shortest_write_op_number);	// Operation Number when the shortest write op time occured during this pass

	fprintf(xgp->output,"	longest_op_pass_number = %lld\n",(long long)rp->longest_op_pass_number);		// Pass Number when the longest op time occured during this pass
	fprintf(xgp->output,"	longest_read_op_pass_number = %lld\n",(long long)rp->longest_read_op_pass_number);// Pass Number when the longest read op time occured
	fprintf(xgp->output,"	longest_write_op_pass_number = %lld\n",(long long)rp->longest_write_op_pass_number);// Pass Number when the longest write op time occured 
	fprintf(xgp->output,"	shortest_op_pass_number = %lld\n",(long long)rp->shortest_op_pass_number);	// Pass Number when the shortest op time occured 
	fprintf(xgp->output,"	shortest_read_op_pass_number = %lld\n",(long long)rp->shortest_read_op_pass_number);// Pass Number when the shortest read op time occured 
	fprintf(xgp->output,"	shortest_write_op_pass_number = %lld\n",(long long)rp->shortest_write_op_pass_number);// Pass Number when the shortest write op time occured 

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
	return(0);
}
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
