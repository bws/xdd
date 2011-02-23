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
#include "xdd.h"

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
	pclk_t	t1,t2,t3;
#ifdef WIN32 
        int32_t i;
#else
        int 	status;
        int32_t i;
	struct timespec req;
	struct timespec rem;
#endif

	if (xgp->global_options & GO_TIMER_INFO) {
		pclk_now(&t1);
		for (i=0; i<100; i++) 
			pclk_now(&t2);
		pclk_now(&t2);
		t3=t2-t1;

		fprintf(xgp->output,"XDD Timer Calibration Info: Average time to get time stamp=%llu nanoseconds\n",t3/100);

#ifdef WIN32
		for (i=1; i< 1001; i*=10) {
			pclk_now(&t1);
			Sleep(i);
			pclk_now(&t3);
			t3 -= t1;
fprintf(xgp->output,"XDD Timer Calibration Info: Requested sleep time in microseconds=%d, Actual sleep time in microseconds=%llu\n",i*1000,t3/MILLION);
		}
#else // Do this for Systems other than Windows
		for (i=1; i< 1000001; i*=10) {
			req.tv_sec = 0;
			req.tv_nsec = i;
			rem.tv_sec = 0;
			rem.tv_nsec = 0;
			pclk_now(&t1);
			status = nanosleep(&req, &rem);
			pclk_now(&t3);
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
xdd_start_delay_before_pass(ptds_t *p) {
	int32_t 		sleep_time_dw;
	int				status;
	uint64_t 		tmp;	// Temp 
	struct timespec	req;	// The requested sleep time
	struct timespec	rem;	// The remaining sleep time is awoken early


	/* Check to see if this thread has a start delay time. If so, just hang out
	 * until the start delay time has elapsed and then spring into action!
	 * The start_delay time is stored as a pclk_t which is units of pico seconds.
	 */
	if (p->start_delay <= 0.0)
		return; // No start delay

	// Convert the start delay from nanoseconds to milliseconds
	sleep_time_dw = (int32_t)(p->start_delay_psec/BILLION);
#ifdef WIN32
	Sleep(sleep_time_dw);
#elif (IRIX )
	sginap((sleep_time_dw*CLK_TCK)/1000);
#else
	tmp = p->start_delay; // Number of *seconds*
	req.tv_sec = p->start_delay; // Whole seconds
	req.tv_nsec = (p->start_delay - req.tv_sec) * BILLION; // Fraction of a second
	fprintf(xgp->output, "\nTarget %d Beginning requested Start Delay for %f seconds before pass %d...\n",p->my_target_number, p->start_delay, p->my_current_pass_number);
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
	fprintf(xgp->output, "\nTarget %d Starting pass %d after requested delay of %f seconds\n",p->my_target_number,p->my_current_pass_number, p->start_delay);
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
xdd_raw_before_pass(ptds_t *p) {

	if ((p->target_options & TO_READAFTERWRITE) == 0)
		return;

	// Initialize the read-after-write variables
	p->raw_msg_sent = 0;
	p->raw_msg_recv = 0;
	p->raw_msg_last_sequence = 0;
	p->raw_msg.sequence = 0;
	p->raw_prev_loc = 0;
	p->raw_prev_len = 0;
	p->raw_data_ready = 0;
	p->raw_data_length = 0;

} // End of xdd_raw_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_e2e_before_pass() - This subroutine initializes the variables that
 * are used by the end-to-end option
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_e2e_before_pass(ptds_t *p) {

	if ((p->target_options & TO_ENDTOEND) == 0)
		return;

	// Initialize the read-after-write variables
	p->e2e_msg_sent = 0;
	p->e2e_msg_recv = 0;
	p->e2e_msg_sequence_number = 0;
	p->e2e_prev_loc = 0;
	p->e2e_prev_len = 0;
	p->e2e_data_length = 0;
	p->e2e_sr_time = 0;

} // End of xdd_e2e_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_init_ptds_before_pass() - Reset variables to known state
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void 
xdd_init_ptds_before_pass(ptds_t *p) {
    
	// Init all the pass-related variables to 0
	p->my_elapsed_pass_time = 0;
	p->my_first_op_start_time = 0;
	p->my_accumulated_op_time = 0; 
	p->my_accumulated_read_op_time = 0;
	p->my_accumulated_write_op_time = 0;
	p->my_accumulated_pattern_fill_time = 0;
	p->my_accumulated_flush_time = 0;
	//
	p->my_current_op_number = 0; 		// The current operation number init to 0
	p->my_current_op_count = 0; 		// The number of read+write operations that have completed so far
	p->my_current_read_op_count = 0;	// The number of read operations that have completed so far 
	p->my_current_write_op_count = 0;	// The number of write operations that have completed so far 
	p->my_current_bytes_xfered = 0;		// Total number of bytes transferred to far (to storage device, not network)
	p->my_current_bytes_read = 0;		// Total number of bytes read to far (from storage device, not network)
	p->my_current_bytes_written = 0;	// Total number of bytes written to far (to storage device, not network)
	p->my_current_byte_location = 0; 	// Current byte location for this I/O operation 
	p->my_current_io_status = 0; 				// I/O Status of the last I/O operation for this qthread
	p->my_current_io_errno = 0; 				// The errno associated with the status of this I/O for this thread
	p->my_error_break = 0; 			// This is set after an I/O Operation if the op failed in some way
	p->my_current_error_count = 0;		// The number of I/O errors for this qthread
	//
	// Longest and shortest op times - RESET AT THE START OF EACH PASS
	p->my_longest_op_time = 0;			// Longest op time that occured during this pass
	p->my_longest_op_number = 0; 		// Number of the operation where the longest op time occured during this pass
	p->my_longest_read_op_time = 0; 	// Longest read op time that occured during this pass
	p->my_longest_read_op_number = 0; 	// Number of the read operation where the longest op time occured during this pass
	p->my_longest_write_op_time = 0; 	// Longest write op time that occured during this pass
	p->my_longest_write_op_number = 0; 	// Number of the write operation where the longest op time occured during this pass
	p->my_shortest_op_time = PCLK_MAX; 	// Shortest op time that occurred during this pass
	p->my_shortest_op_number = 0; 		// Number of the operation where the shortest op time occured during this pass
	p->my_shortest_read_op_time = PCLK_MAX;	// Shortest read op time that occured during this pass
	p->my_shortest_read_op_number = 0; 	// Number of the read operation where the shortest op time occured during this pass
	p->my_shortest_write_op_time = PCLK_MAX;// Shortest write op time that occured during this pass
	p->my_shortest_write_op_number = 0;	// Number of the write operation where the shortest op time occured during this pass

	p->my_time_limit_expired = 0;		// The time limit expiration indicator

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
xdd_target_ttd_before_pass(ptds_t *p) {
	ptds_t	*qp;	// Pointer to a QThread PTDS


	// Timer Calibration and Information
	xdd_timer_calibration_before_pass();

	// Process Start Delay
	xdd_start_delay_before_pass(p);

	// Lock Step Processing
	xdd_lockstep_before_pass(p);

	// Read-After_Write setup
	xdd_raw_before_pass(p);

	// End-to-End setup
	xdd_e2e_before_pass(p);

	xdd_init_ptds_before_pass(p);

	/* Initialize counters, barriers, clocks, ...etc */
	p->iosize = p->reqsize * p->block_size;

	/* Get the starting time stamp */
	if (p->my_current_pass_number == 1) { // For the *first* pass...
		if ((p->target_options & TO_ENDTOEND) && (p->target_options & TO_E2E_DESTINATION)) {  
			// Since the Destination Side starts and *waits* for the Source Side, the
			// actual "first pass start time" is set to a LARGE number so that it later
			// gets set to the time that the first packet of data was actually received.
			// That is done by the Results Manager at the end of a pass.
			p->first_pass_start_time = PCLK_MAX;
			p->my_pass_start_time = PCLK_MAX;
		} else { // This is either a non-E2E run or this is the Source Side of an E2E
			pclk_now(&p->first_pass_start_time);
			p->my_pass_start_time = p->first_pass_start_time;
		}
		// Get the current CPU user and system times 
		times(&p->my_starting_cpu_times_this_run);
		memcpy(&p->my_starting_cpu_times_this_pass,&p->my_starting_cpu_times_this_run, sizeof(struct tms));
	} else { // For pass number greater than 1
		if ((p->target_options & TO_ENDTOEND) && (p->target_options & TO_E2E_DESTINATION)) {
			// Same as above... Pass numbers greater than one will be used when the
			// multi-file copy support is added
			p->my_pass_start_time = PCLK_MAX;
		} else { // This is either a non-E2E run or this is the Source Side of an E2E
			pclk_now(&p->my_pass_start_time);
		}
		times(&p->my_starting_cpu_times_this_pass);
	}

	qp = p->next_qp;
	while (qp) { // Set up the pass_start_times for all the QThreads 
		if (p->my_current_pass_number == 1) {
			qp->first_pass_start_time = p->first_pass_start_time;
			qp->my_pass_start_time = qp->first_pass_start_time;
			// Get the current CPU user and system times 
			times(&qp->my_starting_cpu_times_this_run);
			memcpy(&qp->my_starting_cpu_times_this_pass,&qp->my_starting_cpu_times_this_run, sizeof(struct tms));
		} else { 
			qp->my_pass_start_time = p->my_pass_start_time;
			times(&qp->my_starting_cpu_times_this_pass);
		}
		xdd_init_ptds_before_pass(qp);
		qp = qp->next_qp;
	}

	return(0);
		
} // End of xdd_target_ttd_before_pass()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
