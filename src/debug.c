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
 * Note: CLO == Command Line Option
 */
void
xdd_show_ptds(ptds_t *p) {
		fprintf(stderr,"********* Start of PTDS for target my_target_number=%d **********\n",p->my_target_number);
		//p->my_qthread_number = 0;
		fprintf(stderr,"mypid=%d\n",p->mypid);
		//p->mythreadid = 0; // This is set later by the actual thread 
		//p->thread_complete = 0; // set upon creation
		//p->nextp = 0; // set upon creation, used when other qthreads are created
		//p->pm1 = 0; // set upon creation
		//p->rwbuf = 0; // set during rwbuf allocation
		//p->rwbuf_shmid = -1; // set upon creation of a shared memory segment
		//p->rwbuf_save = 0; // used by the rwbuf allocation routine
		//p->targetdir = DEFAULT_TARGETDIR; // can be changed by CLO
		fprintf(stderr,"target=%s\n",p->target);
		//sprintf(p->targetext,"%08d",1);  // can be changed by CLO
		fprintf(stderr,"reqsize=%d\n",p->reqsize); 
		//p->throttle = DEFAULT_THROTTLE;
		//p->throttle_variance = DEFAULT_VARIANCE;
		//p->throttle_type = PTDS_THROTTLE_BW;
		//fprintf(stderr,"ts_options=0x%16x\n",p->ts_options);
		//fprintf(stderr,"target_options=0x%16x\n",p->target_options);
		//p->time_limit = DEFAULT_TIME_LIMIT;
		fprintf(stderr,"numreqs=%lld\n",(long long)p->numreqs);
		fprintf(stderr,"flushwrite=%lld\n",(long long)p->flushwrite);
		fprintf(stderr,"bytes=%lld\n",(long long)p->bytes); 
		//p->report_threshold = DEFAULT_REPORT_THRESHOLD;
		//p->start_offset = DEFAULT_STARTOFFSET;
		//p->pass_offset = DEFAULT_PASSOFFSET;
		//p->preallocate = DEFAULT_PREALLOCATE;
		//p->queue_depth = DEFAULT_QUEUEDEPTH;
		//p->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
		//p->data_pattern = DEFAULT_DATA_PATTERN;
		//p->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
		//p->data_pattern_prefix = DEFAULT_DATA_PATTERN_PREFIX;
		//p->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
		fprintf(stderr,"block_size=%d\n",p->block_size);
		//p->mem_align = getpagesize();
        //p->my_current_elapsed_time = 0;
        //p->my_current_end_time = 0;
        //p->my_current_start_time = 0;
        //p->my_end_time = 0;
        //p->my_start_time = 0;
		//p->processor = -1;
		//p->start_delay = DEFAULT_START_DELAY;
		//p->start_trigger_time = 0; /* Time to trigger another target to start */
		//p->stop_trigger_time = 0; /* Time to trigger another target to stop */
		//p->start_trigger_op = 0; /* Operation number to trigger another target to start */
		//p->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		//p->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		//p->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		//p->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		//p->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		//p->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		//p->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		//p->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
		//p->trigger_types = 0;
		//p->ls_master = -1; /* The default master number  */
		//p->ls_slave  = -1; /* The default slave number */
		//p->ls_interval_type  = 0; /* The default interval type */
		//p->ls_interval_value  = 0; /* The default interval value  */
		//p->ls_interval_units  = "not defined"; /* The default interval units  */
		//p->ls_task_type  = 0; /* The default task type */
		//p->ls_task_value  = 0; /* The default task value  */
		//p->ls_task_units  = "not defined"; /* The default task units  */
		//p->ls_task_counter = 0; /* the default task counter */
		/* Init the seeklist header fields */
		//p->seekhdr.seek_options = 0;
		//p->seekhdr.seek_range = DEFAULT_RANGE;
		//p->seekhdr.seek_seed = DEFAULT_SEED;
		//p->seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
		//p->seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
		//p->seekhdr.seek_num_rw_ops = DEFAULT_NUMREQS;
		//p->seekhdr.seek_total_ops = DEFAULT_NUMREQS;
		//p->seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
		//p->seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
		//p->seekhdr.seek_savefile = NULL; /* file to save seek locations into */
		//p->seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
		//p->seekhdr.seek_pattern = "sequential";
		/* Init the read-after-write fields */
		//p->raw_sd = 0; /* raw socket descriptor */
		//p->raw_hostname = NULL;  /* Reader hostname */
		//p->raw_lag = DEFAULT_RAW_LAG; 
		//p->raw_port = DEFAULT_RAW_PORT;
		//p->raw_trigger = PTDS_RAW_MP; /* default to a message passing */
		
		fprintf(stderr,"+++++++++++++ End of PTDS for target my_target_number=%d +++++++++++++\n",p->my_target_number);
} /* end of xdd_show_ptds() */ 
 
 
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
