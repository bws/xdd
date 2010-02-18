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

		p->my_target_number = n; // set upon creation of this PTDS
		p->my_qthread_number = 0; // Set upon creation
		p->mypid = getpid(); // Set upon creation
		p->mythreadid = 0; // This is set later by the actual thread 
		p->pass_complete = 0; // set upon creation
		p->run_complete = 0; // set upon creation
		p->nextp = 0; // set upon creation, used when other qthreads are created
		p->pm1 = 0; // set upon creation
		p->rwbuf = 0; // set during rwbuf allocation
		p->rwbuf_shmid = -1; // set upon creation of a shared memory segment
		p->rwbuf_save = 0; // used by the rwbuf allocation routine
		p->targetdir = DEFAULT_TARGETDIR; // can be changed by CLO
		p->target = DEFAULT_TARGET;  // can be changed by CLO
		sprintf(p->targetext,"%08d",1);  // can be changed by CLO
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
		p->ls_master = -1; /* The default master number  */
		p->ls_slave  = -1; /* The default slave number */
		p->ls_interval_type  = 0; /* The default interval type */
		p->ls_interval_value  = 0; /* The default interval value  */
		p->ls_interval_units  = "not defined"; /* The default interval units  */
		p->ls_task_type  = 0; /* The default task type */
		p->ls_task_value  = 0; /* The default task value  */
		p->ls_task_units  = "not defined"; /* The default task units  */
		p->ls_task_counter = 0; /* the default task counter */
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
		p->e2e_useUDP = 0;
		/* Init the results structure */
		memset((unsigned char *)&p->qthread_average_results,0,sizeof(results_t)); 
} /* end of xdd_init_new_ptds() */

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
 * The main ptds for the target is QThread 0. The "nextp" member of the ptds points to the QThread 1. 
 * The "nextp" member in the QThread 1 ptds points to QThread 2, and so on. The last QThread "nextp" member is 0.
 * 
 *  Target0   Target1    Target2 ..... TargetN
 *  (QThread0)   (Qthread0)   (Qthread0)  (Qthread0)
 *   |            |            |           |
 *   V            V            V           V
 *   QThread1     QThread1     NULL        QThread1
 *   |            |                        |
 *   V            V                        V
 *   QThread2     QThread2                 QThread2
 *   |            |                        |
 *   V            V                        V
 *   QThread3     NULL                     QThread3
 *   |                                     |
 *   V                                     V
 *  NULL                                   QThread4
 *                                         |
 *                                         V
 *                                         NULL  
 * The above example shows that Target0 has a -queuedepth of 4 and thus
 * has 4 QThread (0-3). Similarly, Target 1 has a -queuedepth of 3 (QThreads 0-2), Target3 has a queuedepth
 * of 1 (the default) and hence has no QThreads other than 0. Target N has a queuedepth of 5, possibly more.  
 *  
 */
void
xdd_build_ptds_substructure(void)
{
	int32_t 	q;		/* working variable */
	int32_t 	target_number;	/* working variable */
	ptds_t 		*p;
	ptds_t 		*psave;

	
	/* For each target check to see if we need to add ptds structures for queue-depths greater than 1 */
	xgp->number_of_iothreads = 0;
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number];
		// The following calculates the number of I/O requests (numreqs) to issue to a "target"
		// This value represents the combined number of ops from each qthread if queue_depth > 1.
		/* Now lets get down to business... */
		p->iosize = p->reqsize*p->block_size;
		if (p->iosize == 0) {
			fprintf(xgp->errout,"%s: io_thread_init: ALERT! iothread for target %d queue %d has an iosize of 0, reqsize of %d, blocksize of %d\n",
				xgp->progname, p->my_target_number, p->my_qthread_number, p->reqsize, p->block_size);
			fflush(xgp->errout);
		}
		if (p->numreqs) 
			p->target_bytes_to_xfer_per_pass = (uint64_t)(p->numreqs * p->iosize);
		else if (p->bytes)
			p->target_bytes_to_xfer_per_pass = (uint64_t)p->bytes;
		else { // Yikes - something was not specified
			fprintf(xgp->errout,"%s: io_thread_init: ERROR! iothread for target %d queue %d has numreqs of %lld, bytes of %lld - one of these must be specified\n",
				xgp->progname, p->my_target_number, p->my_qthread_number, (long long)p->numreqs, (long long)p->bytes);
			fflush(xgp->errout);
			exit(0);
		}

		// Check to see if this is a restart operation
		if (p->restartp) {
			// We have a good restart pointer 
			if (p->restartp->flags & RESTART_FLAG_RESUME_COPY) {
				// Change the startoffset to reflect the shift in starting point
				p->start_offset = p->restartp->byte_offset / p->block_size;
				// Change the bytes-to-be-transferred to reflect the shift in starting point
				// Since qthreads transfer 1/qd*totalbytes we need to recalculate this carefully
				p->target_bytes_to_xfer_per_pass -= p->restartp->byte_offset;
			}
		}

		// At this point it is possible that the number of bytes to transfer is an odd number.
		// This means that one of the qthreads will need to do a "short" I/O operation - something less than the iosize 
		// and possibly not an even number.
		// The p->target_ops includes the last short op if there is one.
		// The "p->last_iosize" will contain the number of bytes for the last I/O *ONLY* if the last
		// I/O operation is not equal to p->iosize.
		//

	 	//
		// This calculates the number of iosize (or smaller) operations that need to be performed. 
		p->target_ops = p->target_bytes_to_xfer_per_pass / p->iosize;

	 	// In the event the number of bytes to transfer is not an integer number of iosize requests then 
	 	// the total number of ops is incremented by 1 and the last I/O op will be the residual.
		if (p->target_bytes_to_xfer_per_pass % p->iosize) {
			p->target_ops++;
			p->last_iosize = p->target_bytes_to_xfer_per_pass % p->iosize;
		}

		// Adjust the number of ops based on the queue_depth (aka number of qthreads)
		// This is the situation where the number of ops is 3 but the qdepth is 4.
		// In this case the qdepth is reset to the number of ops (3 in this example) and the number
		// of ops for each qthread is set to 1.
		if ((p->target_ops > 0) && (p->target_ops < p->queue_depth)) { 
			fprintf(xgp->errout,"%s: Target %d Number of requests is too small to use with a queuedepth of %d\n",
				xgp->progname, 
				target_number, 
				p->queue_depth);
			fprintf(xgp->errout,"%s: queuedepth will be reset to be equal to the number of requests: %lld\n",
				xgp->progname, 
				(long long)p->target_ops);

			p->queue_depth = p->target_ops;
			p->qthread_ops = 1;
			p->residual_ops = 0;
		} else {
			// Otherwise, the number of ops for each target is simply target_ops / queue_depth
			// with the exception for some qthread targets getting one extra op if,for example,
			// the total number of ops is not an interger multiple of the qdepth like total ops being
			// 10 with a qdepth of 4 - targets 0 and 1 will do 3 ops each and targets 2 & 3 will only
			// do 2 ops each.
			// The "residual_ops" are the number of left over ops (i.e. 2 in the previous example)
			p->residual_ops = p->target_ops % p->queue_depth;
			p->qthread_ops = p->target_ops / p->queue_depth;
		}
		// This is where the qthread PTDSs are added to a target PTDS if the queue_depth is greater than 1
		p->mythreadnum = xgp->number_of_iothreads;
		xgp->number_of_iothreads++;
		psave = p;
		if (p->queue_depth <= 1) { // For a single qthread (qthread 0) set the correct bytes to xfer and qthread ops
			p->qthread_bytes_to_xfer_per_pass = p->target_bytes_to_xfer_per_pass;
			p->qthread_ops = p->target_ops;
		} else { // For qdepths greater than 1 we need to set up the qthread ptds structs
			for (q = 1; q < p->queue_depth; q++ ) {
				p->seekhdr.seek_pattern = "queued_interleaved";
				p->seekhdr.seek_interleave = p->queue_depth;
				p->nextp = malloc(sizeof(struct ptds));
				if (p->nextp == NULL) {
					fprintf(xgp->errout,"%s: error getting memory for ptds for target %d queue number %d\n",
						xgp->progname, p->my_target_number, q);
				}

				// Copy the existing PTDS into the new qthread PTDS 
				memcpy(p->nextp, psave, sizeof(struct ptds));
				// The qthread PTDSs are a linked list so update the links
				p->nextp->nextp = 0; 
				p->nextp->my_qthread_number = q;
				p->nextp->mythreadnum = xgp->number_of_iothreads;
				if ((p->residual_ops) && (q < p->residual_ops)) { 
					// This means that the number of requests 
					// is not an even multiple of the queue depth. 
					// Therefore some qthreads will have more requests 
					// than others to account for the difference.
					p->nextp->qthread_ops++; 
				}
				p->nextp->restartp = NULL; // Zero this because QThread0 should be the only QThread with a restart struct
				p->nextp->qthread_bytes_to_xfer_per_pass = p->nextp->qthread_ops * p->nextp->iosize;
				xgp->number_of_iothreads++;
				p = p->nextp;
			} // End of FOR loop that adds all PTDSs to a target PTDS linked list

			// This is for qthread0 which was not done in the for loop above
			p = xgp->ptdsp[target_number]; // reset p-> to the primary target PTDS
			if (p->residual_ops) { 
				// This means that the number of requests is not an even multiple 
				// of the queue depth. Therefore some qthreads will have more 
				// requests than others to account for the difference.
				p->qthread_ops++; 
			}
			p->qthread_bytes_to_xfer_per_pass = p->qthread_ops * p->iosize;
		} // End of IF clause that adds PTDSs to a target PTDS
		if ((p->last_iosize) && (p->queue_depth > 1)) { // This means that the qthread that issues the last I/O must use this request size which is 0 < last_iosize < iosize
			if (p->residual_ops == 0) // This means that the q number to issue the last I/O is the last qthread ->> qdepth-1  - otherwise it is residual_ops-1
				q = p->queue_depth - 1;
			else 	q = p->residual_ops - 1; 
			psave = p;
			if (q) { // walk down the chain to get the correct ptds pointer
				while(q) {
					p = p->nextp;
					q--;
				}
			}
			// At this point p points to the qthread that will have to issue the weird iosize
			// As such, we need to adjust its bytes_to_xfer_per_pass to be slightly less than it is
			p->qthread_bytes_to_xfer_per_pass = (p->qthread_bytes_to_xfer_per_pass - p->iosize + p->last_iosize);
		} // End of figuring out which qthread issues the last weird-sized I/O
			
	} // End of FOR loop that builds PTDS for each target
} /* End of xdd_build_ptds_substructure() */

