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
#include "xint.h"
#include <unistd.h>

//******************************************************************************
// Things the Target Thread needs to do after a single pass 
//******************************************************************************

/*----------------------------------------------------------------------------*/
/* xdd_target_ttd_after_pass() - This subroutine will do all the stuff needed to 
 * be done after the inner-most loop has done does all the I/O operations
 * that constitute a "pass" or some portion of a pass if it terminated early.
 */
int32_t
xdd_target_ttd_after_pass(target_data_t *tdp) {
	int32_t  status;
	worker_data_t	*wdp;


	status = 0;
	// Issue an fdatasync() to flush all the write buffers to disk for this file if the -syncwrite option was specified
	if (tdp->td_target_options & TO_SYNCWRITE) {
#if (LINUX || AIX)
            status = fdatasync(tdp->td_file_desc);
#else
            status = fsync(tdp->td_file_desc);
#endif
        }
	/* Get the ending time stamp */
	nclk_now(&tdp->td_counters.tc_pass_end_time);
	tdp->td_counters.tc_pass_elapsed_time = tdp->td_counters.tc_pass_end_time - tdp->td_counters.tc_pass_start_time;

	/* Get the current CPU user and system times and the effective current wall clock time using nclk_now() */
	times(&tdp->td_counters.tc_current_cpu_times);

	// Loop through all the Worker Threads to put the Earliest Start Time and Latest End Time into this Target Data Struct
	wdp = tdp->td_next_wdp;
	while (wdp) {
		if (wdp->wd_counters.tc_time_first_op_issued_this_pass <= tdp->td_counters.tc_time_first_op_issued_this_pass) 
			tdp->td_counters.tc_time_first_op_issued_this_pass = wdp->wd_counters.tc_time_first_op_issued_this_pass;
		if (wdp->wd_counters.tc_pass_start_time <= tdp->td_counters.tc_pass_start_time) 
			tdp->td_counters.tc_pass_start_time = wdp->wd_counters.tc_pass_start_time;
		if (wdp->wd_counters.tc_pass_end_time >= tdp->td_counters.tc_pass_end_time) 
			tdp->td_counters.tc_pass_end_time = wdp->wd_counters.tc_pass_end_time;
		wdp = wdp->wd_next_wdp;
	}
	if (tdp->td_target_options & TO_ENDTOEND) { 
		// Average the Send/Receive Time 
		tdp->td_e2ep->e2e_sr_time /= tdp->td_queue_depth;
	}

	return(status);
} // End of xdd_target_ttd_after_pass()

 
