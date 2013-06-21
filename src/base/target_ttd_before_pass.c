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
#include "xint.h"

//******************************************************************************
// Things the Target Thread has to do before a Pass is started
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_timer_calibration_before_pass() - This subroutine will check the 
 * resolution of the timer on the system and display information about it if 
 * requested.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_timer_calibration_before_pass(void) {
	nclk_t	t1,t2,t3;
#ifdef WIN32 
        int32_t i;
#else
        int 	status;
        int32_t i;
	struct timespec req;
	struct timespec rem;
#endif

	if (xgp->global_options & GO_TIMER_INFO) {
		nclk_now(&t1);
		for (i=0; i<100; i++) 
			nclk_now(&t2);
		nclk_now(&t2);
		t3=t2-t1;

		fprintf(xgp->output,"XDD Timer Calibration Info: Average time to get time stamp=%llu nanoseconds\n",t3/100);

#ifdef WIN32
		for (i=1; i< 1001; i*=10) {
			nclk_now(&t1);
			Sleep(i);
			nclk_now(&t3);
			t3 -= t1;
fprintf(xgp->output,"XDD Timer Calibration Info: Requested sleep time in microseconds=%d, Actual sleep time in microseconds=%llu\n",i*1000,t3/MILLION);
		}
#else // Do this for Systems other than Windows
		for (i=1; i< 1000001; i*=10) {
			req.tv_sec = 0;
			req.tv_nsec = i;
			rem.tv_sec = 0;
			rem.tv_nsec = 0;
			nclk_now(&t1);
			status = nanosleep(&req, &rem);
			nclk_now(&t3);
			if (status) { // This means that the nanosleep was interrupted early
				req.tv_sec = rem.tv_sec;
				req.tv_nsec = rem.tv_nsec;
				if (i > 1) 
					i=1;
				continue;
			}
			t3 -= t1;
fprintf(xgp->output,"XDD Timer Calibration Info: Requested sleep time in nanoseconds=%d, Actual sleep time in nanoseconds=%llu\n",i*1000,t3/MILLION);
		}
#endif
	}
} // End of xdd_timer_calibration_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_start_delay_before_pass() - This subroutine will sleep for the period 
 * of time specified by the -startdelay option.  
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_start_delay_before_pass(target_data_t *tdp) {
	int				status;
	struct timespec	req;	// The requested sleep time
	struct timespec	rem;	// The remaining sleep time is awoken early
#if (WIN32 | IRIX)
	int32_t 		sleep_time_dw;
#endif


	/* Check to see if this thread has a start delay time. If so, just hang out
	 * until the start delay time has elapsed and then spring into action!
	 * The start_delay time is stored as a nclk_t which is units of nano seconds.
	 */
	if (tdp->td_start_delay <= 0.0)
		return; // No start delay

	// Convert the start delay from nanoseconds to milliseconds
#ifdef WIN32
	sleep_time_dw = (int32_t)(tdp->td_start_delay_psec/BILLION);
	Sleep(sleep_time_dw);
#elif (IRIX )
	sleep_time_dw = (int32_t)(tdp->td_start_delay_psec/BILLION);
	sginap((sleep_time_dw*CLK_TCK)/1000);
#else
	req.tv_sec = tdp->td_start_delay; // Whole seconds
	req.tv_nsec = (tdp->td_start_delay - req.tv_sec) * BILLION; // Fraction of a second
	fprintf(xgp->output, "\nTarget %d Beginning requested Start Delay for %f seconds before pass %d...\n",tdp->td_target_number, tdp->td_start_delay, tdp->td_tgtstp->my_current_pass_number);
	fflush(xgp->output);
	rem.tv_sec = 0; // zero this out just to make sure
	rem.tv_nsec = 0; // zero this out just to make sure
	status = nanosleep(&req, &rem);
	while (status) { // If we woke up early, just keep going to sleep for a while
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
		rem.tv_sec = 0; // zero this out just to make sure
		rem.tv_nsec = 0; // zero this out just to make sure
		status = nanosleep(&req, &rem);
	} 
	fprintf(xgp->output, "\nTarget %d Starting pass %d after requested delay of %f seconds\n",tdp->td_target_number,tdp->td_tgtstp->my_current_pass_number, tdp->td_start_delay);
	fflush(xgp->output);
#endif
} // End of xdd_start_delay_before_pass()


/*----------------------------------------------------------------------------*/
/* xdd_raw_before_pass() - This subroutine initializes the variables that
 * are used by the read_after_write option
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_raw_before_pass(target_data_t *tdp) {
	xdd_raw_t		*rawp;

	if ((tdp->td_target_options & TO_READAFTERWRITE) == 0)
		return;

	rawp = tdp->td_rawp;

	// Initialize the read-after-write variables
	rawp->raw_msg_sent = 0;
	rawp->raw_msg_recv = 0;
	rawp->raw_msg_last_sequence = 0;
	rawp->raw_msg.sequence = 0;
	rawp->raw_prev_loc = 0;
	rawp->raw_prev_len = 0;
	rawp->raw_data_ready = 0;
	rawp->raw_data_length = 0;

} // End of xdd_raw_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_e2e_before_pass() - This subroutine initializes the variables that
 * are used by the end-to-end option
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_e2e_before_pass(target_data_t *tdp) {

	if ((tdp->td_target_options & TO_ENDTOEND) == 0)
		return;

	// Initialize the read-after-write variables
	tdp->td_e2ep->e2e_msg_sent = 0;
	tdp->td_e2ep->e2e_msg_recv = 0;
	tdp->td_e2ep->e2e_msg_sequence_number = 0;
	tdp->td_e2ep->e2e_prev_loc = 0;
	tdp->td_e2ep->e2e_prev_len = 0;
	tdp->td_e2ep->e2e_data_length = 0;
	tdp->td_e2ep->e2e_sr_time = 0;

} // End of xdd_e2e_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_init_ptds_before_pass() - Reset variables to known state
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void 
xdd_init_ptds_before_pass(target_data_t *tdp) {
    
	// Init all the pass-related variables to 0
	tdp->td_tgtstp->my_elapsed_pass_time = 0;
	tdp->td_tgtstp->my_first_op_start_time = 0;
	tdp->td_tgtstp->my_accumulated_op_time = 0; 
	tdp->td_tgtstp->my_accumulated_read_op_time = 0;
	tdp->td_tgtstp->my_accumulated_write_op_time = 0;
	tdp->td_tgtstp->my_accumulated_pattern_fill_time = 0;
	tdp->td_tgtstp->my_accumulated_flush_time = 0;
	//
	tdp->td_tgtstp->my_current_op_number = 0; 		// The current operation number init to 0
	tdp->td_tgtstp->my_current_op_count = 0; 		// The number of read+write operations that have completed so far
	tdp->td_tgtstp->my_current_read_op_count = 0;	// The number of read operations that have completed so far 
	tdp->td_tgtstp->my_current_write_op_count = 0;	// The number of write operations that have completed so far 
	tdp->td_tgtstp->my_current_bytes_xfered = 0;		// Total number of bytes transferred to far (to storage device, not network)
	tdp->td_tgtstp->my_current_bytes_read = 0;		// Total number of bytes read to far (from storage device, not network)
	tdp->td_tgtstp->my_current_bytes_written = 0;	// Total number of bytes written to far (to storage device, not network)
	tdp->td_tgtstp->my_current_byte_location = 0; 	// Current byte location for this I/O operation 
	tdp->td_tgtstp->my_current_io_status = 0; 				// I/O Status of the last I/O operation for this qthread
	tdp->td_tgtstp->my_current_io_errno = 0; 				// The errno associated with the status of this I/O for this thread
	tdp->td_tgtstp->my_current_error_count = 0;		// The number of I/O errors for this qthread
	//
	// Longest and shortest op times - RESET AT THE START OF EACH PASS
	if (tdp->td_esp) {
		tdp->td_esp->my_longest_op_time = 0;			// Longest op time that occured during this pass
		tdp->td_esp->my_longest_op_number = 0; 		// Number of the operation where the longest op time occured during this pass
		tdp->td_esp->my_longest_read_op_time = 0; 	// Longest read op time that occured during this pass
		tdp->td_esp->my_longest_read_op_number = 0; 	// Number of the read operation where the longest op time occured during this pass
		tdp->td_esp->my_longest_write_op_time = 0; 	// Longest write op time that occured during this pass
		tdp->td_esp->my_longest_write_op_number = 0; 	// Number of the write operation where the longest op time occured during this pass
		tdp->td_esp->my_shortest_op_time = NCLK_MAX; 	// Shortest op time that occurred during this pass
		tdp->td_esp->my_shortest_op_number = 0; 		// Number of the operation where the shortest op time occured during this pass
		tdp->td_esp->my_shortest_read_op_time = NCLK_MAX;	// Shortest read op time that occured during this pass
		tdp->td_esp->my_shortest_read_op_number = 0; 	// Number of the read operation where the shortest op time occured during this pass
		tdp->td_esp->my_shortest_write_op_time = NCLK_MAX;// Shortest write op time that occured during this pass
		tdp->td_esp->my_shortest_write_op_number = 0;	// Number of the write operation where the shortest op time occured during this pass
	}

	tdp->td_tgtstp->my_time_limit_expired = 0;		// The time limit expiration indicator

} // End of xdd_init_ptds_before_pass()
 
/*----------------------------------------------------------------------------*/
/* xdd_target_ttd_before_pass() - This subroutine is called by targetpass()
 * inside the Target Thread and will do all the things to do (ttd)  
 * before entering the inner-most loop that does all the I/O operations
 * that constitute a "pass".
 * Return values: 0 is good
 *                1 is bad
 */
int32_t
xdd_target_ttd_before_pass(target_data_t *tdp) {
	worker_data_t	*wdp;	// Pointer to a QThread PTDS


	// Timer Calibration and Information
	xdd_timer_calibration_before_pass();

	// Process Start Delay
	xdd_start_delay_before_pass(tdp);

	// Lock Step Processing
	xdd_lockstep_before_pass(tdp);

	// Read-After_Write setup
	xdd_raw_before_pass(tdp);

	// End-to-End setup
	xdd_e2e_before_pass(tdp);

	xdd_init_ptds_before_pass(tdp);

	/* Initialize counters, barriers, clocks, ...etc */
	tdp->td_io_size = tdp->td_reqsize * tdp->td_block_size;

	/* Get the starting time stamp */
	if (tdp->td_tgtstp->my_current_pass_number == 1) { // For the *first* pass...
		if ((tdp->td_target_options & TO_ENDTOEND) && (tdp->td_target_options & TO_E2E_DESTINATION)) {  
			// Since the Destination Side starts and *waits* for the Source Side, the
			// actual "first pass start time" is set to a LARGE number so that it later
			// gets set to the time that the first packet of data was actually received.
			// That is done by the Results Manager at the end of a pass.
			tdp->td_first_pass_start_time = NCLK_MAX;
			tdp->td_tgtstp->my_pass_start_time = NCLK_MAX;
		} else { // This is either a non-E2E run or this is the Source Side of an E2E
			nclk_now(&tdp->td_first_pass_start_time);
			tdp->td_tgtstp->my_pass_start_time = tdp->td_first_pass_start_time;
		}
		// Get the current CPU user and system times 
		times(&tdp->td_tgtstp->my_starting_cpu_times_this_run);
		memcpy(&tdp->td_tgtstp->my_starting_cpu_times_this_pass,&tdp->td_tgtstp->my_starting_cpu_times_this_run, sizeof(struct tms));
	} else { // For pass number greater than 1
		if ((tdp->td_target_options & TO_ENDTOEND) && (tdp->td_target_options & TO_E2E_DESTINATION)) {
			// Same as above... Pass numbers greater than one will be used when the
			// multi-file copy support is added
			tdp->td_tgtstp->my_pass_start_time = NCLK_MAX;
		} else { // This is either a non-E2E run or this is the Source Side of an E2E
			nclk_now(&tdp->td_tgtstp->my_pass_start_time);
		}
		times(&tdp->td_tgtstp->my_starting_cpu_times_this_pass);
	}

	wdp = tdp->td_next_wdp;
	while (wdp) { // Set up the pass_start_times for all the QThreads 
		if (tdp->td_tgtstp->my_current_pass_number == 1) {
			wdp->wd_first_pass_start_time = tdp->td_first_pass_start_time;
			wdp->wd_pass_start_time = wdp->wd_first_pass_start_time;
			// Get the current CPU user and system times 
			times(&wdp->wd_starting_cpu_times_this_run);
			memcpy(&wdp->wd_starting_cpu_times_this_pass,&wdp->wd_starting_cpu_times_this_run, sizeof(struct tms));
		} else { 
			wdp->wd_pass_start_time = tdp->td_tgtstp->my_pass_start_time;
			times(&wdp->wd_starting_cpu_times_this_pass);
		}
		xdd_init_ptds_before_pass(tdp);
		wdp = wdp->wd_next_wdp;
	}

	return(0);
		
} // End of xdd_target_ttd_before_pass()

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
