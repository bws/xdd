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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_init_new_ptds() - Initialize the default Per-Target-Data-Structure 
 * Note: CLO == Command Line Option
 */
void
xdd_init_new_ptds(ptds_t *p, int32_t n) {

	// Zero out the memory first
	memset((unsigned char *)p, 0, sizeof(ptds_t));

	p->next_qp = 0; // set upon creation, used when other qthreads are created
	p->my_target_number = n; // set upon creation of this PTDS
	p->my_qthread_number = 0; // Set upon creation
	p->my_pid = getpid(); // Set upon creation
	p->my_thread_id = 0; // This is set later by the actual thread 
	p->pass_complete = 0; // set upon creation
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
	p->ts_options = DEFAULT_TS_OPTIONS;
	p->target_options = DEFAULT_TARGET_OPTIONS; // Zero the target options field
	p->time_limit = DEFAULT_TIME_LIMIT;
	p->numreqs = 0; // This must init to 0
	p->report_threshold = DEFAULT_REPORT_THRESHOLD;
	p->flushwrite_current_count = 0;
	p->flushwrite = DEFAULT_FLUSHWRITE;
	p->bytes = 0; // This must init to 0
	p->start_offset = DEFAULT_STARTOFFSET;
	p->pass_offset = DEFAULT_PASSOFFSET;
	p->preallocate = DEFAULT_PREALLOCATE;
	p->queue_depth = DEFAULT_QUEUEDEPTH;
	p->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
	p->data_pattern = (unsigned char *)DEFAULT_DATA_PATTERN;
	p->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
	p->data_pattern_prefix = (unsigned char *)DEFAULT_DATA_PATTERN_PREFIX;
	p->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
	p->block_size = DEFAULT_BLOCKSIZE;
	p->mem_align = getpagesize();

	p->processor = -1;
	p->start_delay = DEFAULT_START_DELAY;
	p->start_trigger_time = 0; /* Time to trigger another target to start */
	p->stop_trigger_time = 0; /* Time to trigger another target to stop */
	p->start_trigger_op = 0; /* Operation number to trigger another target to start */
	p->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
	p->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
	p->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
	p->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
	p->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
	p->start_trigger_target = -1; /* The number of the target to send the start trigger to */
	p->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
	p->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
	p->trigger_types = 0;
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
	p->raw_sd = 0; /* raw socket descriptor */
	p->raw_hostname = NULL;  /* Reader hostname */
	p->raw_lag = DEFAULT_RAW_LAG; 
	p->raw_port = DEFAULT_RAW_PORT;
	p->raw_trigger = PTDS_RAW_MP; /* default to a message passing */
	/* Init the end-to-end fields */
	p->e2e_sd = 0; /* destination machine socket descriptor */
	p->e2e_src_hostname = NULL;  /* E2E source hostname */
	p->e2e_dest_hostname = NULL;  /* E2E destination hostname */
	p->e2e_dest_port = DEFAULT_E2E_PORT;
	p->e2e_dest_addr = 0;
	p->e2e_wait_1st_msg = 0;
	/* Init the results structure */
	memset((unsigned char *)&p->qthread_average_results,0,sizeof(results_t)); 

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
fprintf(stderr,"target_bytes_to_xfer_per_pass is %lld, iosize is %d, target_ops is %lld\n",(long long int)tp->target_bytes_to_xfer_per_pass, tp->iosize, (long long int)tp->target_ops);

 	// In the event the number of bytes to transfer is not an integer multiple of iosized requests then 
 	// the total number of ops is incremented by 1 and the last I/O op will be the something less than
	// than the normal iosize.
	if (tp->target_bytes_to_xfer_per_pass % tp->iosize) 
		tp->target_ops++;
fprintf(stderr,"target_ops is now %lld\n",(long long int)tp->target_ops);
	
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
	newqp->my_thread_number = xgp->number_of_iothreads;
	xgp->number_of_iothreads++;
	
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
xdd_build_ptds_substructure(void) {
	int32_t 	q;				// working variable 
	int32_t 	target_number;	// working variable 
	ptds_t 		*tp;			// Target PTDS pointer
	ptds_t 		*qp;			// QThread PTDS pointer

	
	/* For each target check to see if we need to add ptds structures for queue-depths greater than 1 */
	xgp->number_of_iothreads = 0;
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		// The ptdsp[] array contains pointers to the TargetThread PTDS for each Target. 
		// The TargetThread PTDS is allocated during parsing for each target that is identified.
		tp = xgp->ptdsp[target_number];

		// Get my absolute thread number 
		tp->my_thread_number = xgp->number_of_iothreads;
		xgp->number_of_iothreads++;

		// Calcualte the data transfer information - number of ops, bytes, starting offset, ...etc.
		xdd_calculate_xfer_info(tp);

		// Allocate a shiney new PTDS for each qthread 
		qp = tp;
		for (q = 0; q < tp->queue_depth; q++ ) {
			// Get a new QThread PTDS and have the previous PTDS point to it.
			qp->next_qp = xdd_create_qthread_ptds(tp,q);
			if (qp->next_qp == NULL) break;
			qp = qp->next_qp;
			// Make sure this QThread pTDS points back to the target PTDS
			qp->target_ptds = tp;
		} // End of FOR loop that adds all PTDSs to a target PTDS linked list

	} // End of FOR loop that builds PTDS for each target
} /* End of xdd_build_ptds_substructure() */
