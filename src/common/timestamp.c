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
 * This file contains the subroutines necessary to perform the time stamping
 * operations during an xdd run.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_ts_overhead() - determine time stamp overhead
 */
void
xdd_ts_overhead(struct xdd_tthdr *ttp) { 
	int32_t  i;
	nclk_t  tv[101];
	ttp->timer_oh = 0;
	for (i = 0; i < 101; i++) {
		nclk_now(&tv[i]);
	}
	for (i = 0; i < 100; i++) 
		ttp->timer_oh += (tv[i+1]-tv[i]);
	ttp->timer_oh /= 100;
	if (xgp->global_options & GO_TIMER_INFO) { /* only display this information if requested */
		fprintf(xgp->errout,"Timer overhead is %lld nanoseconds\n",
		(long long)(ttp->timer_oh/1000));
		fflush(xgp->errout);
	}
} /* End of xdd_ts_overhead() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_setup() - set up the time stamping
 * It is important to distinguish between a normal run and one that involves 
 * the DESKEW option. If the DESKEW option is set then we need to make sure
 * timestamping is turned on and that the size of the time stamp table is 
 * adequate for this run. If time stamping is already turned on for this target
 * then we need to make sure that the size of the time stamp table is adequate
 * and enlarge it if necessary. 
 * If the DESKEW option is not set then just do the normal setup.
 * 
 */
void
xdd_ts_setup(target_data_t *tdp) {
	xint_timestamp_t	*tsp;
	nclk_t		cycleval; /* resolution of the clock in nanoseconds per ticl */
	time_t 		t;  /* Time */
	int64_t 	tt_entries; /* number of entries inthe time stamp table */
	int32_t 	tt_bytes; /* size of time stamp table in bytes */
	int32_t		ts_filename_size; // Number of bytes in the size of the file name



	tsp = tdp->td_tsp;
	/* check to make sure we really need to do this */
	if (!(tsp->ts_options & TS_ON) && !(xgp->global_options & GO_DESKEW))
		return;

	/* If DESKEW is TRUE but the TS option was not requested, then do a DESKEW ts setup */
	if ((xgp->global_options & GO_DESKEW) && !(tsp->ts_options & TS_ON)) {
		tsp->ts_options |= (TS_ON | TS_ALL | TS_ONESHOT | TS_SUPPRESS_OUTPUT);
		tsp->ts_size = (tdp->td_planp->passes + 1) * tdp->td_queue_depth;
	} else tsp->ts_size = (tdp->td_planp->passes * tdp->td_target_ops) + tdp->td_queue_depth;
	if (tsp->ts_options & (TS_TRIGTIME | TS_TRIGOP)) 
		tsp->ts_options &= ~TS_ALL; /* turn off the "time stamp all operations" flag if a trigger was requested */
	if (tsp->ts_options & TS_TRIGTIME) { /* adjust the trigger time to an actual local time */
		tsp->ts_trigtime *= BILLION;
		tsp->ts_trigtime += tdp->td_planp->ActualLocalStartTime;
	}

	/* Calculate size of the time stamp table and malloc it */
	tt_entries = tsp->ts_size; 
	if (tt_entries < ((tdp->td_planp->passes * tdp->td_target_ops) + tdp->td_queue_depth)) { /* Display a NOTICE message if ts_wrap or ts_oneshot have not been specified to compensate for a short time stamp buffer */
		if (((tsp->ts_options & TS_WRAP) == 0) &&
			((tsp->ts_options & TS_ONESHOT) == 0) &&
			(!(xgp->global_options & GO_DESKEW))) {
			fprintf(xgp->errout,"%s: ***NOTICE*** The size specified for timestamp table for target %d is too small - enabling time stamp wrapping to compensate\n",xgp->progname,tdp->td_target_number);
			fflush(xgp->errout);
			tsp->ts_options |= TS_WRAP;
		}
	}
	/* calculate the total size in bytes of the time stamp table */
	tt_bytes = (int)((sizeof(struct xdd_tthdr)) + (tt_entries * sizeof(struct tte)));
#if (LINUX || SOLARIS || AIX || DARWIN)
	tdp->td_ttp = (struct xdd_tthdr *)valloc(tt_bytes);
#else
	tdp->td_ttp = (struct xdd_tthdr *)malloc(tt_bytes);
#endif
	if (tdp->td_ttp == 0) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp table\n",
			xgp->progname,tdp->td_target_number, tt_bytes);
		fflush(xgp->errout);
		perror("Reason");
		tsp->ts_options &= ~TS_ON;
		return;
	}
	/* Lock the time stamp table in memory */
	xdd_lock_memory((unsigned char *)tdp->td_ttp, tt_bytes, "TIMESTAMP");
	/* clear everything out of the trace table */
	memset(tdp->td_ttp,0, tt_bytes);
	/* get access to the high-res clock*/
	nclk_initialize(&cycleval);
	if (cycleval == NCLK_BAD) {
		fprintf(xgp->errout,"%s: Could not initialize high-resolution clock\n",
			xgp->progname);
		fflush(xgp->errout);
		tsp->ts_options &= ~TS_ON;
	}
	xdd_ts_overhead(tdp->td_ttp);

        /* Set the XDD Version into the timestamp header */
        tdp->td_ttp->magic = 0xDEADBEEF;
        snprintf(tdp->td_ttp->version, sizeof(tdp->td_ttp->version), "%s", PACKAGE_STRING);
        
	/* init entries in the trace table header */
	tdp->td_ttp->target_thread_id = tdp->td_pid;
	tdp->td_ttp->res = cycleval;
	tdp->td_ttp->reqsize = tdp->td_xfer_size;
	tdp->td_ttp->blocksize = tdp->td_block_size;
	strcpy(tdp->td_ttp->id, xgp->id); 
	tdp->td_ttp->range = tdp->td_seekhdr.seek_range;
	tdp->td_ttp->start_offset = tdp->td_start_offset;
	tdp->td_ttp->target_offset = tdp->td_planp->target_offset;
	tdp->td_ttp->global_options = xgp->global_options;
	tdp->td_ttp->target_options = tdp->td_target_options;
	t = time(NULL);
	strcpy(tdp->td_ttp->td, ctime(&t));
	tdp->td_ttp->tt_size = tt_entries;
	tdp->td_ttp->trigtime = tsp->ts_trigtime;
	tdp->td_ttp->trigop = tsp->ts_trigop;
	tdp->td_ttp->tt_bytes = tt_bytes;
	tdp->td_ttp->tte_indx = 0;
	tdp->td_ttp->delta = tdp->td_planp->gts_delta;
	tsp->ts_current_entry = 0;

	// Generate the name(s) of the ASCII and/or binary output files

	// First we do the binary output file name
	ts_filename_size = strlen(tdp->td_planp->ts_binary_filename_prefix) + 32;
    tsp->ts_binary_filename = malloc(ts_filename_size);
	if (tsp->ts_binary_filename == NULL) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp binary output filename\n",
			xgp->progname,tdp->td_target_number, ts_filename_size);
		fflush(xgp->errout);
		perror("Reason");
		tsp->ts_options &= ~TS_ON;
		return;
	}
	snprintf(tsp->ts_binary_filename,ts_filename_size,"%s.target.%04d.bin",tdp->td_planp->ts_binary_filename_prefix,tdp->td_target_number);

	// Now do the ASCII output file name
	// If -ts output filename option not used, then dont set it.
    if ( tdp->td_planp->ts_output_filename_prefix != NULL ) {
      ts_filename_size = strlen(tdp->td_planp->ts_output_filename_prefix) + 32;
      tsp->ts_output_filename = malloc(ts_filename_size);
  	  if (tsp->ts_output_filename == NULL) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp output filename\n",
			xgp->progname,tdp->td_target_number, ts_filename_size);
		fflush(xgp->errout);
		perror("Reason");
		tsp->ts_options &= ~TS_ON;
		return;
	  }
	  snprintf(tsp->ts_output_filename,ts_filename_size,"%s.target.%04d.csv",tdp->td_planp->ts_output_filename_prefix,tdp->td_target_number);
    }
	return;
} /* end of xdd_ts_setup() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_write() - write the timestamp entried to a file. 
 */
void
xdd_ts_write(target_data_t *tdp) {
	xint_timestamp_t	*tsp;
	int32_t i;   /* working variable */
	int32_t ttfd;   /* file descriptor for the timestamp file */
	int64_t newsize;  /* new size of the time stamp table */
	xdd_tthdr_t *ttp;   /* pointer to the time stamp table header */


	tsp = tdp->td_tsp;
	ttp = tdp->td_ttp;
	if ((tsp->ts_options & TS_DUMP) == 0)  /* dump only if DUMP was specified */
		return;
	ttfd = open(tsp->ts_binary_filename,O_WRONLY|O_CREAT,0666);
	if (ttfd < 0) {
		fprintf(xgp->errout,"%s: cannot open timestamp table binary output file %s\n", xgp->progname,tsp->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
		return;
	}
	newsize = sizeof(struct xdd_tthdr) + (sizeof(struct tte) * ttp->numents);
	i = write(ttfd,ttp,newsize);
	if (i != newsize) {
		fprintf(xgp->errout,"(%d) %s: cannot write timestamp table binary output file %s\n", tdp->td_target_number, xgp->progname,tsp->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
	}
	fprintf(xgp->output,"Timestamp table written to %s - %lld entries, %lld bytes\n",
		tsp->ts_binary_filename, (long long)ttp->numents, (long long)newsize);

	close(ttfd);
} /* end of xdd_ts_write() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_cleanup() - Free up ts tables and stuff
 */
void
xdd_ts_cleanup(struct xdd_tthdr *ttp) {
// xdd_unlock_memory((unsigned char *)ttp, ttp->tt_bytes, "TimeStampTable");
    free(ttp);
} /* end of xdd_ts_cleanup() */

/*----------------------------------------------------------------------------*/
/* xdd_ts_reports() - Generate the time stamp reports.    
 *    In this section disk_io_time is the time from the start of
 *    I/O to the end of the I/O. The Relative I/O time is 
 *    the time from time 0 to the end of the current I/O.
 *    The Dead Time is the time from the end of an I/O to
 *    the start of the next I/O - time between I/O.
 * IMPORTANT NOTE: All times are in nclk units of nanoseconds.
 *    
 */
void
xdd_ts_reports(target_data_t *tdp) {
	xint_timestamp_t	*tsp;
    int32_t  i;  /* working variable */
    int32_t  count;  /* counter for the number of seeks performed */
    int64_t  hi_dist, lo_dist; /* high and low distances traveled */
    int64_t  total_distance; /* Sum of all distances seeked */
    int64_t  *distance; /* distance between two successive operations */
    int64_t  mean_distance; /* average distance */
    int64_t  mean_iotime; /* average iotime */
    int64_t  disk_start_ts; /* starting timestamp to print*/
    int64_t  disk_end_ts;  /* ending timestamp to print*/
    int64_t  net_start_ts; /* starting timestamp to print*/
    int64_t  net_end_ts;  /* ending timestamp to print*/
    double  fmean_iotime; /* average iotime */
    uint64_t *disk_io_time; /* io time in ms for a single disk operation */
    uint64_t *net_io_time; /* io time in ms for a single net operation */
    double  disk_fio_time; /* io time in floating point */
    double  net_fio_time; /* io time in floating point */
    int64_t  relative_time; /* relative time in ms from the beginning of the pass */
    double  frelative_time; /* same thing in floating point */
    uint64_t hi_time, lo_time; /* high and low times for io times */
    double  fhi_time, flo_time; /* high and low times for io times */
    uint64_t total_time; /* Sum of all io times */
    int64_t  loop_time; /* time between the end of last I/O and end of this one */
    double  floop_time; /* same thing in floating point */
    double  disk_irate; /* instantaneous data rate */
    double  net_irate; /* instantaneous data rate */
    int64_t  indx; /* Current TS index */
    xdd_tthdr_t  *ttp;  /* pointer to the time stamp table header */
    char  *opc;  /* pointer to the operation string */
    char  opc2[8];

	tsp = tdp->td_tsp;
#ifdef WIN32 /* This is required to circumvent the problem of mulitple streams to multiple files */
    /* We need to wait for the previous thread to finish writing its ts report and close the output stream before we can continue */
    WaitForSingleObject(tsp->ts_serializer_mutex,INFINITE);
#endif
    if(tsp->ts_options & TS_SUPPRESS_OUTPUT)
	return;
    if (!(tdp->td_current_state & CURRENT_STATE_PASS_COMPLETE)) {
	fprintf(xgp->errout,"%s: ALERT! ts_reports: target %d has not yet completed! Results beyond this point are unpredictable!\n",
		xgp->progname, tdp->td_target_number);
	fflush(xgp->errout);
    }
    ttp = tdp->td_ttp;
    /* Open the correct output file */
    if (tdp->td_planp->ts_output_filename_prefix != 0) {
	if (tsp->ts_options & TS_APPEND)
	    tdp->td_tsfp = fopen(tsp->ts_output_filename, "a");
	else
	    tdp->td_tsfp = fopen(tsp->ts_output_filename, "w");
	if (tdp->td_tsfp == NULL)  {
	    fprintf(xgp->errout,"Cannot open file '%s' as output for time stamp reports - using stdout\n", tsp->ts_output_filename);
	    fflush(xgp->errout);
	    tdp->td_tsfp = stdout;
	}
    } else tdp->td_tsfp = stdout;
    /* Print the information in the TS header if this is not STDOUT */
    if (tdp->td_tsfp != stdout ) {
	fprintf(tdp->td_tsfp,"Target number for this report, %d\n",tdp->td_target_number);
	xdd_options_info(tdp->td_planp, tdp->td_tsfp);
	fflush(tdp->td_tsfp);
	xdd_system_info(tdp->td_planp, tdp->td_tsfp);
	fflush(tdp->td_tsfp);
	xdd_target_info(tdp->td_tsfp,tdp);
	fflush(tdp->td_tsfp);
    }
    /* allocate some temporary arrays */
    distance = (int64_t *)calloc(ttp->numents,sizeof(int64_t));
    disk_io_time = (uint64_t *)calloc(ttp->numents,sizeof(uint64_t));
    net_io_time = (uint64_t *)calloc(ttp->numents,sizeof(uint64_t));
    if ((distance == NULL) || (disk_io_time == NULL) || (net_io_time == NULL)) {
	fprintf(xgp->errout,"%s: cannot allocate memory for temporary buffers to analyze timestamp info\n",
		xgp->progname);
	fflush(xgp->errout);
	perror("Reason");
	return;
    }
    if (tsp->ts_options & TS_DETAILED) { /* Generate the detailed and summary report */
	if ((tsp->ts_options & TS_WRAP) || (tsp->ts_options & TS_ONESHOT)) {
	    if (ttp->tte_indx == 0) 
		indx = ttp->tt_size - 1;
	    else indx = ttp->tte_indx - 1;
	    fprintf(tdp->td_tsfp, "Last time stamp table entry, %lld\n",
		    (long long)indx);
	}
	
	fprintf(tdp->td_tsfp,"Start of DETAILED Time Stamp Report, Number of entries, %lld, Picoseconds per clock tick, %lld, delta, %lld\n",
		(long long)ttp->numents,
		(long long)ttp->res,
		(long long)ttp->delta);
	
	// Print a header line with the Quantities as they appear across the page
	fprintf(tdp->td_tsfp,"WorkerThread");
	fprintf(tdp->td_tsfp,"WorkerThreadID");
	fprintf(tdp->td_tsfp,",Op");
	fprintf(tdp->td_tsfp,",Pass");
	fprintf(tdp->td_tsfp,",OP");
	fprintf(tdp->td_tsfp,",Location");
	fprintf(tdp->td_tsfp,",Distance");
	fprintf(tdp->td_tsfp,",IOSize");
	fprintf(tdp->td_tsfp,",DiskCPUStart");
	fprintf(tdp->td_tsfp,",DiskCPUEnd");
	fprintf(tdp->td_tsfp,",DiskStart");
	fprintf(tdp->td_tsfp,",DiskEnd");
	fprintf(tdp->td_tsfp,",DiskIOTime");
	fprintf(tdp->td_tsfp,",DiskRate");
	fprintf(tdp->td_tsfp,",RelativeTime");
	fprintf(tdp->td_tsfp,",LoopTime");
	if (tdp->td_target_options & TO_ENDTOEND) {
	    fprintf(tdp->td_tsfp,",NetIOSize");
	    fprintf(tdp->td_tsfp,",NetCPUStart");
	    fprintf(tdp->td_tsfp,",NetCPUEnd");
	    fprintf(tdp->td_tsfp,",NetStart");
	    fprintf(tdp->td_tsfp,",NetEnd");
	    fprintf(tdp->td_tsfp,",NetTime");
	    fprintf(tdp->td_tsfp,",NetRate");
	}
	fprintf(tdp->td_tsfp,"\n");

	// Print the UNITS of the above quantities
	fprintf(tdp->td_tsfp,"Number");
	fprintf(tdp->td_tsfp,"Number");
	fprintf(tdp->td_tsfp,",Type");
	fprintf(tdp->td_tsfp,",Number");
	fprintf(tdp->td_tsfp,",Number");
	fprintf(tdp->td_tsfp,",Bytes");
	fprintf(tdp->td_tsfp,",Blocks");
	fprintf(tdp->td_tsfp,",Bytes");
	fprintf(tdp->td_tsfp,",Number");
	fprintf(tdp->td_tsfp,",Number");
	fprintf(tdp->td_tsfp,",TimeStamp");
	fprintf(tdp->td_tsfp,",TimeStamp");
	fprintf(tdp->td_tsfp,",milliseconds");
	fprintf(tdp->td_tsfp,",MBytes/sec");
	fprintf(tdp->td_tsfp,",milliseconds");
	fprintf(tdp->td_tsfp,",milliseconds");
	if (tdp->td_target_options & TO_ENDTOEND) {
	    fprintf(tdp->td_tsfp,",Bytes");
	    fprintf(tdp->td_tsfp,",Number");
	    fprintf(tdp->td_tsfp,",Number");
	    fprintf(tdp->td_tsfp,",TimeStamp");
	    fprintf(tdp->td_tsfp,",TimeStamp");
	    fprintf(tdp->td_tsfp,",milliseconds");
	    fprintf(tdp->td_tsfp,",MBytes/sec");
	}
	fprintf(tdp->td_tsfp,"\n");
	fflush(tdp->td_tsfp);
    }
    
    /* Scan the time stamp table and calculate the numbers */
    count = 0;
    hi_dist = 0;
    hi_time = 0;
    lo_dist = 1000000000000LL;
    lo_time = 1000000000000LL;
    total_distance = 0;
    total_time = 0;
    for (i = 0; i < ttp->numents; i++) {
	/* print out the detailed information */
	if (i == 0) {
	    distance[i] = 0;
	    loop_time = 0;
	} else {
	    if (ttp->blocksize == 0) {
		fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a blocksize of zero for target %d, setting it to %d\n",
			xgp->progname, tdp->td_target_number, tdp->td_block_size);
		fflush(xgp->errout);
		ttp->blocksize = tdp->td_block_size;
	    }
	    if (ttp->tte[i].byte_offset  > ttp->tte[i-1].byte_offset) {
		distance[i] = (ttp->tte[i].byte_offset - 
			       (ttp->tte[i-1].byte_offset + 
				(ttp->reqsize)));
	    } else {
		distance[i] = (ttp->tte[i-1].byte_offset - 
			       (ttp->tte[i].byte_offset + 
				(ttp->reqsize)));
	    }
	    loop_time = ttp->tte[i].disk_end - ttp->tte[i-1].disk_end;
	}
	disk_io_time[i] = ttp->tte[i].disk_end - ttp->tte[i].disk_start;
	net_io_time[i] = ttp->tte[i].net_end - ttp->tte[i].net_start;
	relative_time = ttp->tte[i].disk_start - ttp->tte[0].disk_start;
	if (i > 0) {
	    if (ttp->tte[i].pass_number > ttp->tte[i-1].pass_number) {
		fprintf(tdp->td_tsfp,"\n");
	    }
	} else { 
	    total_distance += distance[i];
	    if (hi_dist < distance[i]) hi_dist = distance[i];
	    if (lo_dist > distance[i]) lo_dist = distance[i];
	    total_time += disk_io_time[i];
	    if (hi_time < disk_io_time[i]) hi_time = disk_io_time[i];
	    if (lo_time > disk_io_time[i]) lo_time = disk_io_time[i];
	    count++;
	}
	disk_fio_time = (double)disk_io_time[i];
	frelative_time = (double)relative_time;
	floop_time = (double)loop_time;
	if (disk_fio_time > 0.0) 
	    disk_irate = ((ttp->reqsize)/(disk_fio_time / BILLION))/1000000.0;
	else disk_irate = 0.0;
	net_fio_time = (double)net_io_time[i];
	if (net_fio_time > 0.0) 
	    net_irate = ((ttp->reqsize)/(net_fio_time / BILLION))/1000000.0;
	else net_irate = 0.0;
	if (tsp->ts_options & TS_DETAILED) { /* Print the detailed report */
	    disk_start_ts = ttp->tte[i].disk_start + ttp->delta;
	    disk_end_ts = ttp->tte[i].disk_end + ttp->delta;
	    net_start_ts = ttp->tte[i].net_start + ttp->delta;
	    net_end_ts = ttp->tte[i].net_end + ttp->delta;
	    switch (ttp->tte[i].op_type) {
		case SO_OP_READ: 
		case TASK_OP_TYPE_READ: 
		    opc="r"; 
		    break;
		    
		case SO_OP_WRITE: 
		case TASK_OP_TYPE_WRITE: 
		    opc="w"; 
		    break;
		case SO_OP_WRITE_VERIFY: 
		    opc="v"; 
		    break;
		case SO_OP_NOOP: 
		case TASK_OP_TYPE_NOOP: 
		    opc="n"; 
		    break;
		case SO_OP_EOF: 
		case TASK_OP_TYPE_EOF: 
		    opc="e"; 
		    break;
		default: 
		    sprintf(opc2,"0x%02x",ttp->tte[i].op_type); 
		    opc=opc2; 
		    break;
	    }
	    
	    fprintf(tdp->td_tsfp,"%d,",ttp->tte[i].worker_thread_number);
	    fprintf(tdp->td_tsfp,"%d,",ttp->tte[i].thread_id);
	    fprintf(tdp->td_tsfp,"%s,",opc);
	    fprintf(tdp->td_tsfp,"%d,",ttp->tte[i].pass_number); 
	    fprintf(tdp->td_tsfp,"%lld,",(long long)ttp->tte[i].op_number); 
	    fprintf(tdp->td_tsfp,"%lld,",(long long)ttp->tte[i].byte_offset); 
	    fprintf(tdp->td_tsfp,"%lld,",(long long)distance[i]);  
	    fprintf(tdp->td_tsfp,"%lld,",(long long)ttp->tte[i].disk_xfer_size); 
	    fprintf(tdp->td_tsfp,"%d,",  ttp->tte[i].disk_processor_start); 
	    fprintf(tdp->td_tsfp,"%d,",  ttp->tte[i].disk_processor_end); 
	    fprintf(tdp->td_tsfp,"%llu,",(unsigned long long)disk_start_ts);  
	    fprintf(tdp->td_tsfp,"%llu,",(unsigned long long)disk_end_ts); 
	    fprintf(tdp->td_tsfp,"%15.5f,",disk_fio_time/1000000000.0); 
	    fprintf(tdp->td_tsfp,"%15.5f,",disk_irate);
	    fprintf(tdp->td_tsfp,"%15.5f,",frelative_time/1000000000.0); 
	    fprintf(tdp->td_tsfp,"%15.5f,",floop_time/1000000000.0);
	    if (tdp->td_target_options & TO_ENDTOEND) {
		fprintf(tdp->td_tsfp,"%lld,",(long long)ttp->tte[i].net_xfer_size); 
		fprintf(tdp->td_tsfp,"%d,",  ttp->tte[i].net_processor_start); 
		fprintf(tdp->td_tsfp,"%d,",  ttp->tte[i].net_processor_end); 
		fprintf(tdp->td_tsfp,"%llu,",(unsigned long long)net_start_ts);  
		fprintf(tdp->td_tsfp,"%llu,",(unsigned long long)net_end_ts); 
		fprintf(tdp->td_tsfp,"%15.5f,",net_fio_time/1000000000.0); 
		fprintf(tdp->td_tsfp,"%15.3f",net_irate);
	    }
	    fprintf(tdp->td_tsfp,"\n");
	    fflush(tdp->td_tsfp);
	}
    } /* end of FOR loop that scans time stamp table */

    /* Free the arrays with timestamp data */
    free(distance);
    free(disk_io_time);
    free(net_io_time);
    
    if (tsp->ts_options & TS_DETAILED)  /* Print the detailed report trailer */
	fprintf(tdp->td_tsfp,"End of DETAILED Time Stamp Report\n");
    fflush(tdp->td_tsfp);
    if (tsp->ts_options & TS_SUMMARY) {  /* Generate just the summary report */
	if (ttp->numents == 0) {
	    fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a numents of zero for target %d, skipping\n",
					xgp->progname, tdp->td_target_number);
	    fflush(xgp->errout);
	    mean_distance = -1;
	    mean_iotime = -1;
	} else {
	    mean_distance = (uint64_t)(total_distance / ttp->numents);
	    mean_iotime = total_time / ttp->numents;
	}
	fprintf(tdp->td_tsfp, "Start of SUMMARY Time Stamp Report\n");
	fflush(tdp->td_tsfp);
	
	/* display the results */
	if (ttp->blocksize > 0) {
	    fprintf(tdp->td_tsfp,"Average seek distance in %d byte blocks, %lld, request size in blocks, %d\n",
		    ttp->blocksize, 
		    (long long)mean_distance,
		    ttp->reqsize/ttp->blocksize);
	    fflush(tdp->td_tsfp);
	} else {
	    fprintf(tdp->td_tsfp,"No average seek distance with 0 byte blocks\n");
	}
	
	fprintf(tdp->td_tsfp,"Range:  Longest seek distance in blocks, %lld, shortest seek distance in blocks, %lld\n",
		(long long)hi_dist, 
		(long long)lo_dist);
	fflush(tdp->td_tsfp);
	fmean_iotime = (double)mean_iotime;
	fprintf(tdp->td_tsfp,"Average I/O time in milliseconds, %15.5f\n",
				fmean_iotime/1000000000.0);
	fhi_time = (double)hi_time;
	flo_time = (double)lo_time;
	fprintf(tdp->td_tsfp,"I/O Time Range:  Longest I/O time in milliseconds, %15.5f, shortest I/O time in milliseconds, %15.5f\n",
		fhi_time/1000000000.0, flo_time/1000000000.0);
	fprintf(tdp->td_tsfp, "End of SUMMARY Time Stamp Report\n");
    } /* end of IF clause that prints SUMMARY */
    /* close the ts output file if necessary */
    fflush(tdp->td_tsfp);
    if (tdp->td_tsfp != stdout)
	fclose(tdp->td_tsfp);
#ifdef WIN32 /* Allow the next thread to write its file */
    ReleaseMutex(tsp->ts_serializer_mutex);
#endif
} /* end of xdd_ts_reports() */

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
