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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_show_ptds() - Display values in the specified Per-Target-Data-Structure 
 */
void
xdd_show_ptds(ptds_t *p) {
	fprintf(stderr,"********* Start of PTDS **********\n");
	fprintf(xgp->output,"target_ptds     0x%p Pointer back to the Parent Target PTDS \n",p->target_ptds); 	
	fprintf(xgp->output,"next_qp         %p Pointer to the next QThread PTDS in the PTDS Substructure \n",p->next_qp); 		
	//pthread_t  			target_thread;		// Handle for this Target Thread 
	//pthread_t  			qthread;			// Handle for this QThread 
	//pthread_t  			issue_thread;		// Handle for this Target's Issue Thread 
	//pthread_t  			completion_thread;	// Handle for this Target's Completion Thread 
	fprintf(xgp->output,"my_target_number, %d My target number \n",p->my_target_number);	
	fprintf(xgp->output,"my_qthread_number,%d My queue number within this target \n",p->my_qthread_number);	
	fprintf(xgp->output,"my_thread_number, %d My thread number relative to the total number of threads \n",p->my_thread_number); 	
	fprintf(xgp->output,"my_thread_id,     %d My system thread ID (like a process ID) \n",p->my_thread_id);  	
	fprintf(xgp->output,"my_pid,           %d My process ID \n",p->my_pid);   			
	fprintf(xgp->output,"total_threads,    %d Total number of threads -> target threads + QThreads \n",p->total_threads); 	
	fprintf(xgp->output,"fd,               %d File Descriptor for the target device/file \n",p->fd);
	fprintf(xgp->output,"target_open_flags,0x%8x Flags used during open processing of a target \n",(unsigned int)p->target_open_flags);
	fprintf(xgp->output,"rwbuf           0x%p The re-aligned I/O buffers \n",p->rwbuf);
	fprintf(xgp->output,"rwbuf_shmid,    0x%8x  Shared Memeory ID for rwbuf \n",(unsigned int)p->rwbuf_shmid);
	fprintf(xgp->output,"rwbuf_save      0x%p The original I/O buffers \n",p->rwbuf_save);
	fprintf(xgp->output,"tthdr           0x%p Pointers to the timestamp table \n",p->ttp);
	fprintf(xgp->output,"iobuffersize,     %d Size of the I/O buffer in bytes \n",p->iobuffersize);
	fprintf(xgp->output,"iosize,           %d Number of bytes per request \n",p->iosize);
	fprintf(xgp->output,"actual_iosize,    %d Number of bytes actually transferred for this request \n",p->actual_iosize);
	fprintf(xgp->output,"last_iosize,      %d Number of bytes for the final request \n",p->last_iosize);
	fprintf(xgp->output,"op_delay,         %d Number of seconds to delay between operations \n",p->op_delay);
	fprintf(xgp->output,"filetype,       0x%8x Type of file: regular, device, socket, ...  \n",(unsigned int)p->filetype);
	fprintf(xgp->output,"filesize,         %lld Size of target file in bytes \n",(long long)p->filesize);
	fprintf(xgp->output,"qthread_ops,      %lld Total number of ops to perform per qthread \n",(long long)p->qthread_ops);
	fprintf(xgp->output,"target_ops,       %lld Total number of ops to perform on behalf of a target \n",(long long)p->target_ops);
//	seekhdr_t			seekhdr;  			// For all the seek information 
//	results_t			qthread_average_results;	// Averaged results for this qthread - accumulated over all passes
//	FILE				*tsfp;   			// Pointer to the time stamp output file 
	fprintf(xgp->output,"bytes_issued      %lld Bytes currently scheduled for I/O but not complete \n",(long long int)p->bytes_issued);
	fprintf(xgp->output,"bytes_completed   %lld Bytes that have completed transfer \n",(long long int)p->bytes_completed);
	fprintf(xgp->output,"bytes_remaining   %lld Bytes remaining to be transferred \n",(long long int)p->bytes_remaining);

	fprintf(xgp->output,"task_request      0x%02x Type of Task to perform \n",p->task_request);
	fprintf(xgp->output,"task_op           0x%02x Operation to perform \n",p->task_op);
	fprintf(xgp->output,"task_xfer_size     %d Number of bytes to transfer \n",p->task_xfer_size);
	fprintf(xgp->output,"task_byte_location %lld The task variables \n",(long long int)p->task_byte_location);
	fprintf(xgp->output,"task_time_to_issue %lld Time to issue the I/O operation or 0 if not used \n",(long long int)p->task_time_to_issue);

	// command line option values 
	fprintf(xgp->output,"start_offset                   %lld starting block offset value \n",(long long int)p->start_offset);
	fprintf(xgp->output,"pass_offset                    %lld number of blocks to add to seek locations between passes \n",(long long int)p->pass_offset);
	fprintf(xgp->output,"flushwrite                     %lld number of write operations to perform between flushes \n",(long long int)p->flushwrite);
	fprintf(xgp->output,"flushwrite_current_count       %lld Running number of write operations - used to trigger a flush operation \n",(long long int)p->flushwrite_current_count);
	fprintf(xgp->output,"bytes                          %lld number of bytes to process overall \n",(long long int)p->bytes);
	fprintf(xgp->output,"numreqs                        %lld Number of requests to perform per pass per qthread\n",(long long int)p->numreqs);
	fprintf(xgp->output,"rwratio                        %f read/write ratios \n",p->rwratio);
	fprintf(xgp->output,"report_threshold               %lld reporting threshold for long operations \n",(long long int)p->report_threshold);
	fprintf(xgp->output,"reqsize                        %d number of *blocksize* byte blocks per operation for each target \n",p->reqsize);
	fprintf(xgp->output,"retry_count                    %d number of retries to issue on an error \n",p->retry_count);
	fprintf(xgp->output,"time_limit                     %f timelimit in seconds for each thread \n",p->time_limit);
	fprintf(xgp->output,"*target_directory             '%s' The target directory for the target \n",(p->target_directory != NULL)?p->target_directory:"NA");
	fprintf(xgp->output,"*target_basename              '%s' devices to perform operation on \n",(p->target_basename != NULL)?p->target_basename:"NA");
	fprintf(xgp->output,"*target_full_pathname         '%s' Fully qualified path name to the target device/file\n",(p->target_full_pathname != NULL)?p->target_full_pathname:"NA");
	fprintf(xgp->output,"target_extension              '%s' The target extension number \n",p->target_extension);
	fprintf(xgp->output,"processor                      %d Processor/target assignments \n",p->processor);
	fprintf(xgp->output,"start_delay                    %f number of picoseconds to delay the start  of this operation \n",p->start_delay);

    fprintf(xgp->output,"Stuff REFERENCED during run time\n");
	fprintf(xgp->output,"run_start_time                 %lld This is time t0 of this run - set by xdd_main \n",(long long int)p->run_start_time);
	fprintf(xgp->output,"first_pass_start_time          %lld Time the first pass started but before the first operation is issued \n",(long long int)p->first_pass_start_time);
	fprintf(xgp->output,"target_bytes_to_xfer_per_pass  %lld Number of bytes to xfer per pass for the entire target (all qthreads) \n",(long long int)p->target_bytes_to_xfer_per_pass);
	fprintf(xgp->output,"qthread_bytes_to_xfer_per_pass %lld Number of bytes to xfer per pass for this qthread \n",(long long int)p->qthread_bytes_to_xfer_per_pass);
	fprintf(xgp->output,"block_size                     %d Size of a block in bytes for this target \n",p->block_size);
	fprintf(xgp->output,"queue_depth                    %d Command queue depth for each target \n",p->queue_depth);
	fprintf(xgp->output,"preallocate                    %lld File preallocation value \n",(long long int)p->preallocate);
	fprintf(xgp->output,"mem_align                      %d Memory read/write buffer alignment value in bytes \n",p->mem_align);
	fprintf(xgp->output,"target_options               0x%016x I/O Options specific to each target \n",(unsigned int)p->target_options);
	fprintf(xgp->output,"last_committed_op              %lld Operation number of last r/w operation relative to zero \n",(long long int)p->last_committed_op);
	fprintf(xgp->output,"last_committed_location        %lld Byte offset into target of last r/w operation \n",(long long int)p->last_committed_location);
	fprintf(xgp->output,"last_committed_length          %d Number of bytes transferred to/from last_committed_location \n",p->last_committed_length);
	
    fprintf(xgp->output,"Stuff UPDATED during each pass\n"); 
	fprintf(xgp->output,"my_current_pass_number         %d Current pass number \n",p->my_current_pass_number);

	fprintf(xgp->output,"Time stamps and timing information - RESET AT THE START OF EACH PASS \n");
	fprintf(xgp->output,"my_pass_start_time             %lld The time stamp that this pass started but before the first operation is issued \n",(long long int)p->my_pass_start_time);
	fprintf(xgp->output,"my_pass_end_time               %lld The time stamp that this pass ended \n",(long long int)p->my_pass_end_time);

	fprintf(xgp->output,"Counters  - RESET AT THE START OF EACH PASS\n");

	fprintf(xgp->output,"Updated by xdd_issue() at at the start of a Task IO request to a QThread\n");
	fprintf(xgp->output,"my_current_byte_location       %lld Current byte location for this I/O operation \n",(long long int)p->my_current_byte_location);
	fprintf(xgp->output,"my_current_io_size             %d Size of the I/O to be performed \n",p->my_current_io_size);
	 fprintf(xgp->output,"my_current_op_str             '%s' - ASCII string of the I/O operation type - 'READ', 'WRITE', or 'NOOP' \n",(p->my_current_op_str != NULL)?p->my_current_op_str:"NA");
	fprintf(xgp->output,"my_current_op_type             %d Current I/O operation type READ=%d, WRITE=%d, NOOP=%d\n",p->my_current_op_type, OP_TYPE_READ, OP_TYPE_WRITE, OP_TYPE_NOOP);

	fprintf(xgp->output,"Updated by the QThread upon completion of an I/O operation\n");
	fprintf(xgp->output,"target_op_number                    %lld Operation number for the target represented by this I/O \n",(long long int)p->target_op_number);
	fprintf(xgp->output,"my_current_op_number                %lld Current I/O operation number \n",(long long int)p->my_current_op_number);
	fprintf(xgp->output,"my_current_op_count                 %lld The number of read+write operations that have completed so far \n",(long long int)p->my_current_op_count);
	fprintf(xgp->output,"my_current_read_op_count            %lld The number of read operations that have completed so far \n",(long long int)p->my_current_read_op_count);
	fprintf(xgp->output,"my_current_write_op_count           %lld The number of write operations that have completed so far \n",(long long int)p->my_current_write_op_count);
	fprintf(xgp->output,"my_current_noop_op_count            %lld The number of noops that have completed so far \n",(long long int)p->my_current_noop_op_count);
	fprintf(xgp->output,"my_current_bytes_xfered             %lld Total number of bytes transferred so far (to storage device, not network) \n",(long long int)p->my_current_bytes_xfered);
	fprintf(xgp->output,"my_current_bytes_xfered_this_op     %lld Total number of bytes transferred on the most recent I/O operation \n",(long long int)p->my_current_bytes_xfered_this_op);
	fprintf(xgp->output,"my_current_bytes_read               %lld Total number of bytes read so far (from storage device, not network) \n",(long long int)p->my_current_bytes_read);
	fprintf(xgp->output,"my_current_bytes_written            %lld Total number of bytes written so far (to storage device, not network) \n",(long long int)p->my_current_bytes_written);
	fprintf(xgp->output,"my_current_bytes_noop               %lld Total number of bytes processed by noops so far \n",(long long int)p->my_current_bytes_noop);
	fprintf(xgp->output,"my_current_io_status                %d I/O Status of the last I/O operation for this qthread \n",p->my_current_io_status);
	fprintf(xgp->output,"my_current_errno                    %d The errno associated with the status of this I/O for this thread \n",p->my_current_io_errno);
	fprintf(xgp->output,"my_current_error_count              %lld The number of I/O errors for this qthread \n",(long long int)p->my_current_error_count);
	fprintf(xgp->output,"my_elapsed_pass_time                %lld Rime between the start and end of this pass \n",(long long int)p->my_elapsed_pass_time);
	fprintf(xgp->output,"my_first_op_start_time              %lld Time this qthread was able to issue its first operation for this pass \n",(long long int)p->my_first_op_start_time);
	fprintf(xgp->output,"my_current_op_start_time            %lld Start time of the current op \n",(long long int)p->my_current_op_start_time);
	fprintf(xgp->output,"my_current_op_end_time              %lld End time of the current op \n",(long long int)p->my_current_op_end_time);
	fprintf(xgp->output,"my_current_elapsed_time             %lld Elapsed time of the current op \n",(long long int)p->my_current_op_elapsed_time);
	fprintf(xgp->output,"my_accumulated_op_time              %lld Accumulated time spent in I/O \n",(long long int)p->my_accumulated_op_time);
	fprintf(xgp->output,"my_accumulated_read_op_time         %lld Accumulated time spent in read \n",(long long int)p->my_accumulated_read_op_time);
	fprintf(xgp->output,"my_accumulated_write_op_time        %lld Accumulated time spent in write \n",(long long int)p->my_accumulated_write_op_time);
	fprintf(xgp->output,"my_accumulated_noop_op_time         %lld Accumulated time spent in noops \n",(long long int)p->my_accumulated_noop_op_time);
	fprintf(xgp->output,"my_accumulated_pattern_fill_time    %lld Accumulated time spent in data pattern fill before all I/O operations \n",(long long int)p->my_accumulated_pattern_fill_time);
	fprintf(xgp->output,"my_accumulated_flush_time           %lld Accumulated time spent doing flush (fsync) operations \n",(long long int)p->my_accumulated_flush_time);
	fprintf(xgp->output,"my_current_state                  0x%08x State of this thread at any given time \n",(unsigned int)p->my_current_state);

	fprintf(stderr,"+++++++++++++ End of PTDS +++++++++++++\n");
} /* end of xdd_show_ptds() */ 
 
 
/*----------------------------------------------------------------------------*/
/* xdd_show_global_data() - Display values in the specified global data structure
 */
void
xdd_show_global_data(void) {
	int		target_number;	


	fprintf(stderr,"********* Start of Global Data **********\n");
	fprintf(xgp->output,"global_options          0x%016llx - I/O Options valid for all targets \n",(long long int)xgp->global_options);
//	fprintf(xgp->output,"current_time_for_this_run %d  - run time for this run \n",xgp->current_time_for_this_run);
	fprintf(xgp->output,"progname                  %s - Program name from argv[0] \n",xgp->progname);
	fprintf(xgp->output,"argc                      %d - The original arg count \n",xgp->argc);
//	fprintf(xgp->output,"argv                    %s\n",xgp->char			**argv;         // The original *argv[]  */
	fprintf(xgp->output,"passes                    %d - number of passes to perform \n",xgp->passes);
	fprintf(xgp->output,"pass_delay                %f - number of seconds to delay between passes \n",xgp->pass_delay);
	fprintf(xgp->output,"max_errors                %lld - max number of errors to tollerate \n",(long long int)xgp->max_errors);
	fprintf(xgp->output,"max_errors_to_print       %lld - Maximum number of compare errors to print \n",(long long int)xgp->max_errors_to_print);
	fprintf(xgp->output,"output_filename          '%s' - name of the output file \n",(xgp->output_filename != NULL)?xgp->output_filename:"NA");
	fprintf(xgp->output,"errout_filename          '%s' - name fo the error output file \n",(xgp->errout_filename != NULL)?xgp->errout_filename:"NA");
	fprintf(xgp->output,"csvoutput_filename       '%s' - name of the csv output file \n",(xgp->csvoutput_filename != NULL)?xgp->csvoutput_filename:"NA");
	fprintf(xgp->output,"combined_output_filename '%s' - name of the combined output file \n",(xgp->combined_output_filename != NULL)?xgp->combined_output_filename:"NA");
	fprintf(xgp->output,"tsbinary_filename        '%s' - timestamp filename prefix \n",(xgp->tsbinary_filename != NULL)?xgp->tsbinary_filename:"NA");
	fprintf(xgp->output,"tsoutput_filename        '%s' - timestamp report output filename prefix \n",(xgp->tsoutput_filename != NULL)?xgp->tsoutput_filename:"NA");
	fprintf(xgp->output,"output                  0x%p - Output file pointer \n",xgp->output);
	fprintf(xgp->output,"errout                  0x%p - Error Output file pointer \n",xgp->errout);
	fprintf(xgp->output,"csvoutput               0x%p - Comma Separated Values output file \n",xgp->csvoutput);
	fprintf(xgp->output,"combined_output         0x%p - Combined output file \n",xgp->combined_output);
	fprintf(xgp->output,"heartbeat                 %d - seconds between heartbeats \n",xgp->heartbeat);
	fprintf(xgp->output,"restart_frequency         %d - seconds between restart monitor checks \n",xgp->restart_frequency);
	fprintf(xgp->output,"syncio                    %d - the number of I/Os to perform btw syncs \n",xgp->syncio);
	fprintf(xgp->output,"target_offset             %lld - offset value \n",(long long int)xgp->target_offset);
	fprintf(xgp->output,"number_of_targets         %d - number of targets to operate on \n",xgp->number_of_targets);
	fprintf(xgp->output,"number_of_iothreads       %d - number of threads spawned for all targets \n",xgp->number_of_iothreads);
	fprintf(xgp->output,"id                       '%s' - ID string pointer \n",(xgp->id != NULL)?xgp->id:"NA");
	fprintf(xgp->output,"run_time                  %f - Length of time to run all targets, all passes \n",xgp->run_time);
	fprintf(xgp->output,"base_time                 %lld - The time that xdd was started - set during initialization \n",(long long int)xgp->base_time);
	fprintf(xgp->output,"run_start_time            %lld - The time that the targets start their first pass - set after initialization \n",(long long int)xgp->run_start_time);
	fprintf(xgp->output,"estimated_time            %lld - The time at which this run (all passes) should end \n",(long long int)xgp->estimated_end_time);
	fprintf(xgp->output,"number_of_processors      %d - Number of processors \n",xgp->number_of_processors);
//	fprintf(xgp->output,"random_init_state         %s\n",xgp->char			random_init_state[256]; 			/* Random number generator state initalizer array */ 
	fprintf(xgp->output,"clock_tick                %d - Number of clock ticks per second \n",xgp->clock_tick);
// Indicators that are used to control exit conditions and the like
	fprintf(xgp->output,"id_firsttime              %d - ID first time through flag \n",xgp->id_firsttime);
	fprintf(xgp->output,"run_error_count_exceeded  %d - The alarm that goes off when the number of errors for this run has been exceeded \n",xgp->run_error_count_exceeded);
	fprintf(xgp->output,"run_time_expired          %d - The alarm that goes off when the total run time has been exceeded \n",xgp->run_time_expired);
	fprintf(xgp->output,"run_complete              %d - Set to a 1 to indicate that all passes have completed \n",xgp->run_complete);
	fprintf(xgp->output,"deskew_ring               %d - The alarm that goes off when the the first thread finishes \n",xgp->deskew_ring);
	fprintf(xgp->output,"abort                     %d - abort the run due to some catastrophic failure \n",xgp->abort);
	fprintf(xgp->output,"random_initialized        %d - Random number generator has been initialized \n",xgp->random_initialized);
/* information needed to access the Global Time Server */
//	in_addr_t		gts_addr;               			/* Clock Server IP address */
//	in_port_t		gts_port;               			/* Clock Server Port number */
//	pclk_t			gts_time;               			/* global time on which to sync */
//	pclk_t			gts_seconds_before_starting; 		/* number of seconds before starting I/O */
//	int32_t			gts_bounce;             			/* number of times to bounce the time off the global time server */
//	pclk_t			gts_delta;              			/* Time difference returned by the clock initializer */
//	char			*gts_hostname;          			/* name of the time server */
//	pclk_t			ActualLocalStartTime;   			/* The time to start operations */

// PThread structures for the main threads
//	pthread_t 		Results_Thread;						// PThread struct for results manager
//	pthread_t 		Heartbeat_Thread;					// PThread struct for heartbeat monitor
//	pthread_t 		Restart_Thread;						// PThread struct for restart monitor
//	pthread_t 		Interactive_Thread;					// PThread struct for Interactive Control processor Thread
//
// Thread Barriers 
//	pthread_mutex_t	xdd_init_barrier_mutex;				/* locking mutex for the xdd_init_barrier() routine */
	fprintf(xgp->output,"barrier_count,            %d Number of barriers on the chain \n",xgp->barrier_count);         				
//
// Variables used by the Results Manager
	fprintf(xgp->output,"format_string,             '%s'\n",(xgp->format_string != NULL)?xgp->format_string:"NA");
	fprintf(xgp->output,"results_header_displayed    %d\n",xgp->results_header_displayed);
	fprintf(xgp->output,"heartbeat_holdoff           %d\n",xgp->heartbeat_holdoff);

	/* Target Specific variables */
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		fprintf(xgp->output,"PTDS pointer for target %d of %d is 0x%p\n",target_number, xgp->number_of_targets, xgp->ptdsp[target_number]);
	}
	//results_t		*target_average_resultsp[MAX_TARGETS];/* Results area for the "target" which is a composite of all its qthreads */
	fprintf(stderr,"********* End of Global Data **********\n");

} /* end of xdd_show_global_data() */ 
 
