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
/* xdd_show_target_data() - Display values in the specified Per-Target-Data-Structure 
 */
void
xdd_show_target_data(target_data_t *tdp) {
	fprintf(stderr,"********* Start of TARGET_DATA %p **********\n", tdp);

    fprintf(stderr,"struct xdd_plan 	*td_planp=%p\n",tdp->td_planp);
	fprintf(stderr,"struct xint_worker_data	*td_next_wdp=%p\n",tdp->td_next_wdp);	// Pointer to the first worker_data struct in the list
	fprintf(stderr,"pthread_t  			td_thread\n");			// Handle for this Target Thread 
	fprintf(stderr,"int32_t   			td_thread_id=%d\n",tdp->td_thread_id);  		// My system thread ID (like a process ID) 
	fprintf(stderr,"int32_t   			td_pid=%d\n",tdp->td_pid);   			// My process ID 
	fprintf(stderr,"int32_t   			td_target_number=%d\n",tdp->td_target_number);	// My target number 
	fprintf(stderr,"uint64_t			td_target_options=%llx\n",(unsigned long long int)tdp->td_target_options); 			// I/O Options specific to each target 
	fprintf(stderr,"int32_t   			td_file_desc=%d\n",tdp->td_file_desc);		// File Descriptor for the target device/file 
	fprintf(stderr,"int32_t				td_open_flags=%x\n",tdp->td_open_flags);		// Flags used during open processing of a target
	fprintf(stderr,"int32_t				td_xfer_size=%d\n",tdp->td_xfer_size);  		// Number of bytes per request 
	fprintf(stderr,"int32_t				td_filetype=%d\n",tdp->td_filetype);  		// Type of file: regular, device, socket, ... 
	fprintf(stderr,"int64_t				td_filesize=%lld\n",(long long int)tdp->td_filesize);  		// Size of target file in bytes 
	fprintf(stderr,"int64_t				td_target_ops=%lld\n",(long long int)tdp->td_target_ops);  	// Total number of ops to perform on behalf of a "target"
	fprintf(stderr,"seekhdr_t			td_seekhdr\n");  		// For all the seek information 
	fprintf(stderr,"FILE				*td_tsfp=%p\n",tdp->td_planp);   		// Pointer to the time stamp output file 
	// The Occupant Strcuture used by the barriers 
	fprintf(stderr,"xdd_occupant_t		td_occupant\n");							// Used by the barriers to keep track of what is in a barrier at any given time
	fprintf(stderr,"char				td_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH]='%s'\n",tdp->td_occupant_name);	// For a Target thread this is "TARGET####", for a Worker Thread it is "TARGET####WORKER####"
	fprintf(stderr,"xdd_barrier_t		*td_current_barrier=%p\n",tdp->td_current_barrier);					// Pointer to the current barrier this Thread is in at any given time or NULL if not in a barrier

	// Target-specific variables
	fprintf(stderr,"xdd_barrier_t		td_target_worker_thread_init_barrier\n");		// Where the Target Thread waits for the Worker Thread to initialize

	fprintf(stderr,"xdd_barrier_t		td_targetpass_worker_thread_passcomplete_barrier\n");// The barrier used to sync targetpass() with all the Worker Threads at the end of a pass
	fprintf(stderr,"xdd_barrier_t		td_targetpass_worker_thread_eofcomplete_barrier\n");// The barrier used to sync targetpass_eof_desintation_side() with a Worker Thread trying to recv an EOF packet


	fprintf(stderr,"uint64_t			td_current_op_number=%lld\n",(long long int)tdp->td_current_op_number);		// Current operation number
	fprintf(stderr,"uint64_t			td_current_byte_offset=%lld\n",(long long int)tdp->td_current_byte_offset);		// Current offset into target
	fprintf(stderr,"uint64_t			td_current_bytes_issued=%lld\n",(long long int)tdp->td_current_bytes_issued);	// The amount of data for all transfer requests that has been issued so far 
	fprintf(stderr,"uint64_t			td_current_bytes_completed=%lld\n",(long long int)tdp->td_current_bytes_completed);	// The amount of data for all transfer requests that has been completed so far
	fprintf(stderr,"uint64_t			td_current_bytes_remaining=%lld\n",(long long int)tdp->td_current_bytes_remaining);	// Bytes remaining to be transferred 

	fprintf(stderr,"char				td_abort=%d\n",tdp->td_abort);					// Abort this operation (either a Worker Thread or a Target Thread)
	fprintf(stderr,"char				td_time_limit_expired=%d\n",tdp->td_time_limit_expired);		// The time limit for this target has expired
	fprintf(stderr,"pthread_mutex_t 	td_current_state_mutex\n"); 	// Mutex for locking when checking or updating the state info
	fprintf(stderr,"int32_t				td_current_state=%x\n",tdp->td_current_state);			// State of this thread at any given time (see Current State definitions below)
	// State Definitions for "my_current_state"
fprintf(stderr,"CURRENT_STATE_INIT                              0x0000000000000001\n");	// Initialization 
fprintf(stderr,"CURRENT_STATE_IO                                0x0000000000000002\n");	// Waiting for an I/O operation to complete
fprintf(stderr,"CURRENT_STATE_DEST_RECEIVE                      0x0000000000000004\n");	// Waiting to receive data - Destination side of an E2E operation
fprintf(stderr,"CURRENT_STATE_SRC_SEND                          0x0000000000000008\n");	// Waiting for "send" to send data - Source side of an E2E operation
fprintf(stderr,"CURRENT_STATE_BARRIER                           0x0000000000000010\n");	// Waiting inside a barrier
fprintf(stderr,"CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE 0x0000000000000020\n");	// Waiting on the "any Worker Thread available" semaphore
fprintf(stderr,"CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE 0x0000000000000040\n");	// Waiting on the "This Worker Thread Available" semaphore
fprintf(stderr,"CURRENT_STATE_PASS_COMPLETE                     0x0000000000000080\n");	// Indicates that this Target Thread has completed a pass
fprintf(stderr,"CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE    0x0000000000000100\n");	// Worker Thread is waiting for the TOT lock in order to update the block number
fprintf(stderr,"CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE   0x0000000000000200\n");	// Worker Thread is waiting for the TOT lock in order to release the next I/O
fprintf(stderr,"CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS        0x0000000000000400\n");	// Worker Thread is waiting for the TOT lock to set the "wait" time stamp
fprintf(stderr,"CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO        0x0000000000000800\n");	// Waiting on the previous I/O op semaphore

    // Target-specific semaphores and associated pointers
    fprintf(stderr,"tot_t				*td_totp=%p\n",tdp->td_totp);								// Pointer to the target_offset_table for this target
    fprintf(stderr,"pthread_cond_t 		td_any_worker_thread_available_condition\n");
    fprintf(stderr,"pthread_mutex_t 	td_any_worker_thread_available_mutex\n");
    fprintf(stderr,"int 				td_any_worker_thread_available=%d\n",tdp->td_any_worker_thread_available);

	// Worker Thread-specific semaphores and associated pointers
	fprintf(stderr,"pthread_mutex_t		td_worker_thread_target_sync_mutex\n");			// Used to serialize access to the Worker Thread-Target Synchronization flags
	fprintf(stderr,"int32_t				td_worker_thread_target_sync=0x%x\n",tdp->td_worker_thread_target_sync);				// Flags used to synchronize a Worker Thread with its Target

fprintf(stderr,"WTSYNC_AVAILABLE			0x00000001\n");				// This Worker Thread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
fprintf(stderr,"WTSYNC_BUSY					0x00000002\n");				// This Worker Thread is busy
fprintf(stderr,"WTSYNC_TARGET_WAITING		0x00000004\n");				// The parent Target is waiting for this Worker Thread to become available, set by xdd_get_specific_qthread, reset by qthread.
fprintf(stderr,"WTSYNC_EOF_RECEIVED			0x00000008\n");				// This Worker Thread received an EOF packet from the Source Side of an E2E Operation
    //sem_t				this_qthread_is_available_sem;		// xdd_get_specific_qthread() routine waits on this for any Worker Thread to become available
    fprintf(stderr,"pthread_cond_t 		td_this_wthread_is_available_condition\n");
    
	// command line option values 
	fprintf(stderr,"int64_t				td_start_offset=%lld\n",(long long int)tdp->td_start_offset); 			// starting block offset value 
	fprintf(stderr,"int64_t				td_pass_offset=%lld\n",(long long int)tdp->td_pass_offset); 			// number of blocks to add to seek locations between passes 
	fprintf(stderr,"int64_t				td_flushwrite=%lld\n",(long long int)tdp->td_flushwrite);  			// number of write operations to perform between flushes 
	fprintf(stderr,"int64_t				td_flushwrite_current_count=%lld\n",(long long int)tdp->td_flushwrite_current_count);  // Running number of write operations - used to trigger a flush (sync) operation 
	fprintf(stderr,"int64_t				td_bytes=%lld\n",(long long int)tdp->td_bytes);   				// number of bytes to process overall 
	fprintf(stderr,"int64_t				td_numreqs=%lld\n",(long long int)tdp->td_numreqs);  				// Number of requests to perform per pass per qthread
	fprintf(stderr,"double				td_rwratio=%f\n",tdp->td_rwratio);  				// read/write ratios 
	fprintf(stderr,"nclk_t				td_report_threshold=%lld\n",(long long int)tdp->td_report_threshold);		// reporting threshold for long operations 
	fprintf(stderr,"int32_t				td_reqsize=%d\n",tdp->td_reqsize);  				// number of *blocksize* byte blocks per operation for each target 
	fprintf(stderr,"int32_t				td_retry_count=%d\n",tdp->td_retry_count);  			// number of retries to issue on an error 
	fprintf(stderr,"double				td_time_limit=%f\n",tdp->td_time_limit);				// Time of a single pass in seconds
	fprintf(stderr,"nclk_t				td_time_limit_ticks=%lld\n",(long long int)tdp->td_time_limit_ticks);		// Time of a single pass in high-res clock ticks
	fprintf(stderr,"char				*td_target_directory=%s\n",tdp->td_target_directory); 		// The target directory for the target 
	fprintf(stderr,"char				*td_target_basename=%s\n",tdp->td_target_basename); 		// Basename of the target/device
	fprintf(stderr,"char				*td_target_full_pathname=%s\n",tdp->td_target_full_pathname);	// Fully qualified path name to the target device/file
	fprintf(stderr,"char				td_target_extension[32]=%s\n",tdp->td_target_extension); 	// The target extension number 
	fprintf(stderr,"int32_t				td_processor=%d\n",tdp->td_processor);  				// Processor/target assignments 
	fprintf(stderr,"double				td_start_delay=%f\n",tdp->td_start_delay); 			// number of seconds to delay the start  of this operation 
	fprintf(stderr,"nclk_t				td_start_delay_psec=%lld\n",(long long int)tdp->td_start_delay_psec);		// number of nanoseconds to delay the start  of this operation 
	fprintf(stderr,"char				td_random_init_state[256]\n"); 	// Random number generator state initalizer array 
	fprintf(stderr,"int32_t				td_block_size=%d\n",tdp->td_block_size);  			// Size of a block in bytes for this target 
	fprintf(stderr,"int32_t				td_queue_depth=%d\n",tdp->td_queue_depth); 			// Command queue depth for each target 
	fprintf(stderr,"int64_t				td_preallocate=%lld\n",(long long int)tdp->td_preallocate); 			// File preallocation value 
	fprintf(stderr,"int32_t				td_mem_align=%d\n",tdp->td_mem_align);   			// Memory read/write buffer alignment value in bytes 
    //
    // ------------------ Heartbeat stuff --------------------------------------------------
	// The following heartbeat structure and data is for the -heartbeat option
	fprintf(stderr,"struct heartbeat	td_hb=%p\n",tdp->td_planp);					// Heartbeat data
    //
    // ------------------ RUNTIME stuff --------------------------------------------------
    // Stuff REFERENCED during runtime
	//
	fprintf(stderr,"nclk_t				td_run_start_time=%lld\n",(long long int)tdp->td_run_start_time); 			// This is time t0 of this run - set by xdd_main
	fprintf(stderr,"nclk_t				td_first_pass_start_time=%lld\n",(long long int)tdp->td_first_pass_start_time); 	// Time the first pass started but before the first operation is issued
	fprintf(stderr,"uint64_t			td_target_bytes_to_xfer_per_pass=%lld\n",(long long int)tdp->td_target_bytes_to_xfer_per_pass); 	// Number of bytes to xfer per pass for the entire target (all qthreads)
	fprintf(stderr,"int64_t				td_last_committed_op=%lld\n",(long long int)tdp->td_last_committed_op);		// Operation number of last r/w operation relative to zero
	fprintf(stderr,"uint64_t			td_last_committed_location=%lld\n",(long long int)tdp->td_last_committed_location);	// Byte offset into target of last r/w operation
	fprintf(stderr,"int32_t				td_last_committed_length=%d\n",tdp->td_last_committed_length);	// Number of bytes transferred to/from last_committed_location
	//
    // Stuff UPDATED during each pass 
	fprintf(stderr,"int32_t				td_syncio_barrier_index=%d\n",tdp->td_syncio_barrier_index); 		// Used to determine which syncio barrier to use at any given time
	fprintf(stderr,"int32_t				td_results_pass_barrier_index=%d\n",tdp->td_results_pass_barrier_index); // Where a Target thread waits for all other Target threads to complete a pass
	fprintf(stderr,"int32_t				td_results_display_barrier_index=%d\n",tdp->td_results_display_barrier_index); // Where threads wait for the results_manager()to display results
	fprintf(stderr,"int32_t				td_results_run_barrier_index=%d\n",tdp->td_results_run_barrier_index); 	// Where threads wait for all other threads at the completion of the run

	// The following variables are used by the "-reopen" option
	fprintf(stderr,"nclk_t        		td_open_start_time=%lld\n",(long long int)tdp->td_open_start_time); 		// Time just before the open is issued for this target 
	fprintf(stderr,"nclk_t        		td_open_end_time=%lld\n",(long long int)tdp->td_open_end_time); 			// Time just after the open completes for this target 
	fprintf(stderr,"pthread_mutex_t 	td_counters_mutex\n"); 			// Mutex for locking when updating td_counters
	fprintf(stderr,"struct xint_target_counters	td_counters\n");		// Pointer to the target counters
	fprintf(stderr,"struct xint_throttle		*td_throtp=%p\n",tdp->td_throtp);			// Pointer to the throttle sturcture
	fprintf(stderr,"struct xint_timestamp		*td_tsp=%p\n",tdp->td_tsp);			// Pointer to the time stamp stuff
	fprintf(stderr,"struct xdd_tthdr			*td_ttp=%p\n",tdp->td_ttp);			// Pointer to the time stamp stuff
	fprintf(stderr,"struct xint_e2e				*td_e2ep=%p\n",tdp->td_e2ep);			// Pointer to the e2e struct when needed
	fprintf(stderr,"struct xint_extended_stats	*td_esp=%p\n",tdp->td_esp);			// Extended Stats Structure Pointer
	fprintf(stderr,"struct xint_triggers		*td_trigp=%p\n",tdp->td_trigp);			// Triggers Structure Pointer
	fprintf(stderr,"struct xint_data_pattern	*td_dpp=%p\n",tdp->td_dpp);			// Data Pattern Structure Pointer
	fprintf(stderr,"struct xint_raw				*td_rawp=%p\n",tdp->td_rawp);          	// RAW Data Structure Pointer
	fprintf(stderr,"struct lockstep				*td_lsp=%p\n",tdp->td_lsp);			// Pointer to the lockstep structure used by the lockstep option
	fprintf(stderr,"struct xint_restart			*td_restartp=%p\n",tdp->td_restartp);		// Pointer to the restart structure used by the restart monitor
	fprintf(stderr,"struct ptds					*td_tdpm1=%p\n",tdp->td_tdpm1);			// Target_Data minus  1 - used for report print queueing - don't ask 
	fprintf(stderr,"struct stat					td_statbuf\n");			// Target File Stat buffer used by xdd_target_open()
	fprintf(stderr,"int32_t						td_op_delay=%d\n",tdp->td_op_delay); 		// Number of seconds to delay between operations 
	fprintf(stderr,"+++++++++++++ End of TARGET_DATA +++++++++++++\n");
} /* end of xdd_show_target_data() */ 
 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_global_data() - Display values in the specified global data structure
 */
void
xdd_show_global_data(void) {
	fprintf(stderr,"********* Start of Global Data **********\n");
	fprintf(xgp->output,"global_options          0x%016llx - I/O Options valid for all targets \n",(long long int)xgp->global_options);
//	fprintf(xgp->output,"current_time_for_this_run %d  - run time for this run \n",xgp->current_time_for_this_run);
	fprintf(xgp->output,"progname                  %s - Program name from argv[0] \n",xgp->progname);
	fprintf(xgp->output,"argc                      %d - The original arg count \n",xgp->argc);
//	fprintf(xgp->output,"argv                    %s\n",xgp->char			**argv;         // The original *argv[]  */
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
//	fprintf(xgp->output,"random_init_state         %s\n",xgp->char			random_init_state[256]; 			/* Random number generator state initalizer array */ 
	fprintf(xgp->output,"clock_tick                %d - Number of clock ticks per second \n",xgp->clock_tick);
// Indicators that are used to control exit conditions and the like
	fprintf(xgp->output,"id_firsttime              %d - ID first time through flag \n",xgp->id_firsttime);
	fprintf(xgp->output,"run_error_count_exceeded  %d - The alarm that goes off when the number of errors for this run has been exceeded \n",xgp->run_error_count_exceeded);
	fprintf(xgp->output,"run_time_expired          %d - The alarm that goes off when the total run time has been exceeded \n",xgp->run_time_expired);
	fprintf(xgp->output,"run_complete              %d - Set to a 1 to indicate that all passes have completed \n",xgp->run_complete);
	fprintf(xgp->output,"abort                     %d - abort the run due to some catastrophic failure \n",xgp->abort);
	fprintf(xgp->output,"random_initialized        %d - Random number generator has been initialized \n",xgp->random_initialized);
/* information needed to access the Global Time Server */
//	in_addr_t		gts_addr;               			/* Clock Server IP address */
//	in_port_t		gts_port;               			/* Clock Server Port number */
//	nclk_t			gts_time;               			/* global time on which to sync */
//	nclk_t			gts_seconds_before_starting; 		/* number of seconds before starting I/O */
//	int32_t			gts_bounce;             			/* number of times to bounce the time off the global time server */
//	nclk_t			gts_delta;              			/* Time difference returned by the clock initializer */
//	char			*gts_hostname;          			/* name of the time server */
//	nclk_t			ActualLocalStartTime;   			/* The time to start operations */

// PThread structures for the main threads
//	pthread_t 		Results_Thread;						// PThread struct for results manager
//	pthread_t 		Heartbeat_Thread;					// PThread struct for heartbeat monitor
//	pthread_t 		Restart_Thread;						// PThread struct for restart monitor
//	pthread_t 		Interactive_Thread;					// PThread struct for Interactive Control processor Thread
//
// Thread Barriers 
//	pthread_mutex_t	xdd_init_barrier_mutex;				/* locking mutex for the xdd_init_barrier() routine */
//
// Variables used by the Results Manager
	fprintf(stderr,"********* End of Global Data **********\n");

} /* end of xdd_show_global_data() */ 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_plan_data() - Display values in the specified global data structure
 */
void
xdd_show_plan_data(xdd_plan_t* planp) {
	int		target_number;	


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
	fprintf(xgp->output,"estimated_end_time            %lld - The time at which this run (all passes) should end \n",(long long int)planp->estimated_end_time);
	fprintf(xgp->output,"number_of_processors      %d - Number of processors \n",planp->number_of_processors);
//	fprintf(xgp->output,"random_init_state         %s\n",xgp->char			random_init_state[256]; 			/* Random number generator state initalizer array */ 
	fprintf(xgp->output,"clock_tick                %d - Number of clock ticks per second \n",planp->clock_tick);
// Indicators that are used to control exit conditions and the like
	fprintf(xgp->output,"barrier_count,            %d Number of barriers on the chain \n",planp->barrier_count);         				
//
// Variables used by the Results Manager
	fprintf(xgp->output,"format_string,             '%s'\n",(planp->format_string != NULL)?planp->format_string:"NA");
	fprintf(xgp->output,"results_header_displayed    %d\n",planp->results_header_displayed);
	fprintf(xgp->output,"heartbeat_holdoff           %d\n",planp->heartbeat_holdoff);

	/* Target Specific variables */
	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		fprintf(xgp->output,"Target_Data pointer for target %d of %d is 0x%p\n",target_number, planp->number_of_targets, planp->target_datap[target_number]);
	}
	fprintf(stderr,"********* End of Plan Data **********\n");

} /* end of xdd_show_plan_data() */ 
 
