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
 * This file contains the subroutines that perform various debugging functions 
 * that mainly display key data structures and things like that.
 */
#include "xint.h"

 
/*----------------------------------------------------------------------------*/
/* xdd_show_global_data() - Display values in the specified global data structure
 */
void
xdd_show_global_data(void) {
    fprintf(stderr,"********* Start of Global Data **********\n");
    fprintf(xgp->output,"global_options          0x%016llx - I/O Options valid for all targets \n",(long long int)xgp->global_options);
    fprintf(xgp->output,"progname                  %s - Program name from argv[0] \n",xgp->progname);
    fprintf(xgp->output,"argc                      %d - The original arg count \n",xgp->argc);
    fprintf(xgp->output,"max_errors                %lld - max number of errors to tollerate \n",(long long int)xgp->max_errors);
    fprintf(xgp->output,"max_errors_to_print       %lld - Maximum number of compare errors to print \n",(long long int)xgp->max_errors_to_print);
    fprintf(xgp->output,"output_filename          '%s' - name of the output file \n",(xgp->output_filename != NULL)?xgp->output_filename:"NA");
    fprintf(xgp->output,"errout_filename          '%s' - name fo the error output file \n",(xgp->errout_filename != NULL)?xgp->errout_filename:"NA");
    fprintf(xgp->output,"csvoutput_filename       '%s' - name of the csv output file \n",(xgp->csvoutput_filename != NULL)?xgp->csvoutput_filename:"NA");
    fprintf(xgp->output,"combined_output_filename '%s' - name of the combined output file \n",(xgp->combined_output_filename != NULL)?xgp->combined_output_filename:"NA");
    fprintf(xgp->output,"output                  0x%p - Output file pointer \n",xgp->output);
    fprintf(xgp->output,"errout                  0x%p - Error Output file pointer \n",xgp->errout);
    fprintf(xgp->output,"csvoutput               0x%p - Comma Separated Values output file \n",xgp->csvoutput);
    fprintf(xgp->output,"combined_output         0x%p - Combined output file \n",xgp->combined_output);
    fprintf(xgp->output,"id                       '%s' - ID string pointer \n",(xgp->id != NULL)?xgp->id:"NA");
    fprintf(xgp->output,"number_of_processors      %d - Number of processors \n",xgp->number_of_processors);
    fprintf(xgp->output,"clock_tick                %d - Number of clock ticks per second \n",xgp->clock_tick);
    fprintf(xgp->output,"id_firsttime              %d - ID first time through flag \n",xgp->id_firsttime);
    fprintf(xgp->output,"run_error_count_exceeded  %d - The alarm that goes off when the number of errors for this run has been exceeded \n",xgp->run_error_count_exceeded);
    fprintf(xgp->output,"run_time_expired          %d - The alarm that goes off when the total run time has been exceeded \n",xgp->run_time_expired);
    fprintf(xgp->output,"run_complete              %d - Set to a 1 to indicate that all passes have completed \n",xgp->run_complete);
    fprintf(xgp->output,"abort                     %d - abort the run due to some catastrophic failure \n",xgp->abort);
    fprintf(xgp->output,"random_initialized        %d - Random number generator has been initialized \n",xgp->random_initialized);
    fprintf(stderr,"********* End of Global Data **********\n");

} /* end of xdd_show_global_data() */ 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_plan_data() - Display values in the specified global data structure
 */
void
xdd_show_plan_data(xdd_plan_t* planp) {
    int        target_number;    


    fprintf(stderr,"********* Start of Plan Data **********\n");
    fprintf(xgp->output,"passes                    %d - number of passes to perform \n",planp->passes);
    fprintf(xgp->output,"pass_delay                %f - number of seconds to delay between passes \n",planp->pass_delay);
    fprintf(xgp->output,"max_errors                %lld - max number of errors to tollerate \n",(long long int)xgp->max_errors);
    fprintf(xgp->output,"max_errors_to_print       %lld - Maximum number of compare errors to print \n",(long long int)xgp->max_errors_to_print);
    fprintf(xgp->output,"ts_binary_filename_prefix '%s' - timestamp binary filename prefix \n",(planp->ts_binary_filename_prefix != NULL)?planp->ts_binary_filename_prefix:"NA");
    fprintf(xgp->output,"ts_output_filename_prefix '%s' - timestamp report output filename prefix \n",(planp->ts_output_filename_prefix != NULL)?planp->ts_output_filename_prefix:"NA");
    fprintf(xgp->output,"restart_frequency         %d - seconds between restart monitor checks \n",planp->restart_frequency);
    fprintf(xgp->output,"syncio                    %d - the number of I/Os to perform btw syncs \n",planp->syncio);
    fprintf(xgp->output,"target_offset             %lld - offset value \n",(long long int)planp->target_offset);
    fprintf(xgp->output,"number_of_targets         %d - number of targets to operate on \n",planp->number_of_targets);
    fprintf(xgp->output,"number_of_iothreads       %d - number of threads spawned for all targets \n",planp->number_of_iothreads);
    fprintf(xgp->output,"run_time                  %f - Length of time to run all targets, all passes \n",planp->run_time);
    fprintf(xgp->output,"base_time                 %lld - The time that xdd was started - set during initialization \n",(long long int)planp->base_time);
    fprintf(xgp->output,"run_start_time            %lld - The time that the targets start their first pass - set after initialization \n",(long long int)planp->run_start_time);
    fprintf(xgp->output,"estimated_end_time        %lld - The time at which this run (all passes) should end \n",(long long int)planp->estimated_end_time);
    fprintf(xgp->output,"number_of_processors      %d - Number of processors \n",planp->number_of_processors);
    fprintf(xgp->output,"clock_tick                %d - Number of clock ticks per second \n",planp->clock_tick);
    fprintf(xgp->output,"barrier_count,            %d Number of barriers on the chain \n",planp->barrier_count);                         
    fprintf(xgp->output,"format_string,            '%s'\n",(planp->format_string != NULL)?planp->format_string:"NA");
    fprintf(xgp->output,"results_header_displayed   %d\n",planp->results_header_displayed);
    fprintf(xgp->output,"heartbeat_holdoff          %d\n",planp->heartbeat_holdoff);

    fprintf(xgp->output,"Target_Data pointersC\n");
    for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
        fprintf(xgp->output,"\tTarget_Data pointer for target %d of %d is 0x%p\n",target_number, planp->number_of_targets, planp->target_datap[target_number]);
    }
    fprintf(stderr,"********* End of Plan Data **********\n");

} /* end of xdd_show_plan_data() */ 

/*----------------------------------------------------------------------------*/
/* xdd_show_target_data() - Display values in the specified Per-Target-Data-Structure 
 */
void
xdd_show_target_data(target_data_t *tdp) {
    char *sp;

    fprintf(stderr,"********* Start of TARGET_DATA 0x%p **********\n", tdp);

    fprintf(stderr,"struct xdd_plan         *td_planp=%p\n",tdp->td_planp);
    fprintf(stderr,"struct xint_worker_data *td_next_wdp=%p\n",tdp->td_next_wdp);    // Pointer to the first worker_data struct in the list
    fprintf(stderr,"pthread_t               td_thread\n");            // Handle for this Target Thread 
    fprintf(stderr,"int32_t                 td_thread_id=%d\n",tdp->td_thread_id);          // My system thread ID (like a process ID) 
    fprintf(stderr,"int32_t                 td_pid=%d\n",tdp->td_pid);               // My process ID 
    fprintf(stderr,"int32_t                 td_target_number=%d\n",tdp->td_target_number);    // My target number 
    fprintf(stderr,"uint64_t                td_target_options=%llx\n",(unsigned long long int)tdp->td_target_options);             // I/O Options specific to each target 
    fprintf(stderr,"int32_t                 td_file_desc=%d\n",tdp->td_file_desc);        // File Descriptor for the target device/file 
    fprintf(stderr,"int32_t                 td_open_flags=%x\n",tdp->td_open_flags);        // Flags used during open processing of a target
    fprintf(stderr,"int32_t                 td_xfer_size=%d\n",tdp->td_xfer_size);          // Number of bytes per request 
    fprintf(stderr,"int32_t                 td_filetype=%d\n",tdp->td_filetype);          // Type of file: regular, device, socket, ... 
    fprintf(stderr,"int64_t                 td_filesize=%lld\n",(long long int)tdp->td_filesize);          // Size of target file in bytes 
    fprintf(stderr,"int64_t                 td_target_ops=%lld\n",(long long int)tdp->td_target_ops);      // Total number of ops to perform on behalf of a "target"
    fprintf(stderr,"seekhdr_t               td_seekhdr\n");          // For all the seek information 
    fprintf(stderr,"FILE                    *td_tsfp=%p\n",tdp->td_planp);           // Pointer to the time stamp output file 
    fprintf(stderr,"xdd_occupant_t          td_occupant\n");                            // Used by the barriers to keep track of what is in a barrier at any given time
    fprintf(stderr,"char                    td_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH]='%s'\n",tdp->td_occupant_name);    // For a Target thread this is "TARGET####", for a Worker Thread it is "TARGET####WORKER####"
    fprintf(stderr,"xdd_barrier_t           *td_current_barrier=%p\n",tdp->td_current_barrier);                    // Pointer to the current barrier this Thread is in at any given time or NULL if not in a barrier
    fprintf(stderr,"xdd_barrier_t           td_target_worker_thread_init_barrier\n");        // Where the Target Thread waits for the Worker Thread to initialize
    fprintf(stderr,"xdd_barrier_t           td_targetpass_worker_thread_passcomplete_barrier\n");// The barrier used to sync targetpass() with all the Worker Threads at the end of a pass
    fprintf(stderr,"xdd_barrier_t           td_targetpass_worker_thread_eofcomplete_barrier\n");// The barrier used to sync targetpass_eof_desintation_side() with a Worker Thread trying to recv an EOF packet
    fprintf(stderr,"uint64_t                td_current_bytes_issued=%lld\n",(long long int)tdp->td_current_bytes_issued);    // The amount of data for all transfer requests that has been issued so far 
    fprintf(stderr,"uint64_t                td_current_bytes_completed=%lld\n",(long long int)tdp->td_current_bytes_completed);    // The amount of data for all transfer requests that has been completed so far
    fprintf(stderr,"uint64_t                td_current_bytes_remaining=%lld\n",(long long int)tdp->td_current_bytes_remaining);    // Bytes remaining to be transferred 
    fprintf(stderr,"char                    td_abort=%d\n",tdp->td_abort);                    // Abort this operation (either a Worker Thread or a Target Thread)
    fprintf(stderr,"char                    td_time_limit_expired=%d\n",tdp->td_time_limit_expired);        // The time limit for this target has expired
    fprintf(stderr,"pthread_mutex_t         td_current_state_mutex\n");     // Mutex for locking when checking or updating the state info
    switch (tdp->td_current_state) {
        case CURRENT_STATE_INIT:
                sp="CURRENT_STATE_INIT";
        case CURRENT_STATE_IO:
                sp="CURRENT_STATE_IO";
        case CURRENT_STATE_DEST_RECEIVE:
                sp="CURRENT_STATE_DEST_RECEIVE";
        case CURRENT_STATE_SRC_SEND:
                sp="CURRENT_STATE_SRC_SEND";
        case CURRENT_STATE_BARRIER:
                sp="CURRENT_STATE_BARRIER";
        case CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE:
                sp="CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE";
        case CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE:
                sp="CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE";
        case CURRENT_STATE_PASS_COMPLETE:
                sp="CURRENT_STATE_PASS_COMPLETE";
        case CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE:
                sp="CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE";
        case CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE:
                sp="CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE";
        case CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS:
                sp="CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS";
        case CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO:
                sp="CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO";
        default:
                sp="UNDEFINED STATE";
                break;
    }
    fprintf(stderr,"int32_t                 td_current_state=0x%08x, '%s'\n",tdp->td_current_state,sp);            // State of this thread at any given time (see Current State definitions below)
    fprintf(stderr,"tot_t                   *td_totp=%p\n",tdp->td_totp);                                // Pointer to the target_offset_table for this target
    fprintf(stderr,"pthread_cond_t          td_any_worker_thread_available_condition\n");
    fprintf(stderr,"pthread_mutex_t         td_any_worker_thread_available_mutex\n");
    fprintf(stderr,"int                     td_any_worker_thread_available=%d\n",tdp->td_any_worker_thread_available);
    fprintf(stderr,"pthread_cond_t          td_this_wthread_is_available_condition\n");
    fprintf(stderr,"int64_t                 td_start_offset=%lld\n",(long long int)tdp->td_start_offset);             // starting block offset value 
    fprintf(stderr,"int64_t                 td_pass_offset=%lld\n",(long long int)tdp->td_pass_offset);             // number of blocks to add to seek locations between passes 
    fprintf(stderr,"int64_t                 td_flushwrite=%lld\n",(long long int)tdp->td_flushwrite);              // number of write operations to perform between flushes 
    fprintf(stderr,"int64_t                 td_flushwrite_current_count=%lld\n",(long long int)tdp->td_flushwrite_current_count);  // Running number of write operations - used to trigger a flush (sync) operation 
    fprintf(stderr,"int64_t                 td_bytes=%lld\n",(long long int)tdp->td_bytes);                   // number of bytes to process overall 
    fprintf(stderr,"int64_t                 td_numreqs=%lld\n",(long long int)tdp->td_numreqs);                  // Number of requests to perform per pass per qthread
    fprintf(stderr,"double                  td_rwratio=%f\n",tdp->td_rwratio);                  // read/write ratios 
    fprintf(stderr,"nclk_t                  td_report_threshold=%lld\n",(unsigned long long int)tdp->td_report_threshold);        // reporting threshold for long operations 
    fprintf(stderr,"int32_t                 td_reqsize=%d\n",tdp->td_reqsize);                  // number of *blocksize* byte blocks per operation for each target 
    fprintf(stderr,"int32_t                 td_retry_count=%d\n",tdp->td_retry_count);              // number of retries to issue on an error 
    fprintf(stderr,"double                  td_time_limit=%f\n",tdp->td_time_limit);                // Time of a single pass in seconds
    fprintf(stderr,"nclk_t                  td_time_limit_ticks=%lld\n",(unsigned long long int)tdp->td_time_limit_ticks);        // Time of a single pass in high-res clock ticks
    fprintf(stderr,"char                    *td_target_directory=%s\n",tdp->td_target_directory);         // The target directory for the target 
    fprintf(stderr,"char                    *td_target_basename=%s\n",tdp->td_target_basename);         // Basename of the target/device
    fprintf(stderr,"char                    *td_target_full_pathname=%s\n",tdp->td_target_full_pathname);    // Fully qualified path name to the target device/file
    fprintf(stderr,"char                    td_target_extension[32]=%s\n",tdp->td_target_extension);     // The target extension number 
    fprintf(stderr,"int32_t                 td_processor=%d\n",tdp->td_processor);                  // Processor/target assignments 
    fprintf(stderr,"double                  td_start_delay=%f\n",tdp->td_start_delay);             // number of seconds to delay the start  of this operation 
    fprintf(stderr,"nclk_t                  td_start_delay_psec=%lld\n",(unsigned long long int)tdp->td_start_delay_psec);        // number of nanoseconds to delay the start  of this operation 
    fprintf(stderr,"char                    td_random_init_state[256]\n");     // Random number generator state initalizer array 
    fprintf(stderr,"int32_t                 td_block_size=%d\n",tdp->td_block_size);              // Size of a block in bytes for this target 
    fprintf(stderr,"int32_t                 td_queue_depth=%d\n",tdp->td_queue_depth);             // Command queue depth for each target 
    fprintf(stderr,"int64_t                 td_preallocate=%lld\n",(long long int)tdp->td_preallocate);             // File preallocation value 
    fprintf(stderr,"int32_t                 td_mem_align=%d\n",tdp->td_mem_align);               // Memory read/write buffer alignment value in bytes 
    fprintf(stderr,"struct heartbeat        td_hb=%p\n",tdp->td_planp);                    // Heartbeat data
    fprintf(stderr,"uint64_t                td_target_bytes_to_xfer_per_pass=%lld\n",(long long int)tdp->td_target_bytes_to_xfer_per_pass);     // Number of bytes to xfer per pass for the entire target (all qthreads)
    fprintf(stderr,"int64_t                 td_last_committed_op=%lld\n",(long long int)tdp->td_last_committed_op);        // Operation number of last r/w operation relative to zero
    fprintf(stderr,"uint64_t                td_last_committed_location=%lld\n",(long long int)tdp->td_last_committed_location);    // Byte offset into target of last r/w operation
    fprintf(stderr,"int32_t                 td_last_committed_length=%d\n",tdp->td_last_committed_length);    // Number of bytes transferred to/from last_committed_location
    fprintf(stderr,"int32_t                 td_syncio_barrier_index=%d\n",tdp->td_syncio_barrier_index);         // Used to determine which syncio barrier to use at any given time
    fprintf(stderr,"int32_t                 td_results_pass_barrier_index=%d\n",tdp->td_results_pass_barrier_index); // Where a Target thread waits for all other Target threads to complete a pass
    fprintf(stderr,"int32_t                 td_results_display_barrier_index=%d\n",tdp->td_results_display_barrier_index); // Where threads wait for the results_manager()to display results
    fprintf(stderr,"int32_t                 td_results_run_barrier_index=%d\n",tdp->td_results_run_barrier_index);     // Where threads wait for all other threads at the completion of the run
    fprintf(stderr,"nclk_t                  td_open_start_time=%lld\n",(unsigned long long int)tdp->td_open_start_time);         // Time just before the open is issued for this target 
    fprintf(stderr,"nclk_t                  td_open_end_time=%lld\n",(unsigned long long int)tdp->td_open_end_time);             // Time just after the open completes for this target 
    fprintf(stderr,"pthread_mutex_t         td_counters_mutex\n");             // Mutex for locking when updating td_counters
    fprintf(stderr,"struct xint_target_counters td_counters\n");        // Pointer to the target counters
    fprintf(stderr,"struct xint_throttle    *td_throtp=%p\n",tdp->td_throtp);            // Pointer to the throttle sturcture
    fprintf(stderr,"struct xint_timestamp   *td_tsp=%p\n",tdp->td_tsp);            // Pointer to the time stamp stuff
    fprintf(stderr,"struct xdd_tthdr        *td_ttp=%p\n",tdp->td_ttp);            // Pointer to the time stamp stuff
    fprintf(stderr,"struct xint_e2e         *td_e2ep=%p\n",tdp->td_e2ep);            // Pointer to the e2e struct when needed
    fprintf(stderr,"struct xint_extended_stats *td_esp=%p\n",tdp->td_esp);            // Extended Stats Structure Pointer
    fprintf(stderr,"struct xint_triggers     *td_trigp=%p\n",tdp->td_trigp);            // Triggers Structure Pointer
    fprintf(stderr,"struct xint_data_pattern *td_dpp=%p\n",tdp->td_dpp);            // Data Pattern Structure Pointer
    fprintf(stderr,"struct xint_raw          *td_rawp=%p\n",tdp->td_rawp);              // RAW Data Structure Pointer
    fprintf(stderr,"struct lockstep          *td_lsp=%p\n",tdp->td_lsp);            // Pointer to the lockstep structure used by the lockstep option
    fprintf(stderr,"struct xint_restart      *td_restartp=%p\n",tdp->td_restartp);        // Pointer to the restart structure used by the restart monitor
    fprintf(stderr,"struct stat              td_statbuf\n");            // Target File Stat buffer used by xdd_target_open()
    fprintf(stderr,"int32_t                  td_op_delay=%d\n",tdp->td_op_delay);         // Number of seconds to delay between operations 
    fprintf(stderr,"********* End of TARGET_DATA %p **********\n", tdp);
} /* end of xdd_show_target_data() */ 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_worker_data() - Display values in the specified data structure
 */
void
xdd_show_worker_data(worker_data_t *wdp) {
    char *sp;

    fprintf(stderr,"********* Start of Worker Data at 0x%p **********\n",wdp);

    fprintf(stderr,"target_data_t           *wd_tdp=%p\n",wdp->wd_tdp);             // Pointer back to the Parent Target Data
    fprintf(stderr,"struct xint_worker_data *wd_next_wdp=%p\n",wdp->wd_next_wdp);        // Pointer to the next worker_data struct in the list
    fprintf(stderr,"pthread_t               wd_thread=%p\n",wdp->wd_next_wdp);            // Handle for this Worker_Thread
    fprintf(stderr,"int32_t                 wd_worker_number=%d\n",wdp->wd_worker_number);    // My worker number within this target relative to 0
    fprintf(stderr,"int32_t                 wd_thread_id=%d\n",wdp->wd_thread_id);          // My system thread ID (like a process ID) 
    fprintf(stderr,"int32_t                 wd_pid=%d\n",wdp->wd_pid);               // My process ID 
    fprintf(stderr,"unsigned char           *wd_bufp=%p\n",wdp->wd_bufp);            // Pointer to the generic I/O buffer
    fprintf(stderr,"int                     wd_buf_size=%d\n",wdp->wd_buf_size);        // Size in bytes of the generic I/O buffer
    fprintf(stderr,"int64_t                 wd_ts_entry=%lld\n",(long long int)wdp->wd_next_wdp);        // The TimeStamp entry to use when time-stamping an operation

    fprintf(stderr,"struct xint_task        wd_task:\n");        
    xdd_show_task(&wdp->wd_task);

    fprintf(stderr,"struct xint_target_counters  wd_counters:\n");        
    xdd_show_target_counters(&wdp->wd_counters);

    fprintf(stderr,"pthread_mutex_t         wd_worker_thread_target_sync_mutex\n");    // Used to serialize access to the Worker_Thread-Target Synchronization flags

    switch (wdp->wd_worker_thread_target_sync) {
        case WTSYNC_AVAILABLE:
              sp="WTSYNC_AVAILABLE";
              break;
        case WTSYNC_BUSY:
              sp="WTSYNC_BUSY";
              break;
        case WTSYNC_TARGET_WAITING:
              sp="WTSYNC_BUSY";
              break;
        case WTSYNC_EOF_RECEIVED:
              sp="WTSYNC_EOF_RECEIVED";
              break;
        default:
              sp="UNDEFINED";
              break;
    }
    fprintf(stderr,"int32_t                 wd_worker_thread_target_sync=0x%08x, %s\n",wdp->wd_worker_thread_target_sync,sp);        // Flags used to synchronize a Worker_Thread with its Target
    fprintf(stderr,"pthread_cond_t          wd_this_worker_thread_is_available_condition\n");
    fprintf(stderr,"xdd_barrier_t           wd_thread_targetpass_wait_for_task_barrier:\n");    // The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
    fprintf(stderr,"xdd_occupant_t          wd_occupant:\n");        // Used by the barriers to keep track of what is in a barrier at any given time
    xdd_show_occupant(&wdp->wd_occupant);
    fprintf(stderr,"char                    wd_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH]='%s'\n",wdp->wd_occupant_name);    // For a Target thread this is "TARGET####", for a Worker_Thread it is "TARGET####WORKER####"
    fprintf(stderr,"xint_e2e_t              *wd_e2ep=%p\n",wdp->wd_e2ep);            // Pointer to the e2e struct when needed
    fprintf(stderr,"xdd_sgio_t              *wd_sgiop=%p\n",wdp->wd_sgiop);            // SGIO Structure Pointer

    fprintf(stderr,"********* End of Worker Data **********\n");

} // End of xdd_show_worker_data() 

/*----------------------------------------------------------------------------*/
/* xdd_show_task() - Display values in the specified data structure
 * The xint_task structure is used to pass task parameters to a Worker Thread
 */
void
xdd_show_task(xint_task_t *taskp) {
    char *sp;    // String pointer

    fprintf(stderr,"********* Start of TASK Data at 0x%p **********\n",taskp);
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
    fprintf(stderr,"\tchar          task_request=%d, %s\n",taskp->task_request,sp);            // Type of Task to perform
    fprintf(stderr,"\tint           task_file_desc=%d\n",taskp->task_request);                // File Descriptor
    fprintf(stderr,"\tunsigned char *task_datap=%p\n",taskp->task_datap);                    // The data section of the I/O buffer
    fprintf(stderr,"\tchar          task_op_type=%d\n",taskp->task_op_type);                // Operation to perform
    fprintf(stderr,"\tchar          *task_op_string='%s'\n",taskp->task_op_string);            // Operation to perform in ASCII
    fprintf(stderr,"\tuint64_t      task_op_number=%lld\n",(unsigned long long int)taskp->task_op_number);    // Offset into the file where this transfer starts
    fprintf(stderr,"\tsize_t        task_xfer_size=%d\n",(int)taskp->task_xfer_size);                // Number of bytes to transfer
    fprintf(stderr,"\toff_t         task_byte_offset=%lld\n",(long long int)taskp->task_byte_offset);            // Offset into the file where this transfer starts
    fprintf(stderr,"\tuint64_t      task_e2e_sequence_number=%lld\n",(long long int)taskp->task_e2e_sequence_number);    // Sequence number of this task when part of an End-to-End operation
    fprintf(stderr,"\tnclk_t        task_time_to_issue=%lld\n",(unsigned long long int)taskp->task_time_to_issue);            // Time to issue the I/O operation or 0 if not used
    fprintf(stderr,"\tssize_t       task_io_status=%d\n",(int)taskp->task_io_status);                // Returned status of this I/O associated with this task
    fprintf(stderr,"\tint32_t       task_errno=%d\n",taskp->task_errno);                    // Returned errno of this I/O associated with this task
    fprintf(stderr,"********* End of TASK Data at 0x%p **********\n",taskp);
} // End of xdd_show_task()

/*----------------------------------------------------------------------------*/
/* xdd_show_occupant() - Display values in the specified data structure
 */
void
xdd_show_occupant(xdd_occupant_t *op) {
    fprintf(stderr,"********* Start of Occupant Data at 0x%p **********\n",op);

    fprintf(stderr,"\tstruct xdd_occupant   *prev_occupant=%p\n",op->prev_occupant);    // Previous occupant on the chain
    fprintf(stderr,"\tstruct xdd_occupant   *next_occupant=%p\n",op->prev_occupant);    // Next occupant on the chain
    fprintf(stderr,"\tuint64_t              occupant_type=0x%016llx\n",(unsigned long long int)op->occupant_type);    // Bitfield that indicates the type of occupant
    fprintf(stderr,"\tchar                  *occupant_name='%s'\n",op->occupant_name);    // Pointer to a character string that is the name of this occupant
    fprintf(stderr,"\tvoid                  *occupant_data=%p\n",op->occupant_data);    // Pointer to a Target_Data or Worker_Data if the occupant_type is a Target or Worker Thread
    fprintf(stderr,"\tnclk_t                entry_time=%lld\n",(unsigned long long int)op->entry_time);        // Time stamp of when this occupant entered a barrier - filled in by xdd_barrier()
    fprintf(stderr,"\tnclk_t                exit_time=%lld\n",(unsigned long long int)op->exit_time);        // Time stamp of when this occupant was released from a barrier - filled in by xdd_barrier()

    fprintf(stderr,"********* End of Occupant Data at 0x%p **********\n",op);
} // End of xdd_show_occupant()

/*----------------------------------------------------------------------------*/
/* xdd_show_target_counters() - Display values in the specified data structure
 */
void
xdd_show_target_counters(xint_target_counters_t *tcp) {
    fprintf(stderr,"********* Start of Target Counters Data at 0x%p **********\n",tcp);

    // Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
    fprintf(stderr,"\tint32_t    tc_pass_number=%d\n",(int)tcp->tc_pass_number);                 // Current pass number 
    fprintf(stderr,"\tnclk_t     tc_pass_start_time=%lld\n",(unsigned long long int)tcp->tc_pass_start_time);             // The time this pass started but before the first operation is issued
    fprintf(stderr,"\tnclk_t     tc_pass_end_time=%lld\n",(unsigned long long int)tcp->tc_pass_end_time);                 // The time stamp that this pass ended 
    fprintf(stderr,"\tnclk_t     tc_pass_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_pass_elapsed_time);             // Time between the start and end of this pass
    fprintf(stderr,"\tnclk_t     tc_time_first_op_issued_this_pass=%lld\n",(unsigned long long int)tcp->tc_time_first_op_issued_this_pass); // Time this Worker Thread was able to issue its first operation for this pass
    fprintf(stderr,"\tstruct tms tc_starting_cpu_times_this_run:\n");    // CPU times from times() at the start of this run
    fprintf(stderr,"\tstruct tms tc_starting_cpu_times_this_pass:\n");// CPU times from times() at the start of this pass
    fprintf(stderr,"\tstruct tms tc_current_cpu_times:\n");            // CPU times from times()

    // Updated by the Worker Thread in Worker Data during an I/O operation
    // and then copied to the Target Data strcuture at the completion of an I/O operation
    fprintf(stderr,"\tuint64_t   tc_current_byte_offset=%lld\n",(unsigned long long int)tcp->tc_current_byte_offset);         // Current byte location for this I/O operation 
    fprintf(stderr,"\tuint64_t   tc_current_op_number=%lld\n",(unsigned long long int)tcp->tc_current_op_number);            // Current I/O operation number 
    fprintf(stderr,"\tint32_t    tc_current_io_status=%d\n",(int)tcp->tc_current_io_status);             // I/O Status of the last I/O operation for this Worker Thread
    fprintf(stderr,"\tint32_t    tc_current_io_errno=%d\n",(int)tcp->tc_current_io_errno);             // The errno associated with the status of this I/O for this thread
    fprintf(stderr,"\tint32_t    tc_current_bytes_xfered_this_op=%d\n",(int)tcp->tc_current_bytes_xfered_this_op);// I/O Status of the last I/O operation for this Worker Thread
    fprintf(stderr,"\tuint64_t   tc_current_error_count=%lld\n",(unsigned long long int)tcp->tc_current_error_count);            // The number of I/O errors for this Worker Thread
    fprintf(stderr,"\tnclk_t     tc_current_op_start_time=%lld\n",(unsigned long long int)tcp->tc_current_op_start_time);         // Start time of the current op
    fprintf(stderr,"\tnclk_t     tc_current_op_end_time=%lld\n",(unsigned long long int)tcp->tc_current_op_end_time);         // End time of the current op
    fprintf(stderr,"\tnclk_t     tc_current_op_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_current_op_elapsed_time);        // Elapsed time of the current op
    fprintf(stderr,"\tnclk_t     tc_current_net_start_time=%lld\n",(unsigned long long int)tcp->tc_current_net_start_time);         // Start time of the current network op (e2e only)
    fprintf(stderr,"\tnclk_t     tc_current_net_end_time=%lld\n",(unsigned long long int)tcp->tc_current_net_end_time);         // End time of the current network op (e2e only)
    fprintf(stderr,"\tnclk_t     tc_current_net_elapsed_time=%lld\n",(unsigned long long int)tcp->tc_current_net_elapsed_time);    // Elapsed time of the current network op (e2e only)

    // Accumulated Counters 
    // Updated in the Target Data structure by each Worker Thread at the completion of an I/O operation
    fprintf(stderr,"\tuint64_t   tc_accumulated_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_op_count);         // The number of read+write operations that have completed so far
    fprintf(stderr,"\tuint64_t   tc_accumulated_read_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_read_op_count);    // The number of read operations that have completed so far 
    fprintf(stderr,"\tuint64_t   tc_accumulated_write_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_write_op_count);    // The number of write operations that have completed so far 
    fprintf(stderr,"\tuint64_t   tc_accumulated_noop_op_count=%lld\n",(unsigned long long int)tcp->tc_accumulated_noop_op_count);    // The number of noops that have completed so far 
    fprintf(stderr,"\tuint64_t   tc_accumulated_bytes_xfered=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_xfered);    // Total number of bytes transferred so far (to storage device, not network)
    fprintf(stderr,"\tuint64_t   tc_accumulated_bytes_read=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_read);        // Total number of bytes read so far (from storage device, not network)
    fprintf(stderr,"\tuint64_t   tc_accumulated_bytes_written=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_written);    // Total number of bytes written so far (to storage device, not network)
    fprintf(stderr,"\tuint64_t   tc_accumulated_bytes_noop=%lld\n",(unsigned long long int)tcp->tc_accumulated_bytes_noop);        // Total number of bytes processed by noops so far
    fprintf(stderr,"\tnclk_t     tc_accumulated_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_op_time);         // Accumulated time spent in I/O 
    fprintf(stderr,"\tnclk_t     tc_accumulated_read_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_read_op_time);     // Accumulated time spent in read 
    fprintf(stderr,"\tnclk_t     tc_accumulated_write_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_write_op_time);    // Accumulated time spent in write 
    fprintf(stderr,"\tnclk_t     tc_accumulated_noop_op_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_noop_op_time);    // Accumulated time spent in noops 
    fprintf(stderr,"\tnclk_t     tc_accumulated_pattern_fill_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_pattern_fill_time); // Accumulated time spent in data pattern fill before all I/O operations 
    fprintf(stderr,"\tnclk_t     tc_accumulated_flush_time=%lld\n",(unsigned long long int)tcp->tc_accumulated_flush_time);         // Accumulated time spent doing flush (fsync) operations

    fprintf(stderr,"********* End of Target Counters Data at 0x%p **********\n",tcp);
} // End of xdd_show_target_counters()

/*----------------------------------------------------------------------------*/
/* xdd_show_e2e() - Display values in the specified data structure
 */
void
xdd_show_e2e(xint_e2e_t *e2ep) {
    fprintf(stderr,"********* Start of E2E Data at 0x%p **********\n",e2ep);
    fprintf(stderr,"\tchar       *e2e_dest_hostname='%s'\n",e2ep->e2e_dest_hostname);     // Name of the Destination machine 
    fprintf(stderr,"\tchar       *e2e_src_hostname='%s'\n",e2ep->e2e_dest_hostname);         // Name of the Source machine 
    fprintf(stderr,"\tchar       *e2e_src_file_path='%s'\n",e2ep->e2e_dest_hostname);     // Full path of source file for destination restart file 
    fprintf(stderr,"\ttime_t     e2e_src_file_mtime\n");     // stat -c %Y *e2e_src_file_path, i.e., last modification time
    fprintf(stderr,"\tin_addr_t  e2e_dest_addr=%d\n",e2ep->e2e_dest_addr);          // Destination Address number of the E2E socket 
    fprintf(stderr,"\tin_port_t  e2e_dest_port=%d\n",e2ep->e2e_dest_port);          // Port number to use for the E2E socket 
    fprintf(stderr,"\tint32_t    e2e_sd=%d\n",e2ep->e2e_sd);                   // Socket descriptor for the E2E message port 
    fprintf(stderr,"\tint32_t    e2e_nd=%d\n",e2ep->e2e_nd);                   // Number of Socket descriptors in the read set 
    fprintf(stderr,"\tsd_t       e2e_csd[FD_SETSIZE]\n");;    // Client socket descriptors 
    fprintf(stderr,"\tfd_set     e2e_active?\n");              // This set contains the sockets currently active 
    fprintf(stderr,"\tfd_set     e2e_readset?\n");             // This set is passed to select() 
    fprintf(stderr,"\tstruct sockaddr_in  e2e_sname?\n");                 // used by setup_server_socket 
    fprintf(stderr,"\tuint32_t   e2e_snamelen=%d\n",e2ep->e2e_snamelen);             // the length of the socket name 
    fprintf(stderr,"\tstruct sockaddr_in  e2e_rname?\n");                 // used by destination machine to remember the name of the source machine 
    fprintf(stderr,"\tuint32_t   e2e_rnamelen=%d\n",e2ep->e2e_rnamelen);             // the length of the source socket name 
    fprintf(stderr,"\tint32_t    e2e_current_csd=%d\n",e2ep->e2e_current_csd);         // the current csd used by the select call on the destination side
    fprintf(stderr,"\tint32_t    e2e_next_csd=%d\n",e2ep->e2e_next_csd);             // The next available csd to use 
    fprintf(stderr,"\txdd_e2e_header_t *e2e_hdrp=%p\n",e2ep->e2e_hdrp);                // Pointer to the header portion of a packet
	if (e2ep->e2e_hdrp) xdd_show_e2e_header(e2ep->e2e_hdrp);
    fprintf(stderr,"\tunsigned char *e2e_datap=%p\n",e2ep->e2e_datap);                // Pointer to the data portion of a packet
    fprintf(stderr,"\tint32_t    e2e_header_size=%d\n",e2ep->e2e_header_size);         // Size of the header portion of the buffer 
    fprintf(stderr,"\tint32_t    e2e_data_size=%d\n",e2ep->e2e_data_size);             // Size of the data portion of the buffer
    fprintf(stderr,"\tint32_t    e2e_xfer_size=%d\n",e2ep->e2e_xfer_size);             // Number of bytes per End to End request - size of data buffer plus size of E2E Header
    fprintf(stderr,"\tint32_t    e2e_send_status=%d\n",e2ep->e2e_send_status);         // Current Send Status
    fprintf(stderr,"\tint32_t    e2e_recv_status=%d\n",e2ep->e2e_recv_status);         // Current Recv status
    fprintf(stderr,"\tint64_t    e2e_msg_sequence_number=%lld\n",(long long int)e2ep->e2e_msg_sequence_number);// The Message Sequence Number of the most recent message sent or to be received
    fprintf(stderr,"\tint32_t    e2e_msg_sent=%d\n",e2ep->e2e_msg_sent);             // The number of messages sent 
    fprintf(stderr,"\tint32_t    e2e_msg_recv=%d\n",e2ep->e2e_msg_recv);             // The number of messages received 
    fprintf(stderr,"\tint64_t    e2e_prev_loc=%lld\n",(long long int)e2ep->e2e_prev_loc);             // The previous location from a e2e message from the source 
    fprintf(stderr,"\tint64_t    e2e_prev_len=%lld\n",(long long int)e2ep->e2e_prev_len);             // The previous length from a e2e message from the source 
    fprintf(stderr,"\tint64_t    e2e_data_recvd=%lld\n",(long long int)e2ep->e2e_data_recvd);         // The amount of data that is received each time we call xdd_e2e_dest_recv()
    fprintf(stderr,"\tint64_t    e2e_data_length=%lld\n",(long long int)e2ep->e2e_data_length);         // The amount of data that is ready to be read for this operation 
    fprintf(stderr,"\tint64_t    e2e_total_bytes_written=%lld\n",(long long int)e2ep->e2e_total_bytes_written); // The total amount of data written across all restarts for this file
    fprintf(stderr,"\tnclk_t     e2e_wait_1st_msg=%lld\n",(unsigned long long int)e2ep->e2e_wait_1st_msg);        // Time in nanosecs destination waited for 1st source data to arrive 
    fprintf(stderr,"\tnclk_t     e2e_first_packet_received_this_pass=%lld\n",(unsigned long long int)e2ep->e2e_first_packet_received_this_pass);// Time that the first packet was received by the destination from the source
    fprintf(stderr,"\tnclk_t     e2e_last_packet_received_this_pass=%lld\n",(unsigned long long int)e2ep->e2e_last_packet_received_this_pass);// Time that the last packet was received by the destination from the source
    fprintf(stderr,"\tnclk_t     e2e_first_packet_received_this_run=%lld\n",(unsigned long long int)e2ep->e2e_first_packet_received_this_run);// Time that the first packet was received by the destination from the source
    fprintf(stderr,"\tnclk_t     e2e_last_packet_received_this_run=%lld\n",(unsigned long long int)e2ep->e2e_last_packet_received_this_run);// Time that the last packet was received by the destination from the source
    fprintf(stderr,"\tnclk_t     e2e_sr_time=%lld\n",(unsigned long long int)e2ep->e2e_sr_time);             // Time spent sending or receiving data for End-to-End operation
    fprintf(stderr,"\tint32_t    e2e_address_table_host_count=%d\n",e2ep->e2e_address_table_host_count);    // Cumulative number of hosts represented in the e2e address table
    fprintf(stderr,"\tint32_t    e2e_address_table_port_count=%d\n",e2ep->e2e_address_table_port_count);    // Cumulative number of ports represented in the e2e address table
    fprintf(stderr,"\tint32_t    e2e_address_table_next_entry=%d\n",e2ep->e2e_address_table_next_entry);    // Next available entry in the e2e_address_table
    fprintf(stderr,"\txdd_e2e_ate_t e2e_address_table[E2E_ADDRESS_TABLE_ENTRIES]\n"); // Used by E2E to stripe over multiple IP Addresses
    fprintf(stderr,"********* End of E2E Data at 0x%p **********\n",e2ep);
} // End of xdd_show_e2e()

/*----------------------------------------------------------------------------*/
/* xdd_show_e2e_header() - Display values in the specified data structure
 */
void
xdd_show_e2e_header(xdd_e2e_header_t *e2ehp) {
    fprintf(stderr,"\n\t********* Start of E2E Header Data at 0x%p **********\n",e2ehp);
    fprintf(stderr,"\t\tuint32_t   e2eh_magic=0x%8x\n",e2ehp->e2eh_magic);                 // Magic Number - sanity check
    fprintf(stderr,"\t\tint32_t    e2eh_worker_thread_number=%d\n",e2ehp->e2eh_worker_thread_number);  // Sender's Worker Thread Number
    fprintf(stderr,"\t\tint64_t    e2eh_sequence_number=%lld\n",(long long int)e2ehp->e2eh_sequence_number);       // Sequence number of this operation
    fprintf(stderr,"\t\tnclk_t     e2eh_send_time=%lld\n",(unsigned long long int)e2ehp->e2eh_send_time);             // Time this packet was sent in global nano seconds
    fprintf(stderr,"\t\tnclk_t     e2eh_recv_time=%lld\n",(unsigned long long int)e2ehp->e2eh_recv_time);             // Time this packet was received in global nano seconds
    fprintf(stderr,"\t\tint64_t    e2eh_byte_offset=%lld\n",(long long int)e2ehp->e2eh_byte_offset);           // Offset relative to the beginning of the file of where this data belongs
    fprintf(stderr,"\t\tint64_t    e2eh_data_length=%lld\n",(long long int)e2ehp->e2eh_data_length);           // Length of the user data in bytes for this operation
    fprintf(stderr,"\t********* End of E2E Header Data at 0x%p **********\n",e2ehp);

} // End of xdd_show_e2e_header()


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
