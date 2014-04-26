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
 * This file contains the subroutines that perform various functions 
 * with respect to the Target_Data and the Target_Data substructure.
 */
#include "xint.h"


/*----------------------------------------------------------------------------*/
/* xdd_init_new_target_data() - Initialize the default Per-Target-Data-Structure 
 * Note: CLO == Command Line Option
 */
void
xdd_init_new_target_data(target_data_t *tdp, int32_t n) {


	tdp->td_next_wdp = 0; // set upon creation, used when other Worker Threads are created
	tdp->td_target_number = n; // set upon creation of this Target_Data
	tdp->td_pid = getpid(); // Set upon creation
	tdp->td_thread_id = 0; // This is set later by the actual thread 
	tdp->td_target_directory = DEFAULT_TARGETDIR; // can be changed by CLO
	tdp->td_target_basename = DEFAULT_TARGET;  // can be changed by CLO
	sprintf(tdp->td_target_extension,"%08d",1);  // can be changed by CLO
	tdp->td_reqsize = DEFAULT_REQSIZE;  // can be changed by CLO
	tdp->td_ts_table.ts_options = DEFAULT_TS_OPTIONS;
	tdp->td_target_options = DEFAULT_TARGET_OPTIONS; // Zero the target options field
	tdp->td_time_limit = DEFAULT_TIME_LIMIT;
	if (tdp->td_trigp) tdp->td_trigp->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
	tdp->td_numreqs = 0; // This must init to 0
	tdp->td_report_threshold = DEFAULT_REPORT_THRESHOLD;
	tdp->td_flushwrite_current_count = 0;
	tdp->td_flushwrite = DEFAULT_FLUSHWRITE;
	tdp->td_bytes = 0; // This must init to 0
	tdp->td_start_offset = DEFAULT_STARTOFFSET;
	tdp->td_pass_offset = DEFAULT_PASSOFFSET;
	tdp->td_preallocate = DEFAULT_PREALLOCATE;
	tdp->td_pretruncate= DEFAULT_PRETRUNCATE;
	tdp->td_queue_depth = DEFAULT_QUEUEDEPTH;
	tdp->td_dpp->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
	tdp->td_dpp->data_pattern = (unsigned char *)DEFAULT_DATA_PATTERN;
	tdp->td_dpp->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
	tdp->td_dpp->data_pattern_prefix = (unsigned char *)DEFAULT_DATA_PATTERN_PREFIX;
	tdp->td_dpp->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
	tdp->td_block_size = DEFAULT_BLOCKSIZE;
	tdp->td_mem_align = getpagesize();

	tdp->td_processor = -1;
	tdp->td_start_delay = DEFAULT_START_DELAY;
	/* Init the Trigger Structure members if there is a trigger struct */
	if (tdp->td_trigp) {
		tdp->td_trigp->start_trigger_time = 0; /* Time to trigger another target to start */
		tdp->td_trigp->stop_trigger_time = 0; /* Time to trigger another target to stop */
		tdp->td_trigp->start_trigger_op = 0; /* Operation number to trigger another target to start */
		tdp->td_trigp->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		tdp->td_trigp->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		tdp->td_trigp->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		tdp->td_trigp->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		tdp->td_trigp->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		tdp->td_trigp->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		tdp->td_trigp->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		tdp->td_trigp->trigger_types = 0;
	}
	/* Init the seeklist header fields */
	tdp->td_seekhdr.seek_options = 0;
	tdp->td_seekhdr.seek_range = DEFAULT_RANGE;
	tdp->td_seekhdr.seek_seed = DEFAULT_SEED;
	tdp->td_seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
	tdp->td_seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
	tdp->td_seekhdr.seek_num_rw_ops = 0;
	tdp->td_seekhdr.seek_total_ops = 0;
	tdp->td_seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
	tdp->td_seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
	tdp->td_seekhdr.seek_savefile = NULL; /* file to save seek locations into */
	tdp->td_seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
	tdp->td_seekhdr.seek_pattern = "sequential";
	/* Init the read-after-write fields */
	if (tdp->td_rawp) {
		tdp->td_rawp->raw_sd = 0; /* raw socket descriptor */
		tdp->td_rawp->raw_hostname = NULL;  /* Reader hostname */
		tdp->td_rawp->raw_lag = DEFAULT_RAW_LAG; 
		tdp->td_rawp->raw_port = DEFAULT_RAW_PORT;
		tdp->td_rawp->raw_trigger = RAW_MP; /* default to a message passing */
	}
	/* Init the end-to-end fields */
	if (tdp->td_e2ep) {
		tdp->td_e2ep->e2e_sd = 0; /* destination machine socket descriptor */
		tdp->td_e2ep->e2e_src_hostname = NULL;  /* E2E source hostname */
		tdp->td_e2ep->e2e_dest_hostname = NULL;  /* E2E destination hostname */
		tdp->td_e2ep->e2e_dest_port = DEFAULT_E2E_PORT;
		tdp->td_e2ep->e2e_address_table_host_count = 0;
		tdp->td_e2ep->e2e_address_table_port_count = 0;
		tdp->td_e2ep->e2e_dest_addr = 0;
		tdp->td_e2ep->e2e_wait_1st_msg = 0;
		tdp->td_e2ep->e2e_address_table_next_entry=0;
	}

	tdp->xni_ibdevice = DEFAULT_IB_DEVICE;  /* can be changed by '-ibdevice' CLO */

	tdp->xni_tcp_congestion = XNI_TCP_DEFAULT_CONGESTION;  /* can be changed by '-congestion' CLO */

	sprintf(tdp->td_occupant_name,"TARGET%04d",tdp->td_target_number);
	xdd_init_barrier_occupant(&tdp->td_occupant, tdp->td_occupant_name, XDD_OCCUPANT_TYPE_TARGET, (void *)tdp);

	// the default magic cookie has all bits set
	memset(tdp->td_magic_cookie, 0xFF, sizeof(tdp->td_magic_cookie));

} /* end of xdd_init_new_target_data() */

/*----------------------------------------------------------------------------*/
/* xdd_build_target_data_substructure_calculate_xfer_info() - Will calculate the number of data
 * transfers to perform as well as the total number of bytes for the specified
 * target. 
 * This subroutine is only called by xdd_build_target_data_substructure() and is
 * given a pointer to a Target TARGET_DATA to operate on.
 */
void
xdd_calculate_xfer_info(target_data_t *tdp) {
	// The following calculates the number of I/O requests (numreqs) to issue to a "target"
	// This value represents the total number of I/O operations that will be performed on this target.
	/* Now lets get down to business... */
	tdp->td_xfer_size = tdp->td_reqsize * tdp->td_block_size;
	if (tdp->td_xfer_size == 0) {
		fprintf(xgp->errout,"%s: xdd_calculate_xfer_info: ALERT! iothread for target %d has an iosize of 0, reqsize of %d, blocksize of %d\n",
			xgp->progname, tdp->td_target_number, tdp->td_reqsize, tdp->td_block_size);
		fflush(xgp->errout);
		tdp->td_target_bytes_to_xfer_per_pass = 0;
		return;
	}
	if (tdp->td_numreqs) 
		tdp->td_target_bytes_to_xfer_per_pass = (uint64_t)(tdp->td_numreqs * tdp->td_xfer_size);
	else if (tdp->td_bytes)
		tdp->td_target_bytes_to_xfer_per_pass = (uint64_t)tdp->td_bytes;
	else { // Yikes - something was not specified
		fprintf(xgp->errout,"%s: xdd_calculate_xfer_info: ERROR! iothread for target %d has numreqs of %lld, bytes of %lld - one of these must be specified\n",
			xgp->progname, tdp->td_target_number, (long long)tdp->td_numreqs, (long long)tdp->td_bytes);
		fflush(xgp->errout);
		tdp->td_target_bytes_to_xfer_per_pass = 0;
		return;
	}

	// Check to see if this is a restart operation
	// If a restart copy resume operation has been requested then a restart structure will
	// have been allocated and p->restartp should point to that structure. 
	if (tdp->td_restartp) {
		// We have a good restart pointer 
		if (tdp->td_restartp->flags & RESTART_FLAG_RESUME_COPY) {
			// Change the startoffset to reflect the shift in starting point
			tdp->td_start_offset = tdp->td_restartp->byte_offset / tdp->td_block_size;
			// Change the bytes-to-be-transferred to reflect the shift in starting point
			// Since Worker Threads transfer 1/qd*totalbytes we need to recalculate this carefully
			tdp->td_target_bytes_to_xfer_per_pass -= tdp->td_restartp->byte_offset;
		}
	}

	// This calculates the number of iosize (or smaller) operations that need to be performed. 
	tdp->td_target_ops = tdp->td_target_bytes_to_xfer_per_pass / tdp->td_xfer_size;

 	// In the event the number of bytes to transfer is not an integer multiple of iosized requests then 
 	// the total number of ops is incremented by 1 and the last I/O op will be the something less than
	// than the normal iosize.
	if (tdp->td_target_bytes_to_xfer_per_pass % tdp->td_xfer_size) 
		tdp->td_target_ops++;
	
} // End of xdd_calculate_xfer_info()
	
/*----------------------------------------------------------------------------*/
/* xdd_create_worker_data() - will allocate and initialize a new
 * Worker_Data structure for a worker based on the Target Data Strucute that is passed in. 
 * This subroutine is also passed the queue number which is a number from 0 to
 * queue depth minus 1.
 * This subroutine is only called by xdd_build_target_data_substructure as part of the
 * loop that creates all the Worker Threads for a target. 
 * 
 *  
 */
worker_data_t *
xdd_create_worker_data(target_data_t *tdp, int32_t q) {
	worker_data_t	*wdp;		// The new Worker Data Struct 


	// This is the upperlevel Worker_Data and it's next_qp needs to point to this new Worker_Data
	wdp = malloc(sizeof(worker_data_t));
	if (wdp == NULL) {
		fprintf(xgp->errout,"%s: error getting memory for Worker_Data Structure for target %d - worker number %d\n",
				xgp->progname, tdp->td_target_number, q);
		return(NULL);
	}
	memset((unsigned char *)wdp, 0, sizeof(struct xint_worker_data));
	wdp->wd_tdp = tdp;
	wdp->wd_next_wdp = NULL; 
	wdp->wd_worker_number = q;
	wdp->wd_sgiop = NULL;
        
	if (tdp->td_target_options & TO_SGIO) {
#if HAVE_SCSI_SG_H
		wdp->wd_sgiop = xdd_get_sgiop(wdp);
#else
		fprintf(xgp->errout, "%s: ERROR: Cannot use SCSI Generic I/O\n", xgp->progname);
		return(NULL);
	}
#endif
        
	// Allocate and initialize the End-to-End structure if needed
	if (tdp->td_target_options & TO_ENDTOEND) {
	   	wdp->wd_e2ep = xdd_get_e2ep();
		if (NULL == wdp->wd_e2ep) {
	   		fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for WORKER_DATA END TO END Data Structure for worker %d\n",
	    		xgp->progname, (int)sizeof(xint_data_pattern_t), q);
	   		return(NULL);
		}
		if (tdp->td_e2ep)
			memcpy(wdp->wd_e2ep, tdp->td_e2ep, sizeof(xint_e2e_t));
	}

	sprintf(wdp->wd_occupant_name,"TARGET%04d_WORKER%04d",tdp->td_target_number,wdp->wd_worker_number); 
	xdd_init_barrier_occupant(&wdp->wd_occupant, wdp->wd_occupant_name, XDD_OCCUPANT_TYPE_WORKER_THREAD, (void *)wdp);
	
	return(wdp);

} // End of xdd_create_worker_data()
	
/*----------------------------------------------------------------------------*/
/* xdd_build_target_data_substructure() - given that all the targets have been
 * defined by the command-line input, create any new TARGET_DATA's for targets that
 * have queue depths greater than 1.
 * This subroutine is only called by xdd_parse(). 
 *
 * For any given target, if the queue depth is greater than 1 then we create 
 * a WORKER_DATA structure for each Worker.
 *
 * The number of Worker Threads is equal to the qdepth for that target.
 * The "td_next_wdp" member of the Target_Data points to Worker Thread 0. 
 * The "wd_next_wdp" member in the Worker_Data 0 Worker Thread 1, and so on. 
 * The last Worker Thread "wd_next_wdp" member is 0.
 * 
 *     Target0          Target1          Target2 .....    TargetN
 *   TargetThread     TargetThread     TargetThread     TargetThread
 *         |                |                |                |
 *         V                V                V                V
 *   Worker Thread0   Worker Thread0   Worker Thread0   Worker Thread0
 *         |                |                |                |
 *         V                V                V                V
 *   Worker Thread1   Worker Thread1        NULL        Worker Thread1
 *         |                |                                 |
 *         V                V                                 V
 *   Worker Thread2        NULL                         Worker Thread2
 *         |                                                  |
 *         V                                                  V
 *        NULL                                          Worker Thread3
 *                                                            |
 *                                                            V
 *                                                           NULL  
 * The above example shows that there are N targets, each target having a Target Thread that
 * points to at least one Worker Thread "0". For queue depths greater than 1, the additional 
 * Worker Threads are linked in a chain as shown. For example, 
 *     Target0 has a -queuedepth of 3 and thus has 3 Worker Threads (0-2)
 *     Target 1 has a -queuedepth of 2 and thus 2 Worker Threads (0-1)
 *     Target3 has a queuedepth of 1 (the default) and hence has only one Worker Thread (0).
 *     Target N has a queuedepth of 4, possibly more.  
 */
void
xdd_build_target_data_substructure(xdd_plan_t* planp) {
	int32_t 		q;				// working variable 
	int32_t 		target_number;	// working variable 
	target_data_t	*tdp;			// Target Data pointer
	worker_data_t	*wdp;			// Worker Data pointer
	worker_data_t	*prev_wdp;		// Previous Worker Data pointer

	
	/* For each target check to see if we need to add worker_data structures for queue-depths greater than 1 */
	planp->number_of_iothreads = 0;

	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		// The target_datap[] array contains pointers to the Target_Data Structures for each Target. 
		// The Target_Data Structure is allocated during parsing for each target that is identified.
		tdp = planp->target_datap[target_number];

		planp->number_of_iothreads++;

		// Handle the End-to-End cases
		if (tdp->td_target_options & TO_ENDTOEND) { 
			xdd_build_target_data_substructure_e2e(planp, tdp);
		} 
		// Calcualte the data transfer information - number of ops, bytes, starting offset, ...etc.
		xdd_calculate_xfer_info(tdp);

		// Allocate a shiney new Worker Data Struct for each Worker Thread 
		prev_wdp = NULL;
		for (q = 0; q < tdp->td_queue_depth; q++ ) {
			// Increament the number of iothreads
            planp->number_of_iothreads++;
                    
			// Get a new Worker Data and have the previous Worker Data point to it.
			wdp = xdd_create_worker_data(tdp,q);
			if (q == 0) // The first worker_data struct is anchored in the target_data struct
				 tdp->td_next_wdp = wdp;
			else prev_wdp->wd_next_wdp = wdp; // Chain this wdp off the previous wdp
			prev_wdp = wdp;
		} // End of FOR loop that adds all Worker_Datas to a Target_Data linked list

	} // End of FOR loop that builds Worker_Datas for each Target_Data
} /* End of xdd_build_target_data_substructure() */

/*----------------------------------------------------------------------------*/
/* xdd_build_target_data_substructure_e2e()									  */
// If this is an end-to-end operation, figure out the number of Worker Threads 
// which is based on the number of addresses:ports combinations specified
// There are XXXX different scenarios:
//    1) The address:base_port,port_count entries have been spcified in full.  
//		 In this case the number of Worker Threads will be set to meet the cumulative
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
void
xdd_build_target_data_substructure_e2e(xdd_plan_t* planp, target_data_t *tdp) {
	int32_t			entry;			// Entry number
	int32_t			number_of_ports;// Number of ports to distribute - working variable
		
	// Sanity checking....
	if (tdp->td_e2ep->e2e_address_table_host_count == 0) { // This is an error...
		fprintf(xgp->errout,"%s: xdd_build_target_data_substructure: ERROR: No E2E Destination Hosts defined!\n",
			xgp->progname);
		xgp->abort = 1;
		return;
	}
	// At this point the host_count is greater than zero...
	if (tdp->td_e2ep->e2e_address_table_port_count > 0) { 
		// When the port_count > 0 then this determines the queue depth.
		tdp->td_queue_depth = tdp->td_e2ep->e2e_address_table_port_count;
	} else { // At this point the port_count == 0
		// Since no ports were specified, then if the queue depth is less than
		// or equal to the number of destination host addresses in the e2e_address_table
		// then we make the queue depth equal to the number of host address entries
		// and set the port count for each entry equal to 1. 
		if (tdp->td_queue_depth <= tdp->td_e2ep->e2e_address_table_host_count) {
			tdp->td_queue_depth = tdp->td_e2ep->e2e_address_table_host_count;
			tdp->td_e2ep->e2e_address_table_port_count = tdp->td_e2ep->e2e_address_table_host_count;
			for (entry = 0; entry < tdp->td_e2ep->e2e_address_table_host_count; entry++) 
				tdp->td_e2ep->e2e_address_table[entry].port_count = 1;
		} else {
		// Otherwise, at this point no ports were specified and the queue 
		// depth is greater than the number of destination host addresses specified.
		// Thus we use the specified queue depth to evenly distribute
		// the ports to each host entry in the address_table. For example, if the
		// queue depth is 16 and the number of host entries is 2 then each host entry
		// port count will be set to 8.
		// Make sure all the port counts are initialized to 0
			for (entry = 0; entry < tdp->td_e2ep->e2e_address_table_host_count; entry++) 
				tdp->td_e2ep->e2e_address_table[entry].port_count = 0;
			// Increment all the port counts until we run out of ports
			tdp->td_e2ep->e2e_address_table_port_count = tdp->td_queue_depth;
			number_of_ports = tdp->td_e2ep->e2e_address_table_port_count;
			entry = 0;
			while (number_of_ports) {
				tdp->td_e2ep->e2e_address_table[entry].port_count++;
				entry++;
				if (entry >= tdp->td_e2ep->e2e_address_table_host_count)
					entry = 0;
				number_of_ports--;
			} // End of WHILE loop that sets port counts
		} // End of ELSE clause with port_count == 0 and queue_depth > host addresses
	} // End of ELSE clause with port_count == 0
} // End of xdd_build_target_data_substructure_e2e()

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
