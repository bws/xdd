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
 * This file contains the subroutines that perform various functions 
 * with respect to the PTDS and the PTDS substructure.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_init_target_state() - Initialize the default target state struct
 */
void
xdd_init_target_state(xdd_target_state_t *tgtstp) {
	
	// Time stamps and timing information - RESET AT THE START OF EACH PASS (or Operation on some)
	////nclk_t				my_pass_start_time; 		// The time stamp that this pass started but before the first operation is issued
	tgtstp->my_pass_start_time=0; 		// The time stamp that this pass started but before the first operation is issued
	////nclk_t				my_pass_end_time; 			// The time stamp that this pass ended 
	tgtstp->my_pass_end_time=0; 			// The time stamp that this pass ended 
	////struct	tms			my_starting_cpu_times_this_run;	// CPU times from times() at the start of this run
	////struct	tms			my_starting_cpu_times_this_pass;// CPU times from times() at the start of this pass
	////struct	tms			my_current_cpu_times;		// CPU times from times()
	//
	// Updated by xdd_issue() at at the start of a Task IO request to a QThread
	////int32_t				my_current_pass_number; 	// Current pass number 
	tgtstp->my_current_pass_number=0; 	// Current pass number 
	////int64_t				my_current_byte_location; 	// Current byte location for this I/O operation 
	tgtstp->my_current_byte_location=0; 	// Current byte location for this I/O operation 
	////int32_t				my_current_io_size; 		// Size of the I/O to be performed
	tgtstp->my_current_io_size=0; 		// Size of the I/O to be performed
	////char				*my_current_op_str; 		// Pointer to an ASCII string of the I/O operation type - "READ", "WRITE", or "NOOP"
	tgtstp->my_current_op_str="\0"; 		// Pointer to an ASCII string of the I/O operation type - "READ", "WRITE", or "NOOP"
	////int32_t				my_current_op_type; 		// Current I/O operation type - OP_TYPE_READ or OP_TYPE_WRITE
	tgtstp->my_current_op_type=0;; 		// Current I/O operation type - OP_TYPE_READ or OP_TYPE_WRITE
	// Updated by the QThread upon completion of an I/O operation
	////int64_t				target_op_number;			// The operation number for the target that this I/O represents
	tgtstp->target_op_number=0;			// The operation number for the target that this I/O represents
	////int64_t				my_current_op_number;		// Current I/O operation number 
	tgtstp->my_current_op_number=0;		// Current I/O operation number 
	////int64_t				my_current_op_count; 		// The number of read+write operations that have completed so far
	tgtstp->my_current_op_count=0; 		// The number of read+write operations that have completed so far
	////int64_t				my_current_read_op_count;	// The number of read operations that have completed so far 
	tgtstp->my_current_read_op_count=0;	// The number of read operations that have completed so far 
	////int64_t				my_current_write_op_count;	// The number of write operations that have completed so far 
	tgtstp->my_current_write_op_count=0;	// The number of write operations that have completed so far 
	////int64_t				my_current_noop_op_count;	// The number of noops that have completed so far 
	tgtstp->my_current_noop_op_count=0;	// The number of noops that have completed so far 
	////int64_t				my_current_bytes_xfered_this_op; // Total number of bytes transferred for the most recent completed I/O operation
	tgtstp->my_current_bytes_xfered_this_op=0; // Total number of bytes transferred for the most recent completed I/O operation
	////int64_t				my_current_bytes_xfered;	// Total number of bytes transferred so far (to storage device, not network)
	tgtstp->my_current_bytes_xfered=0;	// Total number of bytes transferred so far (to storage device, not network)
	////int64_t				my_current_bytes_read;		// Total number of bytes read so far (from storage device, not network)
	tgtstp->my_current_bytes_read=0;		// Total number of bytes read so far (from storage device, not network)
	////int64_t				my_current_bytes_written;	// Total number of bytes written so far (to storage device, not network)
	tgtstp->my_current_bytes_written=0;	// Total number of bytes written so far (to storage device, not network)
	////int64_t				my_current_bytes_noop;		// Total number of bytes processed by noops so far
	tgtstp->my_current_bytes_noop=0;		// Total number of bytes processed by noops so far
	////int32_t				my_current_io_status; 		// I/O Status of the last I/O operation for this qthread
	tgtstp->my_current_io_status=0; 		// I/O Status of the last I/O operation for this qthread
	////int32_t				my_current_io_errno; 		// The errno associated with the status of this I/O for this thread
	tgtstp->my_current_io_errno=0; 		// The errno associated with the status of this I/O for this thread
	////int64_t				my_current_error_count;		// The number of I/O errors for this qthread
	tgtstp->my_current_error_count=0;		// The number of I/O errors for this qthread
	////nclk_t				my_elapsed_pass_time; 		// Rime between the start and end of this pass
	tgtstp->my_elapsed_pass_time=0; 		// Rime between the start and end of this pass
	////nclk_t				my_first_op_start_time;		// Time this qthread was able to issue its first operation for this pass
	tgtstp->my_first_op_start_time=0;		// Time this qthread was able to issue its first operation for this pass
	////nclk_t				my_current_op_start_time; 	// Start time of the current op
	tgtstp->my_current_op_start_time=0; 	// Start time of the current op
	////nclk_t				my_current_op_end_time; 	// End time of the current op
	tgtstp->my_current_op_end_time=0; 	// End time of the current op
	////nclk_t				my_current_op_elapsed_time;	// Elapsed time of the current op
	tgtstp->my_current_op_elapsed_time=0;	// Elapsed time of the current op
	////nclk_t				my_current_net_start_time; 	// Start time of the current network op (e2e only)
	tgtstp->my_current_net_start_time=0; 	// Start time of the current network op (e2e only)
	////nclk_t				my_current_net_end_time; 	// End time of the current network op (e2e only)
	tgtstp->my_current_net_end_time=0; 	// End time of the current network op (e2e only)
	////nclk_t				my_current_net_elapsed_time;// Elapsed time of the current network op (e2e only)
	tgtstp->my_current_net_elapsed_time=0;// Elapsed time of the current network op (e2e only)
	////nclk_t				my_accumulated_op_time; 	// Accumulated time spent in I/O 
	tgtstp->my_accumulated_op_time=0; 	// Accumulated time spent in I/O 
	////nclk_t				my_accumulated_read_op_time; // Accumulated time spent in read 
	tgtstp->my_accumulated_read_op_time=0; // Accumulated time spent in read 
	////nclk_t				my_accumulated_write_op_time;// Accumulated time spent in write 
	tgtstp->my_accumulated_write_op_time=0;// Accumulated time spent in write 
	////nclk_t				my_accumulated_noop_op_time;// Accumulated time spent in noops 
	tgtstp->my_accumulated_noop_op_time=0;// Accumulated time spent in noops 
	////nclk_t				my_accumulated_pattern_fill_time; // Accumulated time spent in data pattern fill before all I/O operations 
	tgtstp->my_accumulated_pattern_fill_time=0; // Accumulated time spent in data pattern fill before all I/O operations 
	////nclk_t				my_accumulated_flush_time; 	// Accumulated time spent doing flush (fsync) operations
	tgtstp->my_accumulated_flush_time=0; 	// Accumulated time spent doing flush (fsync) operations
	// Updated by the QThread at different times
	////char				my_time_limit_expired;		// Time limit expired indicator
	tgtstp->my_time_limit_expired=0;		// Time limit expired indicator
	////char				abort;						// Abort this operation (either a QThread or a Target Thread)
	tgtstp->abort=0;						// Abort this operation (either a QThread or a Target Thread)
	////char				run_complete;				// Indicates that the entire RUN of all PASSES has completed
	tgtstp->run_complete=0;				// Indicates that the entire RUN of all PASSES has completed
	////pthread_mutex_t 	my_current_state_mutex; 	// Mutex for locking when checking or updating the state info
	////int32_t				my_current_state;			// State of this thread at any given time (see Current State definitions below)
	tgtstp->my_current_state=0;			// State of this thread at any given time (see Current State definitions below)

} // End of xdd_init_target_state()

/*----------------------------------------------------------------------------*/
/* xdd_init_new_ptds() - Initialize the default Per-Target-Data-Structure 
 * Note: CLO == Command Line Option
 */
void
xdd_init_new_ptds(ptds_t *p, int32_t n) {


	p->next_qp = 0; // set upon creation, used when other qthreads are created
	p->my_target_number = n; // set upon creation of this PTDS
	p->my_qthread_number = 0; // Set upon creation
	p->my_pid = getpid(); // Set upon creation
	p->my_thread_id = 0; // This is set later by the actual thread 
	p->pm1 = 0; // set upon creation
	p->rwbuf = 0; // set during rwbuf allocation
	p->rwbuf_shmid = -1; // set upon creation of a shared memory segment
	p->rwbuf_save = 0; // used by the rwbuf allocation routine
	p->target_directory = DEFAULT_TARGETDIR; // can be changed by CLO
	p->target_basename = DEFAULT_TARGET;  // can be changed by CLO
	sprintf(p->target_extension,"%08d",1);  // can be changed by CLO
	p->reqsize = DEFAULT_REQSIZE;  // can be changed by CLO
	p->throttle = DEFAULT_THROTTLE;
	p->throttle_variance = DEFAULT_VARIANCE;
	p->throttle_type = PTDS_THROTTLE_BW;
        xdd_get_tsp(p);
	p->tsp->ts_options = DEFAULT_TS_OPTIONS;
	p->target_options = DEFAULT_TARGET_OPTIONS; // Zero the target options field
	p->time_limit = DEFAULT_TIME_LIMIT;
	if (p->trigp) p->trigp->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
	p->numreqs = 0; // This must init to 0
	p->report_threshold = DEFAULT_REPORT_THRESHOLD;
	p->flushwrite_current_count = 0;
	p->flushwrite = DEFAULT_FLUSHWRITE;
	p->bytes = 0; // This must init to 0
	p->start_offset = DEFAULT_STARTOFFSET;
	p->pass_offset = DEFAULT_PASSOFFSET;
	p->preallocate = DEFAULT_PREALLOCATE;
	p->queue_depth = DEFAULT_QUEUEDEPTH;
	p->dpp->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
	p->dpp->data_pattern = (unsigned char *)DEFAULT_DATA_PATTERN;
	p->dpp->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
	p->dpp->data_pattern_prefix = (unsigned char *)DEFAULT_DATA_PATTERN_PREFIX;
	p->dpp->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
	p->block_size = DEFAULT_BLOCKSIZE;
	p->mem_align = getpagesize();

	p->processor = -1;
	p->start_delay = DEFAULT_START_DELAY;
	/* Init the Trigger Structure members if there is a trigger struct */
	if (p->trigp) {
		p->trigp->start_trigger_time = 0; /* Time to trigger another target to start */
		p->trigp->stop_trigger_time = 0; /* Time to trigger another target to stop */
		p->trigp->start_trigger_op = 0; /* Operation number to trigger another target to start */
		p->trigp->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		p->trigp->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		p->trigp->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		p->trigp->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		p->trigp->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		p->trigp->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		p->trigp->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		p->trigp->trigger_types = 0;
	}
	/* Init the seeklist header fields */
	p->seekhdr.seek_options = 0;
	p->seekhdr.seek_range = DEFAULT_RANGE;
	p->seekhdr.seek_seed = DEFAULT_SEED;
	p->seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
	p->seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
	p->seekhdr.seek_num_rw_ops = 0;
	p->seekhdr.seek_total_ops = 0;
	p->seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
	p->seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
	p->seekhdr.seek_savefile = NULL; /* file to save seek locations into */
	p->seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
	p->seekhdr.seek_pattern = "sequential";
	/* Init the read-after-write fields */
	if (p->rawp) {
		p->rawp->raw_sd = 0; /* raw socket descriptor */
		p->rawp->raw_hostname = NULL;  /* Reader hostname */
		p->rawp->raw_lag = DEFAULT_RAW_LAG; 
		p->rawp->raw_port = DEFAULT_RAW_PORT;
		p->rawp->raw_trigger = PTDS_RAW_MP; /* default to a message passing */
	}
	/* Init the end-to-end fields */
	if (p->e2ep) {
		p->e2ep->e2e_sd = 0; /* destination machine socket descriptor */
		p->e2ep->e2e_src_hostname = NULL;  /* E2E source hostname */
		p->e2ep->e2e_dest_hostname = NULL;  /* E2E destination hostname */
		p->e2ep->e2e_dest_port = DEFAULT_E2E_PORT;
		p->e2ep->e2e_address_table_host_count = 0;
		p->e2ep->e2e_address_table_port_count = 0;
		p->e2ep->e2e_dest_addr = 0;
		p->e2ep->e2e_wait_1st_msg = 0;
		p->e2ep->e2e_address_table_next_entry=0;
	}

	xdd_init_target_state(p->tgtstp);

	sprintf(p->occupant_name,"TARGET%04d",p->my_target_number);
	xdd_init_barrier_occupant(&p->occupant, p->occupant_name, XDD_OCCUPANT_TYPE_TARGET, p);

} /* end of xdd_init_new_ptds() */

/*----------------------------------------------------------------------------*/
/* xdd_build_ptds_substructure_calculate_xfer_info() - Will calculate the number of data
 * transfers to perform as well as the total number of bytes for the specified
 * target. 
 * This subroutine is only called by xdd_build_ptds_substructure() and is
 * given a pointer to a Target PTDS to operate on.
 */
void
xdd_calculate_xfer_info(ptds_t *tp) {
	// The following calculates the number of I/O requests (numreqs) to issue to a "target"
	// This value represents the total number of I/O operations that will be performed on this target.
	/* Now lets get down to business... */
	tp->iosize = tp->reqsize * tp->block_size;
	if (tp->iosize == 0) {
		fprintf(xgp->errout,"%s: io_thread_init: ALERT! iothread for target %d queue %d has an iosize of 0, reqsize of %d, blocksize of %d\n",
			xgp->progname, tp->my_target_number, tp->my_qthread_number, tp->reqsize, tp->block_size);
		fflush(xgp->errout);
		tp->target_bytes_to_xfer_per_pass = 0;
		return;
	}
	if (tp->numreqs) 
		tp->target_bytes_to_xfer_per_pass = (uint64_t)(tp->numreqs * tp->iosize);
	else if (tp->bytes)
		tp->target_bytes_to_xfer_per_pass = (uint64_t)tp->bytes;
	else { // Yikes - something was not specified
		fprintf(xgp->errout,"%s: io_thread_init: ERROR! iothread for target %d queue %d has numreqs of %lld, bytes of %lld - one of these must be specified\n",
			xgp->progname, tp->my_target_number, tp->my_qthread_number, (long long)tp->numreqs, (long long)tp->bytes);
		fflush(xgp->errout);
		tp->target_bytes_to_xfer_per_pass = 0;
		return;
	}

	// Check to see if this is a restart operation
	// If a restart copy resume operation has been requested then a restart structure will
	// have been allocated and p->restartp should point to that structure. 
	if (tp->restartp) {
		// We have a good restart pointer 
		if (tp->restartp->flags & RESTART_FLAG_RESUME_COPY) {
			// Change the startoffset to reflect the shift in starting point
			tp->start_offset = tp->restartp->byte_offset / tp->block_size;
			// Change the bytes-to-be-transferred to reflect the shift in starting point
			// Since qthreads transfer 1/qd*totalbytes we need to recalculate this carefully
			tp->target_bytes_to_xfer_per_pass -= tp->restartp->byte_offset;
		}
	}

	// This calculates the number of iosize (or smaller) operations that need to be performed. 
	tp->target_ops = tp->target_bytes_to_xfer_per_pass / tp->iosize;

 	// In the event the number of bytes to transfer is not an integer multiple of iosized requests then 
 	// the total number of ops is incremented by 1 and the last I/O op will be the something less than
	// than the normal iosize.
	if (tp->target_bytes_to_xfer_per_pass % tp->iosize) 
		tp->target_ops++;
	
} // End of xdd_calculate_xfer_info()
	
/*----------------------------------------------------------------------------*/
/* xdd_build_ptds_create_qthread_ptds() - will allocate and initialize a new
 * PTDS for a qthread based on the Target PTDS that is passed in. 
 * This subroutine is also passed the queue number which is a number from 0 to
 * queue depth minus 1.
 * This subroutine is only called by xdd_build_ptds_substructure as part of the
 * loop that creates all the qthreads for a target. 
 * 
 *  
 */
ptds_t *
xdd_create_qthread_ptds(ptds_t *tp, int32_t q) {
	ptds_t 		*newqp;			// QThread PTDS pointer

	// This is the upperlevel PTDS and it's next_qp needs to point to this new PTDS
	newqp = malloc(sizeof(struct ptds));
	if (newqp == NULL) {
		fprintf(xgp->errout,"%s: error getting memory for PTDS for target %d queue number %d\n",
			xgp->progname, tp->my_target_number, q);
		return(NULL);
	}

	// Copy the Target PTDS into the new qthread PTDS 
	memcpy(newqp, tp, sizeof(struct ptds));
	newqp->next_qp = NULL; 
	newqp->restartp = NULL; // Zero this because the Target PTDS is the only PTDS with a restart struct
	newqp->my_qthread_number = q;
	
	sprintf(newqp->occupant_name,"TARGET%04d_QTHREAD%04d",newqp->my_target_number,newqp->my_qthread_number); 
	xdd_init_barrier_occupant(&newqp->occupant, newqp->occupant_name, XDD_OCCUPANT_TYPE_QTHREAD, newqp);
	
	return(newqp);

} // End of xdd_create_qthread_ptds()
	
/*----------------------------------------------------------------------------*/
/* xdd_build_ptds_substructure() - given that all the targets have been
 * defined by the command-line input, create any new PTDS's for targets that
 * have queue depths greater than 1.
 * This subroutine is only called by xdd_parse(). 
 *
 * For any given target, if the queue depth is greater than 1 then we create a QThread PTDS for each Qthread.
 * Each new QThread ptds is a copy of the base QThread - QThread 0 - for the associated target. The only
 * thing that distinguishes one QThread ptds from another is the "my_qthread_number" member which identifies the QThread for that ptds.
 *
 * The number of QThreads is equal to the qdepth for that target.
 * The "next_qp" member of the Target PTDS points to the QThread 0. 
 * The "next_qp" member in the QThread 0 ptds points to QThread 1, and so on. The last QThread "next_qp" member is 0.
 * 
 *  Target0         Target1         Target2 .....  TargetN
 *   TargetThread    TargetThread    TargetThread   TargetThread
 *   |               |               |              |
 *   V               V               V              V
 *   QThread0        QThread0        QThread0       QThread0
 *   |               |               |              |
 *   V               V               V              V
 *   QThread1        QThread1        NULL           QThread1
 *   |               |                              |
 *   V               V                              V
 *   QThread2        NULL                           QThread2
 *   |                                              |
 *   V                                              V
 *  NULL                                            QThread3
 *                                                  |
 *                                                  V
 *                                                  NULL  
 * The above example shows that there are N targets, each target having a Target Thread that
 * points to at least one QThread0. For queue depths greater than 1, the additional qthreads
 * are linked in a chain as shown. For example, Target0 has a -queuedepth of 3 and thus
 * has 3 QThreads (0-2). Similarly, Target 1 has a -queuedepth of 2 (QThreads 0-1), 
 * Target3 has a queuedepth of 1 (the default) and hence has no QThreads other than 0. 
 * Target N has a queuedepth of 4, possibly more.  
 *  
 */
void
xdd_build_ptds_substructure(xdd_plan_t* planp) {
	int32_t 	q;				// working variable 
	int32_t 	target_number;	// working variable 
	int32_t		entry;			// Entry number
	int32_t		number_of_ports;// Number of ports to distribute - working variable
	ptds_t 		*tp;			// Target PTDS pointer
	ptds_t 		*qp;			// QThread PTDS pointer

	
	/* For each target check to see if we need to add ptds structures for queue-depths greater than 1 */
	planp->number_of_iothreads = 0;

	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		// The ptdsp[] array contains pointers to the TargetThread PTDS for each Target. 
		// The TargetThread PTDS is allocated during parsing for each target that is identified.
		tp = planp->ptdsp[target_number];

		planp->number_of_iothreads++;

		// If this is an end-to-end operation, figure out the number of QThreads 
		// which is based on the number of addresses:ports combinations specified
		// There are XXXX different scenarios:
		//    1) The address:base_port,port_count entries have been spcified in full.  
		//		 In this case the number of QThreads will be set to meet the cumulative
		//       number of ports specified.
		//       Example: If the e2e_address_table contains 4 entries like so:
		//              10.0.1.24:50010,8
		//              10.0.2.24:50010,7
		//              10.0.3.24:50010,6
		//              10.0.4.24:50010,5
		//         then a total of 8+7+6+5 or 26 ports have been specified hence
		//         the Queue_Depth will be set to 26 for this target. 
		//   2) The address:base_port,port_count has been partially specified in that only the 
		//      address:base_port values are valid and the port_count is zero. In this case the
		//      user should have used the -queuedepth option to specify a queue depth which will
		//      be used to determine the actual port_count for each of the entries. 
		//      Example: If the user specified "-queuedepth 12" and the e2e_address_table contains 
		//		4 entries, then the ports will be distributed three to an entry
		//              10.0.1.24:20010,3
		//              10.0.2.24:20010,3
		//              10.0.3.24:20010,3
		//              10.0.4.24:20010,3
		//		In the event that the user specifies only the "address/hostname" and not a base_port
		//		number then the default e2e base port number is used. This is handled by the xddfunc_endtoend()
		//		subroutine in parse_func.c. Similarly, if the user also specifies the "-e2e port #" option
		//		then the parser will make that the default port number for all the entries specified to that point.
		// 	3) If no queue depth has been specified and no port counts have been specified, then the 
		//		queue depth will be set to the number of destination hostnames specified and the port count
		//		for each entry in the e2e_address_table will be set to 1.
		// 
		if (tp->target_options & TO_ENDTOEND) { 
			// Sanity checking....
			if (tp->e2ep->e2e_address_table_host_count == 0) { // This is an error...
				fprintf(xgp->errout,"%s: xdd_build_ptds_substructure: ERROR: No E2E Destination Hosts defined!\n",
					xgp->progname);
				xgp->abort = 1;
				return;
			}
			// At this point the host_count is greater than zero...
			if (tp->e2ep->e2e_address_table_port_count > 0) { 
				// When the port_count > 0 then this determines the queue depth.
				tp->queue_depth = tp->e2ep->e2e_address_table_port_count;
//TMR fprintf(stderr,"build_ptds_substructure: CASE 1: number of host entries %d, total port count %d, queue depth %d\n",tp->e2ep->e2e_address_table_host_count,tp->e2ep->e2e_address_table_port_count,tp->queue_depth);
			} else { // At this point the port_count == 0
				// Since no ports were specified, then if the queue depth is less than
				// or equal to the number of destination host addresses in the e2e_address_table
				// then we make the queue depth equal to the number of host address entries
				// and set the port count for each entry equal to 1. 
				if (tp->queue_depth <= tp->e2ep->e2e_address_table_host_count) {
//TMR fprintf(stderr,"build_ptds_substructure: CASE 2:");
					tp->queue_depth = tp->e2ep->e2e_address_table_host_count;
					tp->e2ep->e2e_address_table_port_count = tp->e2ep->e2e_address_table_host_count;
					for (entry = 0; entry < tp->e2ep->e2e_address_table_host_count; entry++) 
						tp->e2ep->e2e_address_table[entry].port_count = 1;
//TMR fprintf(stderr," number of host entries %d, total port count %d, queue depth %d\n",tp->e2ep->e2e_address_table_host_count,tp->e2ep->e2e_address_table_port_count,tp->queue_depth);
				} else {
				// Otherwise, at this point no ports were specified and the queue 
				// depth is greater than the number of destination host addresses specified.
				// Thus we use the specified queue depth to evenly distribute
				// the ports to each host entry in the address_table. For example, if the
				// queue depth is 16 and the number of host entries is 2 then each host entry
				// port count will be set to 8.
					// Make sure all the port counts are initialized to 0
//TMR fprintf(stderr,"build_ptds_substructure: CASE 3 enter: number of host entries %d, total port count %d, queue depth %d\n",tp->e2ep->e2e_address_table_host_count,tp->e2ep->e2e_address_table_port_count,tp->queue_depth);
					for (entry = 0; entry < tp->e2ep->e2e_address_table_host_count; entry++) 
						tp->e2ep->e2e_address_table[entry].port_count = 0;
					// Increment all the port counts until we run out of ports
					tp->e2ep->e2e_address_table_port_count = tp->queue_depth;
					number_of_ports = tp->e2ep->e2e_address_table_port_count;
					entry = 0;
//TMR fprintf(stderr,"build_ptds_substructure: CASE 3 starting to fill ports...: number of host entries %d, total port count %d, queue depth %d\n",tp->e2ep->e2e_address_table_host_count,tp->e2ep->e2e_address_table_port_count,tp->queue_depth);
					while (number_of_ports) {
						tp->e2ep->e2e_address_table[entry].port_count++;
						entry++;
						if (entry >= tp->e2ep->e2e_address_table_host_count)
							entry = 0;
						number_of_ports--;
					} // End of WHILE loop that sets port counts
				} // End of ELSE clause with port_count == 0 and queue_depth > host addresses
			} // End of ELSE clause with port_count == 0
		} // End of IF stmnt for TO_ENDTOEND
//TMR fprintf(stderr,"build_ptds_substructure: number of host entries %d, total port count %d, queue depth %d\n",tp->e2ep->e2e_address_table_host_count,tp->e2ep->e2e_address_table_port_count,tp->queue_depth);
//TMR for (entry = 0; entry < tp->e2ep->e2e_address_table_host_count; entry++) 
//TMR 	fprintf(stderr,"build_ptds_substructure: entry %d, hostname %s, base port %d, port count %d\n",entry,tp->e2ep->e2e_address_table[entry].hostname,tp->e2ep->e2e_address_table[entry].base_port,tp->e2ep->e2e_address_table[entry].port_count);
		// Calcualte the data transfer information - number of ops, bytes, starting offset, ...etc.
		xdd_calculate_xfer_info(tp);

		// Allocate a shiney new PTDS for each qthread 
		qp = tp;
		for (q = 0; q < tp->queue_depth; q++ ) {

                    // Increament the number of iothreads
                    planp->number_of_iothreads++;
                    
			// Get a new QThread PTDS and have the previous PTDS point to it.
			qp->next_qp = xdd_create_qthread_ptds(tp,q);
			if (qp->next_qp == NULL) break;
			qp = qp->next_qp;
			// Make sure this QThread pTDS points back to the target PTDS
			qp->target_ptds = tp;
		} // End of FOR loop that adds all PTDSs to a target PTDS linked list

	} // End of FOR loop that builds PTDS for each target
} /* End of xdd_build_ptds_substructure() */
