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
/*
 * This file contains the subroutines that perform various debugging functions 
 * that mainly display key data structures and things like that.
 */
#include "xint.h"

 
/*----------------------------------------------------------------------------*/
/* xdd_show_global_data() - Display values in the specified global data structure
 */
void
xdd_show_global_data(void) {
    fprintf(stderr,"\nxdd_show_global_data:********* Start of Global Data **********\n");
    fprintf(stderr,"xdd_show_global_data: global_options          0x%016llx - I/O Options valid for all targets \n",(long long int)xgp->global_options);
    fprintf(stderr,"xdd_show_global_data: progname                  %s - Program name from argv[0] \n",xgp->progname);
    fprintf(stderr,"xdd_show_global_data: argc                      %d - The original arg count \n",xgp->argc);
    fprintf(stderr,"xdd_show_global_data: max_errors                %lld - max number of errors to tollerate \n",(long long int)xgp->max_errors);
    fprintf(stderr,"xdd_show_global_data: max_errors_to_print       %lld - Maximum number of compare errors to print \n",(long long int)xgp->max_errors_to_print);
    fprintf(stderr,"xdd_show_global_data: output_filename          '%s' - name of the output file \n",(xgp->output_filename != NULL)?xgp->output_filename:"NA");
    fprintf(stderr,"xdd_show_global_data: errout_filename          '%s' - name fo the error output file \n",(xgp->errout_filename != NULL)?xgp->errout_filename:"NA");
    fprintf(stderr,"xdd_show_global_data: csvoutput_filename       '%s' - name of the csv output file \n",(xgp->csvoutput_filename != NULL)?xgp->csvoutput_filename:"NA");
    fprintf(stderr,"xdd_show_global_data: combined_output_filename '%s' - name of the combined output file \n",(xgp->combined_output_filename != NULL)?xgp->combined_output_filename:"NA");
    fprintf(stderr,"xdd_show_global_data: output                  0x%p - Output file pointer \n",xgp->output);
    fprintf(stderr,"xdd_show_global_data: errout                  0x%p - Error Output file pointer \n",xgp->errout);
    fprintf(stderr,"xdd_show_global_data: csvoutput               0x%p - Comma Separated Values output file \n",xgp->csvoutput);
    fprintf(stderr,"xdd_show_global_data: combined_output         0x%p - Combined output file \n",xgp->combined_output);
    fprintf(stderr,"xdd_show_global_data: id                       '%s' - ID string pointer \n",(xgp->id != NULL)?xgp->id:"NA");
    fprintf(stderr,"xdd_show_global_data: number_of_processors      %d - Number of processors \n",xgp->number_of_processors);
    fprintf(stderr,"xdd_show_global_data: clock_tick                %d - Number of clock ticks per second \n",xgp->clock_tick);
    fprintf(stderr,"xdd_show_global_data: id_firsttime              %d - ID first time through flag \n",xgp->id_firsttime);
    fprintf(stderr,"xdd_show_global_data: run_error_count_exceeded  %d - The alarm that goes off when the number of errors for this run has been exceeded \n",xgp->run_error_count_exceeded);
    fprintf(stderr,"xdd_show_global_data: run_time_expired          %d - The alarm that goes off when the total run time has been exceeded \n",xgp->run_time_expired);
    fprintf(stderr,"xdd_show_global_data: run_complete              %d - Set to a 1 to indicate that all passes have completed \n",xgp->run_complete);
    fprintf(stderr,"xdd_show_global_data: abort                     %d - abort the run due to some catastrophic failure \n",xgp->abort);
    fprintf(stderr,"xdd_show_global_data: random_initialized        %d - Random number generator has been initialized \n",xgp->random_initialized);
    fprintf(stderr,"xdd_show_global_data: ********* End of Global Data **********\n");

} /* end of xdd_show_global_data() */ 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_plan_data() - Display values in the specified global data structure
 */
void
xdd_show_plan_data(xdd_plan_t* planp) {
    int        target_number;    


    fprintf(stderr,"\nxdd_show_plan_data:********* Start of Plan Data **********\n");
    fprintf(stderr,"xdd_show_plan_data: passes                    %d - number of passes to perform \n",planp->passes);
    fprintf(stderr,"xdd_show_plan_data: pass_delay                %f - number of seconds to delay between passes \n",planp->pass_delay);
    fprintf(stderr,"xdd_show_plan_data: max_errors                %lld - max number of errors to tollerate \n",(long long int)xgp->max_errors);
    fprintf(stderr,"xdd_show_plan_data: max_errors_to_print       %lld - Maximum number of compare errors to print \n",(long long int)xgp->max_errors_to_print);
    fprintf(stderr,"xdd_show_plan_data: ts_binary_filename_prefix '%s' - timestamp binary filename prefix \n",(planp->ts_binary_filename_prefix != NULL)?planp->ts_binary_filename_prefix:"NA");
    fprintf(stderr,"xdd_show_plan_data: ts_output_filename_prefix '%s' - timestamp report output filename prefix \n",(planp->ts_output_filename_prefix != NULL)?planp->ts_output_filename_prefix:"NA");
    fprintf(stderr,"xdd_show_plan_data: restart_frequency         %d - seconds between restart monitor checks \n",planp->restart_frequency);
    fprintf(stderr,"xdd_show_plan_data: syncio                    %d - the number of I/Os to perform btw syncs \n",planp->syncio);
    fprintf(stderr,"xdd_show_plan_data: target_offset             %lld - offset value \n",(long long int)planp->target_offset);
    fprintf(stderr,"xdd_show_plan_data: number_of_targets         %d - number of targets to operate on \n",planp->number_of_targets);
    fprintf(stderr,"xdd_show_plan_data: number_of_iothreads       %d - number of threads spawned for all targets \n",planp->number_of_iothreads);
    fprintf(stderr,"xdd_show_plan_data: run_time                  %f - Length of time to run all targets  all passes \n",planp->run_time);
    fprintf(stderr,"xdd_show_plan_data: base_time                 %lld - The time that xdd was started - set during initialization \n",(long long int)planp->base_time);
    fprintf(stderr,"xdd_show_plan_data: run_start_time            %lld - The time that the targets start their first pass - set after initialization \n",(long long int)planp->run_start_time);
    fprintf(stderr,"xdd_show_plan_data: estimated_end_time        %lld - The time at which this run (all passes) should end \n",(long long int)planp->estimated_end_time);
    fprintf(stderr,"xdd_show_plan_data: number_of_processors      %d - Number of processors \n",planp->number_of_processors);
    fprintf(stderr,"xdd_show_plan_data: clock_tick                %d - Number of clock ticks per second \n",planp->clock_tick);
    fprintf(stderr,"xdd_show_plan_data: barrier_count             %d Number of barriers on the chain \n",planp->barrier_count);                         
    fprintf(stderr,"xdd_show_plan_data: format_string             '%s'\n",(planp->format_string != NULL)?planp->format_string:"NA");
    fprintf(stderr,"xdd_show_plan_data: results_header_displayed   %d\n",planp->results_header_displayed);
    fprintf(stderr,"xdd_show_plan_data: heartbeat_flags          0x%08x\n",planp->heartbeat_flags);

    fprintf(stderr,"xdd_show_plan_data: Target_Data pointers\n");
    for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
        fprintf(stderr,"xdd_show_plan_data:\tTarget_Data pointer for target %d of %d is 0x%p\n",target_number, planp->number_of_targets, planp->target_datap[target_number]);
    }
    fprintf(stderr,"xdd_show_plan_data: ********* End of Plan Data **********\n");

} /* end of xdd_show_plan_data() */ 

/*----------------------------------------------------------------------------*/
/* xdd_show_target_data() - Display values in the specified Per-Target-Data-Structure 
 */
void
xdd_show_target_data(target_data_t *tdp) {
    char option_string[1024];

    fprintf(stderr,"\nxdd_show_target_data:********* Start of TARGET_DATA 0x%p **********\n", tdp);

    fprintf(stderr,"xdd_show_target_data: struct xdd_plan         *td_planp=%p\n",tdp->td_planp);
    fprintf(stderr,"xdd_show_target_data: struct xint_worker_data *td_next_wdp=%p\n",tdp->td_next_wdp);          // Pointer to the first worker_data struct in the list
    fprintf(stderr,"xdd_show_target_data: pthread_t               td_thread\n");                                 // Handle for this Target Thread 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_thread_id=%d\n",tdp->td_thread_id);         // My system thread ID (like a process ID) 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_pid=%d\n",tdp->td_pid);                     // My process ID 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_target_number=%d\n",tdp->td_target_number); // My target number 
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_target_options=%llx\n",(unsigned long long int)tdp->td_target_options); // I/O Options specific to each target 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_file_desc=%d\n",tdp->td_file_desc);         // File Descriptor for the target device/file 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_open_flags=%x\n",tdp->td_open_flags);       // Flags used during open processing of a target
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_xfer_size=%d\n",tdp->td_xfer_size);         // Number of bytes per request 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_filetype=%d\n",tdp->td_filetype);           // Type of file: regular, device, socket, ... 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_filesize=%lld\n",(long long int)tdp->td_filesize);      // Size of target file in bytes 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_target_ops=%lld\n",(long long int)tdp->td_target_ops);  // Total number of ops to perform on behalf of a "target"
    fprintf(stderr,"xdd_show_target_data: seekhdr_t               td_seekhdr\n");          // For all the seek information 
    fprintf(stderr,"xdd_show_target_data: xint_timestamp_t        td_ts_table:\n");        // The timestamp table
	xdd_show_ts_table(&tdp->td_ts_table,tdp->td_target_number);
    fprintf(stderr,"xdd_show_target_data: xdd_occupant_t          td_occupant\n");         // Used by the barriers to keep track of what is in a barrier at any given time
    fprintf(stderr,"xdd_show_target_data: char                    td_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH]='%s'\n",tdp->td_occupant_name); // For a Target thread this is "TARGET####", for a Worker Thread it is "TARGET####WORKER####"
    fprintf(stderr,"xdd_show_target_data: xdd_barrier_t           *td_current_barrier=%p\n",tdp->td_current_barrier);  // Pointer to the current barrier this Thread is in at any given time or NULL if not in a barrier
    fprintf(stderr,"xdd_show_target_data: xdd_barrier_t           td_target_worker_thread_init_barrier\n");            // Where the Target Thread waits for the Worker Thread to initialize
    fprintf(stderr,"xdd_show_target_data: xdd_barrier_t           td_targetpass_worker_thread_passcomplete_barrier\n");// The barrier used to sync targetpass() with all the Worker Threads at the end of a pass
    fprintf(stderr,"xdd_show_target_data: xdd_barrier_t           td_targetpass_worker_thread_eofcomplete_barrier\n"); // The barrier used to sync targetpass_eof_desintation_side() with a Worker Thread trying to recv an EOF packet
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_current_bytes_issued=%lld\n",(long long int)tdp->td_current_bytes_issued);       // The amount of data for all transfer requests that has been issued so far 
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_current_bytes_completed=%lld\n",(long long int)tdp->td_current_bytes_completed); // The amount of data for all transfer requests that has been completed so far
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_current_bytes_remaining=%lld\n",(long long int)tdp->td_current_bytes_remaining); // Bytes remaining to be transferred 
    fprintf(stderr,"xdd_show_target_data: char                    td_abort=%d\n",tdp->td_abort);                       // Abort this operation (either a Worker Thread or a Target Thread)
    fprintf(stderr,"xdd_show_target_data: char                    td_time_limit_expired=%d\n",tdp->td_time_limit_expired); // The time limit for this target has expired
    fprintf(stderr,"xdd_show_target_data: pthread_mutex_t         td_current_state_mutex\n");                          // Mutex for locking when checking or updating the state info
	option_string[0]='\0';
    if (tdp->td_current_state & TARGET_CURRENT_STATE_INIT)
		strcat(option_string, "TARGET_CURRENT_STATE_INIT  ");
	if (tdp->td_current_state & TARGET_CURRENT_STATE_IO)
		strcat(option_string, "TARGET_CURRENT_STATE_IO  ");
	if (tdp->td_current_state & TARGET_CURRENT_STATE_BARRIER)
		strcat(option_string, "TARGET_CURRENT_STATE_BARRIER  ");
   	if (tdp->td_current_state & TARGET_CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE)
		strcat(option_string, "TARGET_CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE  ");
	if (tdp->td_current_state & TARGET_CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE)
		strcat(option_string,"TARGET_CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE  ");
	if (tdp->td_current_state & TARGET_CURRENT_STATE_PASS_COMPLETE)
		strcat(option_string, "TARGET_CURRENT_STATE_PASS_COMPLETE");
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_current_state=0x%08x: '%s'\n",tdp->td_current_state,option_string);            // State of this thread at any given time (see Current State definitions below)
    fprintf(stderr,"xdd_show_target_data: tot_t                   *td_totp=%p\n",tdp->td_totp);                                // Pointer to the target_offset_table for this target
    fprintf(stderr,"xdd_show_target_data: pthread_cond_t          td_any_worker_thread_available_condition\n");
    fprintf(stderr,"xdd_show_target_data: pthread_mutex_t         td_any_worker_thread_available_mutex\n");
    fprintf(stderr,"xdd_show_target_data: int                     td_any_worker_thread_available=%d\n",tdp->td_any_worker_thread_available);
    fprintf(stderr,"xdd_show_target_data: pthread_cond_t          td_this_wthread_is_available_condition\n");
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_start_offset=%lld\n",(long long int)tdp->td_start_offset);             // starting block offset value 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_pass_offset=%lld\n",(long long int)tdp->td_pass_offset);             // number of blocks to add to seek locations between passes 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_flushwrite=%lld\n",(long long int)tdp->td_flushwrite);              // number of write operations to perform between flushes 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_flushwrite_current_count=%lld\n",(long long int)tdp->td_flushwrite_current_count);  // Running number of write operations - used to trigger a flush (sync) operation 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_bytes=%lld\n",(long long int)tdp->td_bytes);                   // number of bytes to process overall 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_numreqs=%lld\n",(long long int)tdp->td_numreqs);                  // Number of requests to perform per pass per qthread
    fprintf(stderr,"xdd_show_target_data: double                  td_rwratio=%f\n",tdp->td_rwratio);                  // read/write ratios 
    fprintf(stderr,"xdd_show_target_data: nclk_t                  td_report_threshold=%lld\n",(unsigned long long int)tdp->td_report_threshold);        // reporting threshold for long operations 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_reqsize=%d\n",tdp->td_reqsize);                  // number of *blocksize* byte blocks per operation for each target 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_retry_count=%d\n",tdp->td_retry_count);              // number of retries to issue on an error 
    fprintf(stderr,"xdd_show_target_data: double                  td_time_limit=%f\n",tdp->td_time_limit);                // Time of a single pass in seconds
    fprintf(stderr,"xdd_show_target_data: nclk_t                  td_time_limit_ticks=%lld\n",(unsigned long long int)tdp->td_time_limit_ticks);        // Time of a single pass in high-res clock ticks
    fprintf(stderr,"xdd_show_target_data: char                    *td_target_directory=%s\n",tdp->td_target_directory);         // The target directory for the target 
    fprintf(stderr,"xdd_show_target_data: char                    *td_target_basename=%s\n",tdp->td_target_basename);         // Basename of the target/device
    fprintf(stderr,"xdd_show_target_data: char                    *td_target_full_pathname=%s\n",tdp->td_target_full_pathname);    // Fully qualified path name to the target device/file
    fprintf(stderr,"xdd_show_target_data: char                    td_target_extension[32]=%s\n",tdp->td_target_extension);     // The target extension number 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_processor=%d\n",tdp->td_processor);                  // Processor/target assignments 
    fprintf(stderr,"xdd_show_target_data: double                  td_start_delay=%f\n",tdp->td_start_delay);             // number of seconds to delay the start  of this operation 
    fprintf(stderr,"xdd_show_target_data: nclk_t                  td_start_delay_psec=%lld\n",(unsigned long long int)tdp->td_start_delay_psec);        // number of nanoseconds to delay the start  of this operation 
    fprintf(stderr,"xdd_show_target_data: char                    td_random_init_state[256]\n");     // Random number generator state initalizer array 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_block_size=%d\n",tdp->td_block_size);              // Size of a block in bytes for this target 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_queue_depth=%d\n",tdp->td_queue_depth);             // Command queue depth for each target 
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_preallocate=%lld\n",(long long int)tdp->td_preallocate);             // File preallocation value 
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_mem_align=%d\n",tdp->td_mem_align);               // Memory read/write buffer alignment value in bytes 
    fprintf(stderr,"xdd_show_target_data: struct heartbeat        td_hb=%p\n",tdp->td_planp);                    // Heartbeat data
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_target_bytes_to_xfer_per_pass=%lld\n",(long long int)tdp->td_target_bytes_to_xfer_per_pass);     // Number of bytes to xfer per pass for the entire target (all qthreads)
    fprintf(stderr,"xdd_show_target_data: int64_t                 td_last_committed_op=%lld\n",(long long int)tdp->td_last_committed_op);        // Operation number of last r/w operation relative to zero
    fprintf(stderr,"xdd_show_target_data: uint64_t                td_last_committed_location=%lld\n",(long long int)tdp->td_last_committed_location);    // Byte offset into target of last r/w operation
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_last_committed_length=%d\n",tdp->td_last_committed_length);    // Number of bytes transferred to/from last_committed_location
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_syncio_barrier_index=%d\n",tdp->td_syncio_barrier_index);         // Used to determine which syncio barrier to use at any given time
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_results_pass_barrier_index=%d\n",tdp->td_results_pass_barrier_index); // Where a Target thread waits for all other Target threads to complete a pass
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_results_display_barrier_index=%d\n",tdp->td_results_display_barrier_index); // Where threads wait for the results_manager()to display results
    fprintf(stderr,"xdd_show_target_data: int32_t                 td_results_run_barrier_index=%d\n",tdp->td_results_run_barrier_index);     // Where threads wait for all other threads at the completion of the run
    fprintf(stderr,"xdd_show_target_data: nclk_t                  td_open_start_time=%lld\n",(unsigned long long int)tdp->td_open_start_time);         // Time just before the open is issued for this target 
    fprintf(stderr,"xdd_show_target_data: nclk_t                  td_open_end_time=%lld\n",(unsigned long long int)tdp->td_open_end_time);             // Time just after the open completes for this target 
    fprintf(stderr,"xdd_show_target_data: pthread_mutex_t         td_counters_mutex\n");             // Mutex for locking when updating td_counters
    fprintf(stderr,"xdd_show_target_data: struct xint_target_counters td_counters\n");        // Pointer to the target counters
    fprintf(stderr,"xdd_show_target_data: struct xint_throttle    *td_throtp=%p\n",tdp->td_throtp);            // Pointer to the throttle sturcture
    fprintf(stderr,"xdd_show_target_data: struct xint_e2e         *td_e2ep=%p\n",tdp->td_e2ep);            // Pointer to the e2e struct when needed
    fprintf(stderr,"xdd_show_target_data: struct xint_extended_stats *td_esp=%p\n",tdp->td_esp);            // Extended Stats Structure Pointer
    fprintf(stderr,"xdd_show_target_data: struct xint_triggers     *td_trigp=%p\n",tdp->td_trigp);            // Triggers Structure Pointer
    fprintf(stderr,"xdd_show_target_data: struct xint_data_pattern *td_dpp=%p\n",tdp->td_dpp);            // Data Pattern Structure Pointer
    fprintf(stderr,"xdd_show_target_data: struct xint_raw          *td_rawp=%p\n",tdp->td_rawp);              // RAW Data Structure Pointer
    fprintf(stderr,"xdd_show_target_data: struct lockstep          *td_lsp=%p\n",tdp->td_lsp);            // Pointer to the lockstep structure used by the lockstep option
    fprintf(stderr,"xdd_show_target_data: struct xint_restart      *td_restartp=%p\n",tdp->td_restartp);        // Pointer to the restart structure used by the restart monitor
    fprintf(stderr,"xdd_show_target_data: struct stat              td_statbuf\n");            // Target File Stat buffer used by xdd_target_open()
    fprintf(stderr,"xdd_show_target_data: int32_t                  td_op_delay=%d\n",tdp->td_op_delay);         // Number of seconds to delay between operations 
    fprintf(stderr,"xdd_show_target_data:********* End of TARGET_DATA %p **********\n", tdp);
} /* end of xdd_show_target_data() */ 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_worker_data() - Display values in the specified data structure
 */
void
xdd_show_worker_data(worker_data_t *wdp) {
    char option_string[1024];

    fprintf(stderr,"\nxdd_show_worker_data:********* Start of Worker Data at 0x%p **********\n",wdp);

    fprintf(stderr,"xdd_show_worker_data: target_data_t           *wd_tdp=%p\n",wdp->wd_tdp);             // Pointer back to the Parent Target Data
    fprintf(stderr,"xdd_show_worker_data: struct xint_worker_data *wd_next_wdp=%p\n",wdp->wd_next_wdp);        // Pointer to the next worker_data struct in the list
    fprintf(stderr,"xdd_show_worker_data: pthread_t               wd_thread=%p\n",wdp->wd_next_wdp);            // Handle for this Worker_Thread
    fprintf(stderr,"xdd_show_worker_data: int32_t                 wd_worker_number=%d\n",wdp->wd_worker_number);    // My worker number within this target relative to 0
    fprintf(stderr,"xdd_show_worker_data: int32_t                 wd_thread_id=%d\n",wdp->wd_thread_id);          // My system thread ID (like a process ID) 
    fprintf(stderr,"xdd_show_worker_data: int32_t                 wd_pid=%d\n",wdp->wd_pid);               // My process ID 
    fprintf(stderr,"xdd_show_worker_data: unsigned char           *wd_bufp=%p\n",wdp->wd_bufp);            // Pointer to the generic I/O buffer
    fprintf(stderr,"xdd_show_worker_data: int                     wd_buf_size=%d\n",wdp->wd_buf_size);        // Size in bytes of the generic I/O buffer
    fprintf(stderr,"xdd_show_worker_data: int64_t                 wd_ts_entry=%lld\n",(long long int)wdp->wd_next_wdp);        // The TimeStamp entry to use when time-stamping an operation

    fprintf(stderr,"xdd_show_worker_data: struct xint_task        wd_task:\n");        
    xdd_show_task(&wdp->wd_task);

    fprintf(stderr,"xdd_show_worker_data: struct xint_target_counters  wd_counters:\n");        
    xdd_show_target_counters(&wdp->wd_counters);

    fprintf(stderr,"xdd_show_worker_data: pthread_mutex_t         wd_worker_thread_target_sync_mutex\n");    // Used to serialize access to the Worker_Thread-Target Synchronization flags

	option_string[0]='\0';
	if (wdp->wd_worker_thread_target_sync & WTSYNC_AVAILABLE)
		strcat(option_string,"WTSYNC_AVAILABLE ");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_BUSY)
		strcat(option_string,"WTSYNC_BUSY ");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_TARGET_WAITING)
		strcat(option_string,"WTSYNC_BUSY ");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_EOF_RECEIVED)
   		strcat(option_string,"WTSYNC_EOF_RECEIVED ");
    fprintf(stderr,"xdd_show_worker_data: int32_t                 wd_worker_thread_target_sync=0x%08x:%s\n",wdp->wd_worker_thread_target_sync,option_string);        // Flags used to synchronize a Worker_Thread with its Target
    fprintf(stderr,"xdd_show_worker_data: pthread_cond_t          wd_this_worker_thread_is_available_condition\n");
    fprintf(stderr,"xdd_show_worker_data: xdd_barrier_t           *wd_current_barrier:%p: '%s'\n",wdp->wd_current_barrier, wdp->wd_current_barrier?wdp->wd_current_barrier->name:"NA");    // The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
    fprintf(stderr,"xdd_show_worker_data: xdd_barrier_t           wd_thread_targetpass_wait_for_task_barrier:\n");    // The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
    fprintf(stderr,"xdd_show_worker_data: xdd_occupant_t          wd_occupant:\n");        // Used by the barriers to keep track of what is in a barrier at any given time
    xdd_show_occupant(&wdp->wd_occupant);
    fprintf(stderr,"xdd_show_worker_data: char                    wd_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH]='%s'\n",wdp->wd_occupant_name);    // For a Target thread this is "TARGET####", for a Worker_Thread it is "TARGET####WORKER####"
    fprintf(stderr,"xdd_show_worker_data: xint_e2e_t              *wd_e2ep=%p\n",wdp->wd_e2ep);            // Pointer to the e2e struct when needed
    fprintf(stderr,"xdd_show_worker_data: xdd_sgio_t              *wd_sgiop=%p\n",wdp->wd_sgiop);            // SGIO Structure Pointer
    
	option_string[0]='\0';
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_INIT)
		strcat(option_string,"WORKER_CURRENT_STATE_INIT ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_IO)
		strcat(option_string,"WORKER_CURRENT_STATE_IO ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_DEST_RECEIVE)
		strcat(option_string,"WORKER_CURRENT_STATE_DEST_RECEIVE ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_SRC_SEND)
		strcat(option_string,"WORKER_CURRENT_STATE_SRC_SEND ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_BARRIER)
		strcat(option_string,"WORKER_CURRENT_STATE_BARRIER ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE)
		strcat(option_string,"WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE)
		strcat(option_string,"WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS)
		strcat(option_string,"WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS ");
	if(wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO)
		strcat(option_string,"WORKER_CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO ");
    fprintf(stderr,"xdd_show_worker_data: uint32_t                 wd_current_state=0x%08x: '%s'\n",wdp->wd_current_state,option_string);            // State of this thread at any given time (see Current State definitions below)

    fprintf(stderr,"xdd_show_worker_data:********* End of Worker Data **********\n");

} // End of xdd_show_worker_data() 

/*----------------------------------------------------------------------------*/
/* xdd_show_task() - Display values in the specified data structure
 * The xint_task structure is used to pass task parameters to a Worker Thread
 */
void
xdd_show_task(xint_task_t *taskp) {
    char *sp;    // String pointer

    fprintf(stderr,"\nxdd_show_task:********* Start of TASK Data at 0x%p **********\n",taskp);
    switch (taskp->task_request) {
        case TASK_REQ_IO:
              sp="TASK_REQ_IO";
            break;
        case TASK_REQ_REOPEN:
              sp="TASK_REQ_REOPEN";
            break;
        case TASK_REQ_STOP:
              sp="TASK_REQ_STOP";
            break;
        case TASK_REQ_EOF:
              sp="TASK_REQ_EOF";
            break;
        default: 
            sp="UNDEFINED TASK";
            break;
    }
    fprintf(stderr,"\txdd_show_task: char          task_request=%d:%s\n",taskp->task_request,sp);            // Type of Task to perform
    fprintf(stderr,"\txdd_show_task: int           task_file_desc=%d\n",taskp->task_request);                // File Descriptor
    fprintf(stderr,"\txdd_show_task: unsigned char *task_datap=%p\n",taskp->task_datap);                    // The data section of the I/O buffer
    fprintf(stderr,"\txdd_show_task: char          task_op_type=%d\n",taskp->task_op_type);                // Operation to perform
    fprintf(stderr,"\txdd_show_task: char          *task_op_string='%s'\n",taskp->task_op_string);            // Operation to perform in ASCII
    fprintf(stderr,"\txdd_show_task: uint64_t      task_op_number=%lld\n",(unsigned long long int)taskp->task_op_number);    // Offset into the file where this transfer starts
    fprintf(stderr,"\txdd_show_task: size_t        task_xfer_size=%d\n",(int)taskp->task_xfer_size);                // Number of bytes to transfer
    fprintf(stderr,"\txdd_show_task: off_t         task_byte_offset=%lld\n",(long long int)taskp->task_byte_offset);            // Offset into the file where this transfer starts
    fprintf(stderr,"\txdd_show_task: uint64_t      task_e2e_sequence_number=%lld\n",(long long int)taskp->task_e2e_sequence_number);    // Sequence number of this task when part of an End-to-End operation
    fprintf(stderr,"\txdd_show_task: nclk_t        task_time_to_issue=%lld\n",(unsigned long long int)taskp->task_time_to_issue);            // Time to issue the I/O operation or 0 if not used
    fprintf(stderr,"\txdd_show_task: ssize_t       task_io_status=%d\n",(int)taskp->task_io_status);                // Returned status of this I/O associated with this task
    fprintf(stderr,"\txdd_show_task: int32_t       task_errno=%d\n",taskp->task_errno);                    // Returned errno of this I/O associated with this task
    fprintf(stderr,"xdd_show_task:********* End of TASK Data at 0x%p **********\n",taskp);
} // End of xdd_show_task()

/*----------------------------------------------------------------------------*/
/* xdd_show_occupant() - Display values in the specified data structure
 */
void
xdd_show_occupant(xdd_occupant_t *op) {
    fprintf(stderr,"\nxdd_show_occupant:********* Start of Occupant Data at 0x%p **********\n",op);

    fprintf(stderr,"\txdd_show_occupant: struct xdd_occupant   *prev_occupant=%p\n",op->prev_occupant);    // Previous occupant on the chain
    fprintf(stderr,"\txdd_show_occupant: struct xdd_occupant   *next_occupant=%p\n",op->prev_occupant);    // Next occupant on the chain
    fprintf(stderr,"\txdd_show_occupant: uint32_t              occupant_type=0x%08x\n",op->occupant_type);    // Bitfield that indicates the type of occupant
    fprintf(stderr,"\txdd_show_occupant: char                  *occupant_name='%s'\n",op->occupant_name);    // Pointer to a character string that is the name of this occupant
    fprintf(stderr,"\txdd_show_occupant: void                  *occupant_data=%p\n",op->occupant_data);    // Pointer to a Target_Data or Worker_Data if the occupant_type is a Target or Worker Thread
    fprintf(stderr,"\txdd_show_occupant: nclk_t                entry_time=%lld\n",(unsigned long long int)op->entry_time);        // Time stamp of when this occupant entered a barrier - filled in by xdd_barrier()
    fprintf(stderr,"\txdd_show_occupant: nclk_t                exit_time=%lld\n",(unsigned long long int)op->exit_time);        // Time stamp of when this occupant was released from a barrier - filled in by xdd_barrier()

    fprintf(stderr,"xdd_show_occupant:********* End of Occupant Data at 0x%p **********\n",op);
} // End of xdd_show_occupant()

/*----------------------------------------------------------------------------*/
/* xdd_show_target_counters() - Display values in the specified data structure
 */
void
xdd_show_target_counters(xint_target_counters_t *tcp) {
    fprintf(stderr,"\nxdd_show_target_counters:********* Start of Target Counters Data at 0x%p **********\n",tcp);

    // Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
    fprintf(stderr,"\txdd_show_target_counters: int32_t    tc_pass_number=%d\n",(int)tcp->tc_pass_number);                 // Current pass number 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_pass_start_time=%lld\n",(unsigned long long int)tcp->tc_pass_start_time);             // The time this pass started but before the first operation is issued
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_pass_end_time=%lld\n",(unsigned long long int)tcp->tc_pass_end_time);                 // The time stamp that this pass ended 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_pass_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_pass_elapsed_time);             // Time between the start and end of this pass
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_time_first_op_issued_this_pass=%lld\n",(unsigned long long int)tcp->tc_time_first_op_issued_this_pass); // Time this Worker Thread was able to issue its first operation for this pass
    fprintf(stderr,"\txdd_show_target_counters: struct tms tc_starting_cpu_times_this_run:\n");    // CPU times from times() at the start of this run
    fprintf(stderr,"\txdd_show_target_counters: struct tms tc_starting_cpu_times_this_pass:\n");// CPU times from times() at the start of this pass
    fprintf(stderr,"\txdd_show_target_counters: struct tms tc_current_cpu_times:\n");            // CPU times from times()

    // Updated by the Worker Thread in Worker Data during an I/O operation
    // and then copied to the Target Data strcuture at the completion of an I/O operation
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_current_byte_offset=%lld\n",(unsigned long long int)tcp->tc_current_byte_offset);         // Current byte location for this I/O operation 
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_current_op_number=%lld\n",(unsigned long long int)tcp->tc_current_op_number);            // Current I/O operation number 
    fprintf(stderr,"\txdd_show_target_counters: int32_t    tc_current_io_status=%d\n",(int)tcp->tc_current_io_status);             // I/O Status of the last I/O operation for this Worker Thread
    fprintf(stderr,"\txdd_show_target_counters: int32_t    tc_current_io_errno=%d\n",(int)tcp->tc_current_io_errno);             // The errno associated with the status of this I/O for this thread
    fprintf(stderr,"\txdd_show_target_counters: int32_t    tc_current_bytes_xfered_this_op=%d\n",(int)tcp->tc_current_bytes_xfered_this_op);// I/O Status of the last I/O operation for this Worker Thread
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_current_error_count=%lld\n",(unsigned long long int)tcp->tc_current_error_count);            // The number of I/O errors for this Worker Thread
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_op_start_time=%lld\n",(unsigned long long int)tcp->tc_current_op_start_time);         // Start time of the current op
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_op_end_time=%lld\n",(unsigned long long int)tcp->tc_current_op_end_time);         // End time of the current op
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_op_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_current_op_elapsed_time);        // Elapsed time of the current op
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_net_start_time=%lld\n",(unsigned long long int)tcp->tc_current_net_start_time);         // Start time of the current network op (e2e only)
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_net_end_time=%lld\n",(unsigned long long int)tcp->tc_current_net_end_time);         // End time of the current network op (e2e only)
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_current_net_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_current_net_elapsed_time);    // Elapsed time of the current network op (e2e only)

    // Accumulated Counters 
    // Updated in the Target Data structure by each Worker Thread at the completion of an I/O operation
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_op_count);         // The number of read+write operations that have completed so far
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_read_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_read_op_count);    // The number of read operations that have completed so far 
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_write_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_write_op_count);    // The number of write operations that have completed so far 
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_noop_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_noop_op_count);    // The number of noops that have completed so far 
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_bytes_xfered=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_xfered);    // Total number of bytes transferred so far (to storage device, not network)
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_bytes_read=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_read);        // Total number of bytes read so far (from storage device, not network)
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_bytes_written=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_written);    // Total number of bytes written so far (to storage device, not network)
    fprintf(stderr,"\txdd_show_target_counters: uint64_t   tc_accumulated_bytes_noop=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_noop);        // Total number of bytes processed by noops so far
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_op_time);         // Accumulated time spent in I/O 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_read_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_read_op_time);     // Accumulated time spent in read 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_write_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_write_op_time);    // Accumulated time spent in write 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_noop_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_noop_op_time);    // Accumulated time spent in noops 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_pattern_fill_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_pattern_fill_time); // Accumulated time spent in data pattern fill before all I/O operations 
    fprintf(stderr,"\txdd_show_target_counters: nclk_t     tc_accumulated_flush_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_flush_time);         // Accumulated time spent doing flush (fsync) operations

    fprintf(stderr,"xdd_show_target_counters:********* End of Target Counters Data at 0x%p **********\n",tcp);
} // End of xdd_show_target_counters()

/*----------------------------------------------------------------------------*/
/* xdd_show_e2e() - Display values in the specified data structure
 */
void
xdd_show_e2e(xint_e2e_t *e2ep) {
    fprintf(stderr,"\nxdd_show_e2e:********* Start of E2E Data at 0x%p **********\n",e2ep);
    fprintf(stderr,"\txdd_show_e2e: char       *e2e_dest_hostname='%s'\n",e2ep->e2e_dest_hostname);     // Name of the Destination machine 
    fprintf(stderr,"\txdd_show_e2e: char       *e2e_src_hostname='%s'\n",e2ep->e2e_dest_hostname);         // Name of the Source machine 
    fprintf(stderr,"\txdd_show_e2e: char       *e2e_src_file_path='%s'\n",e2ep->e2e_dest_hostname);     // Full path of source file for destination restart file 
    fprintf(stderr,"\txdd_show_e2e: time_t     e2e_src_file_mtime\n");     // stat -c %Y *e2e_src_file_path, i.e., last modification time
    fprintf(stderr,"\txdd_show_e2e: in_addr_t  e2e_dest_addr=%d\n",e2ep->e2e_dest_addr);          // Destination Address number of the E2E socket 
    fprintf(stderr,"\txdd_show_e2e: in_port_t  e2e_dest_port=%d\n",e2ep->e2e_dest_port);          // Port number to use for the E2E socket 
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_sd=%d\n",e2ep->e2e_sd);                   // Socket descriptor for the E2E message port 
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_nd=%d\n",e2ep->e2e_nd);                   // Number of Socket descriptors in the read set 
    fprintf(stderr,"\txdd_show_e2e: sd_t       e2e_csd[FD_SETSIZE]\n");;    // Client socket descriptors 
    fprintf(stderr,"\txdd_show_e2e: fd_set     e2e_active?\n");              // This set contains the sockets currently active 
    fprintf(stderr,"\txdd_show_e2e: fd_set     e2e_readset?\n");             // This set is passed to select() 
    fprintf(stderr,"\txdd_show_e2e: struct sockaddr_in  e2e_sname?\n");                 // used by setup_server_socket 
    fprintf(stderr,"\txdd_show_e2e: uint32_t   e2e_snamelen=%d\n",e2ep->e2e_snamelen);             // the length of the socket name 
    fprintf(stderr,"\txdd_show_e2e: struct sockaddr_in  e2e_rname?\n");                 // used by destination machine to remember the name of the source machine 
    fprintf(stderr,"\txdd_show_e2e: uint32_t   e2e_rnamelen=%d\n",e2ep->e2e_rnamelen);             // the length of the source socket name 
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_current_csd=%d\n",e2ep->e2e_current_csd);         // the current csd used by the select call on the destination side
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_next_csd=%d\n",e2ep->e2e_next_csd);             // The next available csd to use 
    fprintf(stderr,"\txdd_show_e2e: xdd_e2e_header_t *e2e_hdrp=%p\n",e2ep->e2e_hdrp);                // Pointer to the header portion of a packet
	if (e2ep->e2e_hdrp) xdd_show_e2e_header(e2ep->e2e_hdrp);
    fprintf(stderr,"\txdd_show_e2e: unsigned char *e2e_datap=%p\n",e2ep->e2e_datap);                // Pointer to the data portion of a packet
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_header_size=%d\n",e2ep->e2e_header_size);         // Size of the header portion of the buffer 
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_data_size=%d\n",e2ep->e2e_data_size);             // Size of the data portion of the buffer
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_xfer_size=%d\n",e2ep->e2e_xfer_size);             // Number of bytes per End to End request - size of data buffer plus size of E2E Header
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_send_status=%d\n",e2ep->e2e_send_status);         // Current Send Status
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_recv_status=%d\n",e2ep->e2e_recv_status);         // Current Recv status
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_msg_sequence_number=%lld\n",(long long int)e2ep->e2e_msg_sequence_number);// The Message Sequence Number of the most recent message sent or to be received
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_msg_sent=%d\n",e2ep->e2e_msg_sent);             // The number of messages sent 
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_msg_recv=%d\n",e2ep->e2e_msg_recv);             // The number of messages received 
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_prev_loc=%lld\n",(long long int)e2ep->e2e_prev_loc);             // The previous location from a e2e message from the source 
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_prev_len=%lld\n",(long long int)e2ep->e2e_prev_len);             // The previous length from a e2e message from the source 
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_data_recvd=%lld\n",(long long int)e2ep->e2e_data_recvd);         // The amount of data that is received each time we call xdd_e2e_dest_recv()
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_data_length=%lld\n",(long long int)e2ep->e2e_data_length);         // The amount of data that is ready to be read for this operation 
    fprintf(stderr,"\txdd_show_e2e: int64_t    e2e_total_bytes_written=%lld\n",(long long int)e2ep->e2e_total_bytes_written); // The total amount of data written across all restarts for this file
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_wait_1st_msg=%lld\n",(unsigned long long int)e2ep->e2e_wait_1st_msg);        // Time in nanosecs destination waited for 1st source data to arrive 
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_first_packet_received_this_pass=%lld\n",(unsigned long long int)e2ep->e2e_first_packet_received_this_pass);// Time that the first packet was received by the destination from the source
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_last_packet_received_this_pass=%lld\n",(unsigned long long int)e2ep->e2e_last_packet_received_this_pass);// Time that the last packet was received by the destination from the source
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_first_packet_received_this_run=%lld\n",(unsigned long long int)e2ep->e2e_first_packet_received_this_run);// Time that the first packet was received by the destination from the source
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_last_packet_received_this_run=%lld\n",(unsigned long long int)e2ep->e2e_last_packet_received_this_run);// Time that the last packet was received by the destination from the source
    fprintf(stderr,"\txdd_show_e2e: nclk_t     e2e_sr_time=%lld\n",(unsigned long long int)e2ep->e2e_sr_time);             // Time spent sending or receiving data for End-to-End operation
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_address_table_host_count=%d\n",e2ep->e2e_address_table_host_count);    // Cumulative number of hosts represented in the e2e address table
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_address_table_port_count=%d\n",e2ep->e2e_address_table_port_count);    // Cumulative number of ports represented in the e2e address table
    fprintf(stderr,"\txdd_show_e2e: int32_t    e2e_address_table_next_entry=%d\n",e2ep->e2e_address_table_next_entry);    // Next available entry in the e2e_address_table
    fprintf(stderr,"\txdd_show_e2e: xdd_e2e_ate_t e2e_address_table[E2E_ADDRESS_TABLE_ENTRIES]\n"); // Used by E2E to stripe over multiple IP Addresses
    fprintf(stderr,"xdd_show_e2e:********* End of E2E Data at 0x%p **********\n",e2ep);
} // End of xdd_show_e2e()

/*----------------------------------------------------------------------------*/
/* xdd_show_e2e_header() - Display values in the specified data structure
 */
void
xdd_show_e2e_header(xdd_e2e_header_t *e2ehp) {
    fprintf(stderr,"\nxdd_show_e2e_header:********* Start of E2E Header Data at 0x%p **********\n",e2ehp);
    fprintf(stderr,"\t\txdd_show_e2e_header: uint32_t   e2eh_magic=0x%8x\n",e2ehp->e2eh_magic);                 // Magic Number - sanity check
    fprintf(stderr,"\t\txdd_show_e2e_header: int32_t    e2eh_worker_thread_number=%d\n",e2ehp->e2eh_worker_thread_number);  // Sender's Worker Thread Number
    fprintf(stderr,"\t\txdd_show_e2e_header: int64_t    e2eh_sequence_number=%lld\n",(long long int)e2ehp->e2eh_sequence_number);       // Sequence number of this operation
    fprintf(stderr,"\t\txdd_show_e2e_header: nclk_t     e2eh_send_time=%lld\n",(unsigned long long int)e2ehp->e2eh_send_time);             // Time this packet was sent in global nano seconds
    fprintf(stderr,"\t\txdd_show_e2e_header: nclk_t     e2eh_recv_time=%lld\n",(unsigned long long int)e2ehp->e2eh_recv_time);             // Time this packet was received in global nano seconds
    fprintf(stderr,"\t\txdd_show_e2e_header: int64_t    e2eh_byte_offset=%lld\n",(long long int)e2ehp->e2eh_byte_offset);           // Offset relative to the beginning of the file of where this data belongs
    fprintf(stderr,"\t\txdd_show_e2e_header: int64_t    e2eh_data_length=%lld\n",(long long int)e2ehp->e2eh_data_length);           // Length of the user data in bytes for this operation
    fprintf(stderr,"\txdd_show_e2e_header:********* End of E2E Header Data at 0x%p **********\n",e2ehp);

} // End of xdd_show_e2e_header()

/*----------------------------------------------------------------------------*/
/* xdd_show_tot_entry() - Display values in the specified data structure
 */
void
xdd_show_tot_entry(tot_t *totp, int i) {
	tot_wait_t	*totwp;


  	fprintf(stderr,"\txdd_show_tot_entry:---------- TOT %p entry %d ----------\n",totp,i);
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> pthread_mutex_t tot_mutex\n",i);		// Mutex that is locked when updating items in this entry
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> pthread_cond_t tot_condition\n",i);
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> nclk_t tot_wait_ts=%lld\n",i,(long long int)totp->tot_entry[i].tot_wait_ts);			// Time that another Worker Thread starts to wait on this
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> nclk_t tot_post_ts=%lld\n",i,(long long int)totp->tot_entry[i].tot_post_ts);			// Time that the responsible Worker Thread posts this semaphore
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> nclk_t tot_update_ts=%lld\n",i,(long long int)totp->tot_entry[i].tot_update_ts);		// Time that the responsible Worker Thread updates the byte_location and io_size
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int64_t tot_byte_offset=%lld\n",i,(long long int)totp->tot_entry[i].tot_byte_offset);	// Byte Location that was just processed
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int64_t tot_op_number=%lld\n",i,(long long int)totp->tot_entry[i].tot_op_number);		// Target Operation Number for the op that processed this block
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int32_t tot_io_size=%d\n",i,totp->tot_entry[i].tot_io_size);							// Size of I/O in bytes that was just processed
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int32_t tot_wait_worker_thread_number=%d\n",i,totp->tot_entry[i].tot_wait_worker_thread_number);	// Number of the Worker Thread that is waiting for this TOT entry to be posted
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int32_t tot_post_worker_thread_number=%d\n",i,totp->tot_entry[i].tot_post_worker_thread_number);	// Number of the Worker Thread that posted this TOT entry 
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int32_t tot_update_worker_thread_number=%d\n",i,totp->tot_entry[i].tot_update_worker_thread_number);	// Number of the Worker Thread that last updated this TOT Entry
   	fprintf(stderr,"\txdd_show_tot_entry: <%d> int32_t status=%d\n",i,totp->tot_entry[i].tot_status);

	if (totp->tot_entry[i].tot_waitp) {
		fprintf(stderr,"\t\txdd_show_tot_entry: <%d> TOT WAIT Chain\n",i);
		totwp = totp->tot_entry[i].tot_waitp;
		while (totwp) {
			fprintf(stderr,"\t\txdd_show_tot_entry: ----------------------------------\n");
   			fprintf(stderr,"\t\txdd_show_tot_entry: <%d> struct tot_wait *tot_waitp=%p\n",i,totp->tot_entry[i].tot_waitp);
   			fprintf(stderr,"\t\t***xdd_show_tot_entry: <%d> tot_wait_t *totw_nextp=%p\n",i,totwp->totw_nextp);
			fprintf(stderr,"\t\t***xdd_show_tot_entry: <%d> struct xint_worker_data	*totw_wdp=%p\n",i,totwp->totw_wdp);		// Pointer to the Worker that owns this tot_wait
    		fprintf(stderr,"\t\t***xdd_show_tot_entry: <%d> pthread_cond_t totw_condition\n",i);; 
    		fprintf(stderr,"\t\t***xdd_show_tot_entry: <%d> int totw_is_released=%d\n",i,totwp->totw_is_released);
			totwp = totwp->totw_nextp;
		}
	}



} // End of xdd_show_tot_entry()

/*----------------------------------------------------------------------------*/
/* xdd_show_tot() - Display values in the specified data structure
 */
void
xdd_show_tot(tot_t *totp) {
	int	i;


    fprintf(stderr,"\nxdd_show_tot:********* Start of TOT Data at 0x%p **********\n",totp);
	fprintf(stderr,"xdd_show_tot: int tot_entries=%d \n",totp->tot_entries); 			// Number of tot entries
	for (i=0; i<totp->tot_entries; i++) 
		xdd_show_tot_entry(totp,i);
    fprintf(stderr,"xdd_show_tot:********* End of TOT Data at 0x%p **********\n",totp);

} // End of xdd_show_tot()

/*----------------------------------------------------------------------------*/
/* xdd_show_ts_table() - Display values in the specified data structure
 */
void
xdd_show_ts_table(xint_timestamp_t *ts_tablep, int target_number) {
	char option_string[256];


    fprintf(stderr,"\nxdd_show_ts_table:********* Start of TS TABLE for Target %d **********\n",target_number);

#define TS_NORMALIZE          0x00000001 /**< Time stamping normalization of output*/
#define TS_ON                 0x00000002 /**< Time stamping is ON */
#define TS_SUMMARY            0x00000004 /**< Time stamping Summary reporting */
#define TS_DETAILED           0x00000008 /**< Time stamping Detailed reporting */
#define TS_APPEND             0x00000010 /**< Time stamping Detailed reporting */
#define TS_DUMP               0x00000020 /**< Time stamping Dumping */
#define TS_WRAP               0x00000040 /**< Wrap the time stamp buffer */
#define TS_ONESHOT            0x00000080 /**< Stop time stamping at the end of the buffer */
#define TS_STOP               0x00000100 /**< Stop time stamping  */
#define TS_ALL                0x00000200 /**< Time stamp all operations */
#define TS_TRIGTIME           0x00000400 /**< Time stamp trigger time */
#define TS_TRIGOP             0x00000800 /**< Time stamp trigger operation number */
#define TS_TRIGGERED          0x00001000 /**< Time stamping has been triggered */
#define TS_SUPPRESS_OUTPUT    0x00002000 /**< Suppress timestamp output */
#define DEFAULT_TS_OPTIONS 0x00000000
	option_string[0]='\0';
	if (ts_tablep->ts_options & TS_NORMALIZE)
		strcat(option_string,"TS_NORMALIZE ");
	if (ts_tablep->ts_options & TS_ON)
		strcat(option_string,"TS_ON ");
	if (ts_tablep->ts_options & TS_SUMMARY)
		strcat(option_string,"TS_SUMMARY ");
	if (ts_tablep->ts_options & TS_DETAILED)
		strcat(option_string,"TS_DETAILED ");
	if (ts_tablep->ts_options & TS_APPEND)
		strcat(option_string,"TS_APPEND ");
	if (ts_tablep->ts_options & TS_DUMP)
		strcat(option_string,"TS_DUMP ");
	if (ts_tablep->ts_options & TS_WRAP)
		strcat(option_string,"TS_WRAP ");
	if (ts_tablep->ts_options & TS_ONESHOT)
		strcat(option_string,"TS_ONESHOT ");
	if (ts_tablep->ts_options & TS_STOP)
		strcat(option_string,"TS_STOP ");
	if (ts_tablep->ts_options & TS_ALL)
		strcat(option_string,"TS_ALL ");
	if (ts_tablep->ts_options & TS_TRIGTIME)
		strcat(option_string,"TS_TRIGTIME ");
	if (ts_tablep->ts_options & TS_TRIGOP)
		strcat(option_string,"TS_TRIGOP ");
	if (ts_tablep->ts_options & TS_TRIGGERED)
		strcat(option_string,"TS_TRIGGERED ");
	if (ts_tablep->ts_options & TS_SUPPRESS_OUTPUT)
		strcat(option_string,"TS_SUPPRESS_OUTPUT ");
	fprintf(stderr,"xdd_show_ts_table: uint64_t        ts_options=0x%016llx: '%s'\n",(unsigned long long int)ts_tablep->ts_options,option_string); // Time Stamping Options 
	fprintf(stderr,"xdd_show_ts_table: int64_t         ts_current_entry=%lld\n",(long long int)ts_tablep->ts_current_entry); 		// Index into the Timestamp Table of the current entry
	fprintf(stderr,"xdd_show_ts_table: int64_t         ts_size=%lld\n",(long long int)ts_tablep->ts_size);  						// Time Stamping Size in number of entries 
	fprintf(stderr,"xdd_show_ts_table: int64_t         ts_trigop=%lld\n",(long long int)ts_tablep->ts_trigop);  					// Time Stamping trigger operation number 
	fprintf(stderr,"xdd_show_ts_table: nclk_t          ts_trigtime=%lld\n",(long long int)ts_tablep->ts_trigtime); 					// Time Stamping trigger time 
	fprintf(stderr,"xdd_show_ts_table: char            *ts_binary_filename=%p: '%s'\n",ts_tablep->ts_binary_filename,
					                                                            (ts_tablep->ts_binary_filename != NULL)?ts_tablep->ts_binary_filename:"NA"); // Timestamp binary output filename for this Target
	fprintf(stderr,"xdd_show_ts_table: char            *ts_output_filename=%p: '%s'\n",ts_tablep->ts_output_filename,
					                                                            (ts_tablep->ts_output_filename != NULL)?ts_tablep->ts_output_filename:"NA"); // Timestamp report output filename for this Target
	fprintf(stderr,"xdd_show_ts_table: FILE            *ts_tsfp=%p\n",ts_tablep->ts_tsfp);   	// Pointer to the time stamp output file 
	fprintf(stderr,"xdd_show_ts_table: xdd_ts_header_t *ts_hdrp=%p\n",ts_tablep->ts_hdrp); // Pointer to the time stamp output file 
	if (ts_tablep->ts_hdrp)
		xdd_show_ts_header(ts_tablep->ts_hdrp, target_number);

    fprintf(stderr,"xdd_show_ts_table:********* End of TS TABLE for Target %d **********\n",target_number);

} // End of xdd_show_ts_table()

/*----------------------------------------------------------------------------*/
/* xdd_show_ts_header() - Display values in the specified data structure
 */
void
xdd_show_ts_header(xdd_ts_header_t *ts_hdrp, int target_number) {

    fprintf(stderr,"\txdd_show_ts_header: uint32_t   tsh_magic=0x%08ux\n",ts_hdrp->tsh_magic);          /**< Magic number indicating the beginning of timestamp data */
    fprintf(stderr,"\txdd_show_ts_header: char       tsh_version[XDD_VERSION_BUFSZ]=%s\n",ts_hdrp->tsh_version); /**< Version string for the timestamp data format */
    fprintf(stderr,"\txdd_show_ts_header: int32_t    tsh_target_thread_id=%d\n",ts_hdrp->tsh_target_thread_id); // My system target thread ID (like a process ID)
    fprintf(stderr,"\txdd_show_ts_header: int32_t    tsh_reqsize=%d\n",ts_hdrp->tsh_reqsize); 	/**< size of these requests in 'blocksize'-byte blocks */
    fprintf(stderr,"\txdd_show_ts_header: int32_t    tsh_blocksize=%d\n",ts_hdrp->tsh_blocksize); 	/**< size of each block in bytes */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_numents=%lld\n",(long long int)ts_hdrp->tsh_numents); 	/**< number of timestamp table entries */
    fprintf(stderr,"\txdd_show_ts_header: nclk_t     tsh_trigtime=%lld\n",(long long int)ts_hdrp->tsh_trigtime); 	/**< Time the time stamp started */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_trigop=%lld\n",(long long int)ts_hdrp->tsh_trigop);  	/**< Operation number that timestamping started */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_res=%lld\n",(long long int)ts_hdrp->tsh_res);  		/**< clock resolution - nano seconds per clock tick */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_range=%lld\n",(long long int)ts_hdrp->tsh_range);  	/**< range over which the IO took place */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_start_offset=%lld\n",(long long int)ts_hdrp->tsh_start_offset);	/**< offset of the starting block */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_target_offset=%lld\n",(long long int)ts_hdrp->tsh_target_offset);	/**< offset of the starting block for each proc*/
    fprintf(stderr,"\txdd_show_ts_header: uint64_t   tsh_global_options=0x%016llx\n",(unsigned long long int)ts_hdrp->tsh_global_options);	/**< options used */
    fprintf(stderr,"\txdd_show_ts_header: uint64_t   tsh_target_options=0x%016llx\n",(unsigned long long int)ts_hdrp->tsh_target_options);	/**< options used */
    fprintf(stderr,"\txdd_show_ts_header: char       tsh_id[MAX_IDLEN]='%s'\n",ts_hdrp->tsh_id); 	/**< ID string */
    fprintf(stderr,"\txdd_show_ts_header: char       tsh_td[CTIME_BUFSZ]='%s'\n",ts_hdrp->tsh_td);  	/**< time and date */
    fprintf(stderr,"\txdd_show_ts_header: nclk_t     tsh_timer_oh=%lld\n",(long long int)ts_hdrp->tsh_timer_oh); 	/**< Timer overhead in nanoseconds */
    fprintf(stderr,"\txdd_show_ts_header: nclk_t     tsh_delta=%lld\n",(long long int)ts_hdrp->tsh_delta);  	/**< Delta used for normalization */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_tt_bytes=%lld\n",(long long int)ts_hdrp->tsh_tt_bytes); 	/**< Size of the entire time stamp table in bytes */
    fprintf(stderr,"\txdd_show_ts_header: size_t     tsh_tt_size=%d\n",(int)ts_hdrp->tsh_tt_size); 	/**< Size of the entire time stamp table in entries */
    fprintf(stderr,"\txdd_show_ts_header: int64_t    tsh_tte_indx=%lld\n",(long long int)ts_hdrp->tsh_tte_indx); 	/**< Index into the time stamp table */
    fprintf(stderr,"\txdd_show_ts_header: struct tte tsh_tte[]\n"); 	/**< timestamp table entries */

} // End of xdd_show_ts_header()

/*----------------------------------------------------------------------------*/
// xdd_results_dump 
void
xdd_show_results_data(results_t *rp, char *dumptype, xdd_plan_t *planp) {

    fprintf(stderr,"\nxdd_show_results_data:********* Start of Results Data at 0x%p **********\n",rp);
	fprintf(stderr,"	flags = 0x%016x\n",(unsigned int)rp->flags);				// Flags that tell the display function what to display
	fprintf(stderr,"	*what = '%s'\n",rp->what);					// The type of information line to display - Queue Pass, Target Pass, Queue Avg, Target Avg, Combined
	fprintf(stderr,"	*output = 0x%16p\n",rp->output);			// This points to the output file 
	fprintf(stderr,"	delimiter = 0x%1x\n",rp->delimiter);		// The delimiter to use between fields - i.e. a space or tab or comma

	// Fundamental Variables
	fprintf(stderr,"	*format_string = '%s'\n",rp->format_string);		// Format String for the xdd_results_display_processor() 
	fprintf(stderr,"	my_target_number = %d\n",rp->my_target_number); 	// Target number of instance 
	fprintf(stderr,"	my_worker_thread_number = %d\n",rp->my_worker_thread_number);	// Qthread number of this instance 
	fprintf(stderr,"	queue_depth = %d\n",rp->queue_depth);		 	// The queue depth of this target
	fprintf(stderr,"	xfer_size_bytes = %12.0f\n",rp->xfer_size_bytes);		// Transfer size in bytes 
	fprintf(stderr,"	xfer_size_blocks = %12.3f\n",rp->xfer_size_blocks);		// Transfer size in blocks 
	fprintf(stderr,"	xfer_size_kbytes = %12.3f\n",rp->xfer_size_kbytes);		// Transfer size in Kbytes 
	fprintf(stderr,"	xfer_size_mbytes = %12.3f\n",rp->xfer_size_mbytes);		// Transfer size in Mbytes 
	fprintf(stderr,"	reqsize = %d\n",rp->reqsize); 			// RequestSize from the target_data 
	fprintf(stderr,"	pass_number = %d\n",rp->pass_number); 	// Pass number of this set of results 
	fprintf(stderr,"	*optype = '%s'\n",rp->optype);			// Operation type - read, write, or mixed

	// Incremented Counters
	fprintf(stderr,"	bytes_xfered = %lld\n",(long long)rp->bytes_xfered);		// Bytes transfered 
	fprintf(stderr,"	bytes_read = %lld\n",(long long)rp->bytes_read);			// Bytes transfered during read operations
	fprintf(stderr,"	bytes_written = %lld\n",(long long)rp->bytes_written);		// Bytes transfered during write operations
	fprintf(stderr,"	op_count = %lld\n",(long long)rp->op_count);    			// Operations performed 
	fprintf(stderr,"	read_op_count = %lld\n",(long long)rp->read_op_count);		// Read operations performed 
	fprintf(stderr,"	write_op_count = %lld\n",(long long)rp->write_op_count); 	// Write operations performed 
	fprintf(stderr,"	error_count = %lld\n",(long long)rp->error_count);  		// Number of I/O errors 

	// Timing Information - calculated from time stamps/values of when things hapened 
	fprintf(stderr,"	accumulated_op_time = %8.3f\n",rp->accumulated_op_time);		// Total Accumulated Time in seconds processing I/O ops 
	fprintf(stderr,"	accumulated_read_op_time = %8.3f\n",rp->accumulated_read_op_time);	// Total Accumulated Time in seconds processing read I/O ops 
	fprintf(stderr,"	accumulated_write_op_time = %8.3f\n",rp->accumulated_write_op_time);	// Total Accumulated Time in seconds processing write I/O ops 
	fprintf(stderr,"	accumulated_pattern_fill_time = %8.3f\n",rp->accumulated_pattern_fill_time);	// Total Accumulated Time in seconds doing pattern fills 
	fprintf(stderr,"	accumulated_flush_time = %8.3f\n",rp->accumulated_flush_time);	// Total Accumulated Time in seconds doing buffer flushes 
	fprintf(stderr,"	earliest_start_time_this_run = %8.3f\n",rp->earliest_start_time_this_run);	// usec, For a Worker Thread this is simply the start time of pass 1, for a target it is the earliest start time of all threads
	fprintf(stderr,"	earliest_start_time_this_pass = %8.3f\n",rp->earliest_start_time_this_pass);	// usec, For a Worker Thread this is simply the start time of pass 1, for a target it is the earliest start time of all threads
	fprintf(stderr,"	latest_end_time_this_run = %8.3f\n",rp->latest_end_time_this_run);			// usec, For a Worker Thread this is the end time of the last pass, for a target it is the latest end time of all Worker Threads
	fprintf(stderr,"	latest_end_time_this_pass = %8.3f\n",rp->latest_end_time_this_pass);			// usec, For a Worker Thread this is the end time of the last pass, for a target it is the latest end time of all Worker Threads
	fprintf(stderr,"	elapsed_pass_time = %8.3f\n",rp->elapsed_pass_time);		// usec, Total time from start of pass to the end of the last operation 
	fprintf(stderr,"	elapsed_pass_time_from_first_op = %8.3f\n",rp->elapsed_pass_time_from_first_op); // usec, Total time from start of first op in pass to the end of the last operation 
	fprintf(stderr,"	pass_start_lag_time = %8.3f\n",rp->pass_start_lag_time); 	// usec, Lag time from start of pass to the start of the first operation 
	fprintf(stderr,"	bandwidth = %8.3f\n",rp->bandwidth);				// Measured data rate in MB/sec from start of first op to end of last op
	fprintf(stderr,"	read_bandwidth = %8.3f\n",rp->read_bandwidth);			// Measured read data rate in MB/sec  from start of first op to end of last op
	fprintf(stderr,"	write_bandwidth = %8.3f\n",rp->write_bandwidth);		// Measured write data rate in MB/sec from start of first op to end of last op 
	fprintf(stderr,"	iops = %8.3f\n",rp->iops);					// Measured I/O Operations per second from start of first op to end of last op 
	fprintf(stderr,"	read_iops = %8.3f\n",rp->read_iops);				// Measured read I/O Operations per second from start of first op to end of last op 
	fprintf(stderr,"	write_iops = %8.3f\n",rp->write_iops);				// Measured write I/O Operations per second from start of first op to end of last op 
	fprintf(stderr,"	latency = %8.3f\n",rp->latency); 				// Latency in milliseconds 

	// CPU Utilization Information >> see user_time, system_time, and us_time below
	fprintf(stderr,"	user_time = %8.3f\n",rp->user_time); 				// Amount of CPU time used by the application for this pass 
	fprintf(stderr,"	system_time = %8.3f\n",rp->system_time); 			// Amount of CPU time used by the system for this pass 
	fprintf(stderr,"	us_time = %8.3f\n",rp->us_time); 				// Total CPU time used by this process: user+system time for this pass 
	fprintf(stderr,"	percent_user = %8.3f\n",rp->percent_user); 			// Percent of User CPU used by this process 
	fprintf(stderr,"	percent_system = %8.3f\n",rp->percent_system); 		// Percent of SYSTEM CPU used by this process 
	fprintf(stderr,"	percent_cpu = %8.3f\n",rp->percent_cpu); 		// Percent of CPU used by this process 

	// Other information - only valid when certain options are used
	fprintf(stderr,"	compare_errors = %lld\n",(long long)rp->compare_errors);			// Number of compare errors on a sequenced data pattern check 
	fprintf(stderr,"	e2e_io_time_this_pass = %8.3f\n",rp->e2e_io_time_this_pass); 			// Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_sr_time_this_pass = %8.3f\n",rp->e2e_sr_time_this_pass); 			// Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_sr_time_percent_this_pass = %8.3f\n",rp->e2e_sr_time_percent_this_pass); 	// Percentage of total Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_io_time_this_pass = %8.3f\n",rp->e2e_io_time_this_run); 			// Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_sr_time_this_pass = %8.3f\n",rp->e2e_sr_time_this_run); 			// Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_sr_time_percent_this_pass = %8.3f\n",rp->e2e_sr_time_percent_this_run); 	// Percentage of total Time spent sending or receiving messages for E2E option 
	fprintf(stderr,"	e2e_wait_1st_msg = %8.3f\n",rp->e2e_wait_1st_msg);    	// Time spent waiting on the destination side of an E2E operation for the first msg 
	fprintf(stderr,"	open_start_time = %8.3f\n",rp->open_start_time); 		// Time stamp at the beginning of openning the target
	fprintf(stderr,"	open_end_time = %8.3f\n",rp->open_end_time); 			// Time stamp after openning the target 
	fprintf(stderr,"	open_time = %8.3f\n",rp->open_time); 				// Time spent openning the target: end_time-start_time

	// Individual Op bandwidths - Only used when -extendedstats option is specified
	fprintf(stderr,"	longest_op_time = %12.3f\n",rp->longest_op_time); 		// Longest op time that occured during this pass
	fprintf(stderr,"	longest_read_op_time = %12.3f\n",rp->longest_read_op_time); 	// Longest read op time that occured during this pass
	fprintf(stderr,"	longest_write_op_time = %12.3f\n",rp->longest_write_op_time); 	// Longest write op time that occured during this pass
	fprintf(stderr,"	shortest_op_time = %12.3f\n",rp->shortest_op_time); 		// Shortest op time that occurred during this pass
	fprintf(stderr,"	shortest_read_op_time = %12.3f\n",rp->shortest_read_op_time); 	// Shortest read op time that occured during this pass
	fprintf(stderr,"	shortest_write_op_time = %12.3f\n",rp->shortest_write_op_time); // Shortest write op time that occured during this pass

	fprintf(stderr,"	longest_op_bytes = %lld\n",(long long)rp->longest_op_bytes); 			// Bytes xfered when the longest op time occured during this pass
	fprintf(stderr," 	longest_read_op_bytes = %lld\n",(long long)rp->longest_read_op_bytes);	 	// Bytes xfered when the longest read op time occured during this pass
	fprintf(stderr," 	longest_write_op_bytes = %lld\n",(long long)rp->longest_write_op_bytes); 	// Bytes xfered when the longest write op time occured during this pass
	fprintf(stderr," 	shortest_op_bytes = %lld\n",(long long)rp->shortest_op_bytes); 			// Bytes xfered when the shortest op time occured during this pass
	fprintf(stderr," 	shortest_read_op_bytes = %lld\n",(long long)rp->shortest_read_op_bytes); 	// Bytes xfered when the shortest read op time occured during this pass
	fprintf(stderr," 	shortest_write_op_bytes = %lld\n",(long long)rp->shortest_write_op_bytes);	// Bytes xfered when the shortest write op time occured during this pass

	fprintf(stderr,"	longest_op_number = %lld\n",(long long)rp->longest_op_number); 			// Operation Number when the longest op time occured during this pass
	fprintf(stderr," 	longest_read_op_number = %lld\n",(long long)rp->longest_read_op_number); 	// Operation Number when the longest read op time occured during this pass
	fprintf(stderr," 	longest_write_op_number = %lld\n",(long long)rp->longest_write_op_number); 	// Operation Number when the longest write op time occured during this pass
	fprintf(stderr," 	shortest_op_number = %lld\n",(long long)rp->shortest_op_number); 		// Operation Number when the shortest op time occured during this pass
	fprintf(stderr," 	shortest_read_op_number = %lld\n",(long long)rp->shortest_read_op_number); 	// Operation Number when the shortest read op time occured during this pass
	fprintf(stderr," 	shortest_write_op_number = %lld\n",(long long)rp->shortest_write_op_number);	// Operation Number when the shortest write op time occured during this pass

	fprintf(stderr,"	longest_op_pass_number = %lld\n",(long long)rp->longest_op_pass_number);		// Pass Number when the longest op time occured during this pass
	fprintf(stderr,"	longest_read_op_pass_number = %lld\n",(long long)rp->longest_read_op_pass_number);// Pass Number when the longest read op time occured
	fprintf(stderr,"	longest_write_op_pass_number = %lld\n",(long long)rp->longest_write_op_pass_number);// Pass Number when the longest write op time occured 
	fprintf(stderr,"	shortest_op_pass_number = %lld\n",(long long)rp->shortest_op_pass_number);	// Pass Number when the shortest op time occured 
	fprintf(stderr,"	shortest_read_op_pass_number = %lld\n",(long long)rp->shortest_read_op_pass_number);// Pass Number when the shortest read op time occured 
	fprintf(stderr,"	shortest_write_op_pass_number = %lld\n",(long long)rp->shortest_write_op_pass_number);// Pass Number when the shortest write op time occured 

	fprintf(stderr,"	highest_bandwidth = %12.3f\n",rp->highest_bandwidth);		// Highest individual op data rate in MB/sec
	fprintf(stderr,"	highest_read_bandwidth = %12.3f\n",rp->highest_read_bandwidth);	// Highest individual op read data rate in MB/sec 
	fprintf(stderr,"	highest_write_bandwidth = %12.3f\n",rp->highest_write_bandwidth);// Highest individual op write data rate in MB/sec 
	fprintf(stderr,"	lowest_bandwidth = %12.3f\n",rp->lowest_bandwidth);		// Lowest individual op data rate in MB/sec
	fprintf(stderr,"	lowest_read_bandwidth = %12.3f\n",rp->lowest_read_bandwidth);	// Lowest individual op read data rate in MB/sec 
	fprintf(stderr,"	lowest_write_bandwidth = %12.3f\n",rp->lowest_write_bandwidth);	// Lowest individual op write data rate in MB/sec 

	fprintf(stderr,"	highest_iops = %12.3f\n",rp->highest_iops);			// Highest individual op I/O Operations per second 
	fprintf(stderr,"	highest_read_iops = %12.3f\n",rp->highest_read_iops);		// Highest individual op read I/O Operations per second 
	fprintf(stderr,"	highest_write_iops = %12.3f\n",rp->highest_write_iops);		// Highest individual op write I/O Operations per second 
	fprintf(stderr,"	lowest_iops = %12.3f\n",rp->lowest_iops);			// Lowest individual op I/O Operations per second 
	fprintf(stderr,"	lowest_read_iops = %12.3f\n",rp->lowest_read_iops);		// Lowest individual op read I/O Operations per second 
	fprintf(stderr,"	lowest_write_iops = %12.3f\n",rp->lowest_write_iops);		// Lowest individual op write I/O Operations per second 

    fprintf(stderr,"\nxdd_show_results_data:********* End of Results Data at 0x%p **********\n",rp);
	return;
}
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
