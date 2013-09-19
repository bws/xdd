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
 * This file contains the subroutines necessary to perform the time stamping
 * operations during an xdd run.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_ts_overhead() - determine time stamp overhead
 */
void
xdd_ts_overhead(struct xdd_ts_header *ts_hdrp) { 
	int32_t  i;
	nclk_t  tv[101];
	ts_hdrp->tsh_timer_oh = 0;
	for (i = 0; i < 101; i++) {
		nclk_now(&tv[i]);
	}
	for (i = 0; i < 100; i++) 
		ts_hdrp->tsh_timer_oh += (tv[i+1]-tv[i]);
	ts_hdrp->tsh_timer_oh /= 100;
	if (xgp->global_options & GO_TIMER_INFO) { /* only display this information if requested */
		fprintf(xgp->errout,"Timer overhead is %lld nanoseconds\n",
		(long long)(ts_hdrp->tsh_timer_oh/1000));
		fflush(xgp->errout);
	}
} /* End of xdd_ts_overhead() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_setup() - set up the time stamping
 * If time stamping is already turned on for this target then we need to make 
 * sure that the size of the time stamp table is adequate and enlarge it if 
 * necessary. 
 */
void
xdd_ts_setup(target_data_t *tdp) {
	xint_timestamp_t	*tsp;
	nclk_t		cycleval; /* resolution of the clock in nanoseconds per ticl */
	time_t 		t;  /* Time */
	size_t 	    tt_entries; /* number of entries in the time stamp table */
	int32_t 	tt_bytes; /* size of time stamp table in bytes */
	int32_t		ts_filename_size; // Number of bytes in the size of the file name



	tsp = &tdp->td_ts_table;
	if (!(tsp->ts_options & TS_ON))
		return;

	// Calculate the size of the timestamp table needed for this entire run
	tsp->ts_size = (tdp->td_planp->passes * tdp->td_target_ops) + tdp->td_queue_depth;
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
			((tsp->ts_options & TS_ONESHOT) == 0)) {
			fprintf(xgp->errout,"%s: ***NOTICE*** The size specified for timestamp table for target %d is too small - enabling time stamp wrapping to compensate\n",xgp->progname,tdp->td_target_number);
			fflush(xgp->errout);
			tsp->ts_options |= TS_WRAP;
		}
	}
	/* calculate the total size in bytes of the time stamp table */
	tt_bytes = (int)((sizeof(struct xdd_ts_header)) + (tt_entries * sizeof(struct xdd_ts_tte)));
#if (LINUX || SOLARIS || AIX || DARWIN)
	tdp->td_ts_table.ts_hdrp = (struct xdd_ts_header *)valloc(tt_bytes);
if (xgp->global_options & GO_DEBUG_TS) fprintf(stderr,"DEBUG_TS: %lld: xdd_ts_setup: Target: %d: Worker: -: TS INITIALIZATION td_ts_table.ts_hdrp: %p: %d: entries\n ", (long long int)pclk_now(),tdp->td_target_number,tdp->td_ts_table.ts_hdrp,(int)tt_entries);
#else
	tdp->td_ts_table.ts_hdrp = (struct xdd_ts_header *)malloc(tt_bytes);
#endif
	if (tdp->td_ts_table.ts_hdrp == 0) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp table\n",
			xgp->progname,tdp->td_target_number, tt_bytes);
		fflush(xgp->errout);
		perror("Reason");
		tsp->ts_options &= ~TS_ON;
		return;
	}
	/* Lock the time stamp table in memory */
	xdd_lock_memory((unsigned char *)tdp->td_ts_table.ts_hdrp, tt_bytes, "TIMESTAMP");
	/* clear everything out of the trace table */
	memset(tdp->td_ts_table.ts_hdrp,0, tt_bytes);
	/* get access to the high-res clock*/
	nclk_initialize(&cycleval);
	if (cycleval == NCLK_BAD) {
		fprintf(xgp->errout,"%s: Could not initialize high-resolution clock\n",
			xgp->progname);
		fflush(xgp->errout);
		tsp->ts_options &= ~TS_ON;
	}
	xdd_ts_overhead(tdp->td_ts_table.ts_hdrp);

        /* Set the XDD Version into the timestamp header */
        tdp->td_ts_table.ts_hdrp->tsh_magic = 0xDEADBEEF;
        snprintf(tdp->td_ts_table.ts_hdrp->tsh_version, sizeof(tdp->td_ts_table.ts_hdrp->tsh_version), "%s", PACKAGE_STRING);
        
	/* init entries in the trace table header */

	tdp->td_ts_table.ts_hdrp->tsh_target_thread_id = tdp->td_pid;
	tdp->td_ts_table.ts_hdrp->tsh_res = cycleval;
	tdp->td_ts_table.ts_hdrp->tsh_reqsize = tdp->td_xfer_size;
	tdp->td_ts_table.ts_hdrp->tsh_blocksize = tdp->td_block_size;
	strcpy(tdp->td_ts_table.ts_hdrp->tsh_id, xgp->id); 
	tdp->td_ts_table.ts_hdrp->tsh_range = tdp->td_seekhdr.seek_range;
	tdp->td_ts_table.ts_hdrp->tsh_start_offset = tdp->td_start_offset;
	tdp->td_ts_table.ts_hdrp->tsh_target_offset = tdp->td_planp->target_offset;
	tdp->td_ts_table.ts_hdrp->tsh_global_options = xgp->global_options;
	tdp->td_ts_table.ts_hdrp->tsh_target_options = tdp->td_target_options;
	t = time(NULL);
	strcpy(tdp->td_ts_table.ts_hdrp->tsh_td, ctime(&t));
	tdp->td_ts_table.ts_hdrp->tsh_tt_size = tt_entries;
	tdp->td_ts_table.ts_hdrp->tsh_trigtime = tsp->ts_trigtime;
	tdp->td_ts_table.ts_hdrp->tsh_trigop = tsp->ts_trigop;
	tdp->td_ts_table.ts_hdrp->tsh_tt_bytes = tt_bytes;
	tdp->td_ts_table.ts_hdrp->tsh_tte_indx = 0;
	tdp->td_ts_table.ts_hdrp->tsh_delta = tdp->td_planp->gts_delta;
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
if (xgp->global_options & GO_DEBUG_TS) fprintf(stderr,"DEBUG_TS: %lld: xdd_ts_setup: Target: %d: Worker: -: TS INITIALIZATION COMPLETE for td_ts_table.ts_hdrp: %p: %d: entries\n ", (long long int)pclk_now(),tdp->td_target_number,tdp->td_ts_table.ts_hdrp,(int)tt_entries);
if (xgp->global_options & GO_DEBUG_TS) xdd_show_ts_table(&tdp->td_ts_table, tdp->td_target_number);
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
	xdd_ts_header_t *ts_hdrp;   /* pointer to the time stamp table header */


	tsp = &tdp->td_ts_table;
	ts_hdrp = tsp->ts_hdrp;
	if ((tsp->ts_options & TS_DUMP) == 0)  /* dump only if DUMP was specified */
		return;
	ttfd = open(tsp->ts_binary_filename,O_WRONLY|O_CREAT,0666);
	if (ttfd < 0) {
		fprintf(xgp->errout,"%s: cannot open timestamp table binary output file %s\n", xgp->progname,tsp->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
		return;
	}
	newsize = sizeof(struct xdd_ts_header) + (sizeof(struct xdd_ts_tte) * ts_hdrp->tsh_numents);
	i = write(ttfd,ts_hdrp,newsize);
	if (i != newsize) {
		fprintf(xgp->errout,"(%d) %s: cannot write timestamp table binary output file %s\n", tdp->td_target_number, xgp->progname,tsp->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
	}
	fprintf(xgp->output,"Timestamp table written to %s - %lld entries, %lld bytes\n",
		tsp->ts_binary_filename, (long long)ts_hdrp->tsh_numents, (long long)newsize);

	close(ttfd);
} /* end of xdd_ts_write() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_cleanup() - Free up ts tables and stuff
 */
void
xdd_ts_cleanup(struct xdd_ts_header *ts_hdrp) {
// xdd_unlock_memory((unsigned char *)ts_hdrp, ts_hdrp->tsh_tt_bytes, "TimeStampTable");
    free(ts_hdrp);
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
    xdd_ts_header_t  *ts_hdrp;  /* pointer to the time stamp table header */
    char  *opc;  /* pointer to the operation string */
    char  opc2[8];
	FILE	*tsfp;	// The output file pointer

	tsp = &tdp->td_ts_table;
	tsfp = tsp->ts_tsfp;
    ts_hdrp = tdp->td_ts_table.ts_hdrp;
#ifdef WIN32 /* This is required to circumvent the problem of mulitple streams to multiple files */
    /* We need to wait for the previous thread to finish writing its ts report and close the output stream before we can continue */
    WaitForSingleObject(tsp->ts_serializer_mutex,INFINITE);
#endif
    if(tsp->ts_options & TS_SUPPRESS_OUTPUT)
	return;
    if (!(tdp->td_current_state & TARGET_CURRENT_STATE_PASS_COMPLETE)) {
	fprintf(xgp->errout,"%s: ALERT! ts_reports: target %d has not yet completed! Results beyond this point are unpredictable!\n",
		xgp->progname, tdp->td_target_number);
	fflush(xgp->errout);
    }
    /* Open the correct output file */
    if (tdp->td_planp->ts_output_filename_prefix != 0) {
	if (tsp->ts_options & TS_APPEND)
	    tsfp = fopen(tsp->ts_output_filename, "a");
	else
	    tsfp = fopen(tsp->ts_output_filename, "w");
	if (tsfp == NULL)  {
	    fprintf(xgp->errout,"Cannot open file '%s' as output for time stamp reports - using stdout\n", tsp->ts_output_filename);
	    fflush(xgp->errout);
	    tsfp = stdout;
	}
    } else tsfp = stdout;
    /* Print the information in the TS header if this is not STDOUT */
    if (tsfp != stdout ) {
	fprintf(tsfp,"Target number for this report, %d\n",tdp->td_target_number);
	xdd_options_info(tdp->td_planp, tsfp);
	fflush(tsfp);
	xdd_system_info(tdp->td_planp, tsfp);
	fflush(tsfp);
	xdd_target_info(tsfp,tdp);
	fflush(tsfp);
    }
    /* allocate some temporary arrays */
    distance = (int64_t *)calloc(ts_hdrp->tsh_numents,sizeof(int64_t));
    disk_io_time = (uint64_t *)calloc(ts_hdrp->tsh_numents,sizeof(uint64_t));
    net_io_time = (uint64_t *)calloc(ts_hdrp->tsh_numents,sizeof(uint64_t));
    if ((distance == NULL) || (disk_io_time == NULL) || (net_io_time == NULL)) {
	fprintf(xgp->errout,"%s: cannot allocate memory for temporary buffers to analyze timestamp info\n",
		xgp->progname);
	fflush(xgp->errout);
	perror("Reason");
	return;
    }
    if (tsp->ts_options & TS_DETAILED) { /* Generate the detailed and summary report */
	if ((tsp->ts_options & TS_WRAP) || (tsp->ts_options & TS_ONESHOT)) {
	    if (ts_hdrp->tsh_tte_indx == 0) 
		indx = ts_hdrp->tsh_tt_size - 1;
	    else indx = ts_hdrp->tsh_tte_indx - 1;
	    fprintf(tsfp, "Last time stamp table entry, %lld\n",
		    (long long)indx);
	}
	
	fprintf(tsfp,"Start of DETAILED Time Stamp Report, Number of entries, %lld, Picoseconds per clock tick, %lld, delta, %lld\n",
		(long long)ts_hdrp->tsh_numents,
		(long long)ts_hdrp->tsh_res,
		(long long)ts_hdrp->tsh_delta);
	
	// Print a header line with the Quantities as they appear across the page
	fprintf(tsfp,"WorkerThread");
	fprintf(tsfp,"WorkerThreadID");
	fprintf(tsfp,",Op");
	fprintf(tsfp,",Pass");
	fprintf(tsfp,",OP");
	fprintf(tsfp,",Location");
	fprintf(tsfp,",Distance");
	fprintf(tsfp,",IOSize");
	fprintf(tsfp,",DiskCPUStart");
	fprintf(tsfp,",DiskCPUEnd");
	fprintf(tsfp,",DiskStart");
	fprintf(tsfp,",DiskEnd");
	fprintf(tsfp,",DiskIOTime");
	fprintf(tsfp,",DiskRate");
	fprintf(tsfp,",RelativeTime");
	fprintf(tsfp,",LoopTime");
	if (tdp->td_target_options & TO_ENDTOEND) {
	    fprintf(tsfp,",NetIOSize");
	    fprintf(tsfp,",NetCPUStart");
	    fprintf(tsfp,",NetCPUEnd");
	    fprintf(tsfp,",NetStart");
	    fprintf(tsfp,",NetEnd");
	    fprintf(tsfp,",NetTime");
	    fprintf(tsfp,",NetRate");
	}
	fprintf(tsfp,"\n");

	// Print the UNITS of the above quantities
	fprintf(tsfp,"Number");
	fprintf(tsfp,"Number");
	fprintf(tsfp,",Type");
	fprintf(tsfp,",Number");
	fprintf(tsfp,",Number");
	fprintf(tsfp,",Bytes");
	fprintf(tsfp,",Blocks");
	fprintf(tsfp,",Bytes");
	fprintf(tsfp,",Number");
	fprintf(tsfp,",Number");
	fprintf(tsfp,",TimeStamp");
	fprintf(tsfp,",TimeStamp");
	fprintf(tsfp,",milliseconds");
	fprintf(tsfp,",MBytes/sec");
	fprintf(tsfp,",milliseconds");
	fprintf(tsfp,",milliseconds");
	if (tdp->td_target_options & TO_ENDTOEND) {
	    fprintf(tsfp,",Bytes");
	    fprintf(tsfp,",Number");
	    fprintf(tsfp,",Number");
	    fprintf(tsfp,",TimeStamp");
	    fprintf(tsfp,",TimeStamp");
	    fprintf(tsfp,",milliseconds");
	    fprintf(tsfp,",MBytes/sec");
	}
	fprintf(tsfp,"\n");
	fflush(tsfp);
    }
    
    /* Scan the time stamp table and calculate the numbers */
    count = 0;
    hi_dist = 0;
    hi_time = 0;
    lo_dist = 1000000000000LL;
    lo_time = 1000000000000LL;
    total_distance = 0;
    total_time = 0;
    for (i = 0; i < ts_hdrp->tsh_numents; i++) {
	/* print out the detailed information */
	if (i == 0) {
	    distance[i] = 0;
	    loop_time = 0;
	} else {
	    if (ts_hdrp->tsh_blocksize == 0) {
		fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a blocksize of zero for target %d, setting it to %d\n",
			xgp->progname, tdp->td_target_number, tdp->td_block_size);
		fflush(xgp->errout);
		ts_hdrp->tsh_blocksize = tdp->td_block_size;
	    }
	    if (ts_hdrp->tsh_tte[i].tte_byte_offset  > ts_hdrp->tsh_tte[i-1].tte_byte_offset) {
		distance[i] = (ts_hdrp->tsh_tte[i].tte_byte_offset - 
			       (ts_hdrp->tsh_tte[i-1].tte_byte_offset + 
				(ts_hdrp->tsh_reqsize)));
	    } else {
		distance[i] = (ts_hdrp->tsh_tte[i-1].tte_byte_offset - 
			       (ts_hdrp->tsh_tte[i].tte_byte_offset + 
				(ts_hdrp->tsh_reqsize)));
	    }
	    loop_time = ts_hdrp->tsh_tte[i].tte_disk_end - ts_hdrp->tsh_tte[i-1].tte_disk_end;
	}
	disk_io_time[i] = ts_hdrp->tsh_tte[i].tte_disk_end - ts_hdrp->tsh_tte[i].tte_disk_start;
	net_io_time[i] = ts_hdrp->tsh_tte[i].tte_net_end - ts_hdrp->tsh_tte[i].tte_net_start;
	relative_time = ts_hdrp->tsh_tte[i].tte_disk_start - ts_hdrp->tsh_tte[0].tte_disk_start;
	if (i > 0) {
	    if (ts_hdrp->tsh_tte[i].tte_pass_number > ts_hdrp->tsh_tte[i-1].tte_pass_number) {
		fprintf(tsfp,"\n");
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
	    disk_irate = ((ts_hdrp->tsh_reqsize)/(disk_fio_time / BILLION))/1000000.0;
	else disk_irate = 0.0;
	net_fio_time = (double)net_io_time[i];
	if (net_fio_time > 0.0) 
	    net_irate = ((ts_hdrp->tsh_reqsize)/(net_fio_time / BILLION))/1000000.0;
	else net_irate = 0.0;
	if (tsp->ts_options & TS_DETAILED) { /* Print the detailed report */
	    disk_start_ts = ts_hdrp->tsh_tte[i].tte_disk_start + ts_hdrp->tsh_delta;
	    disk_end_ts = ts_hdrp->tsh_tte[i].tte_disk_end + ts_hdrp->tsh_delta;
	    net_start_ts = ts_hdrp->tsh_tte[i].tte_net_start + ts_hdrp->tsh_delta;
	    net_end_ts = ts_hdrp->tsh_tte[i].tte_net_end + ts_hdrp->tsh_delta;
	    switch (ts_hdrp->tsh_tte[i].tte_op_type) {
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
		    sprintf(opc2,"0x%02x",ts_hdrp->tsh_tte[i].tte_op_type); 
		    opc=opc2; 
		    break;
	    }
	    
	    fprintf(tsfp,"%d,",ts_hdrp->tsh_tte[i].tte_worker_thread_number);
	    fprintf(tsfp,"%d,",ts_hdrp->tsh_tte[i].tte_thread_id);
	    fprintf(tsfp,"%s,",opc);
	    fprintf(tsfp,"%d,",ts_hdrp->tsh_tte[i].tte_pass_number); 
	    fprintf(tsfp,"%lld,",(long long)ts_hdrp->tsh_tte[i].tte_op_number); 
	    fprintf(tsfp,"%lld,",(long long)ts_hdrp->tsh_tte[i].tte_byte_offset); 
	    fprintf(tsfp,"%lld,",(long long)distance[i]);  
	    fprintf(tsfp,"%lld,",(long long)ts_hdrp->tsh_tte[i].tte_disk_xfer_size); 
	    fprintf(tsfp,"%d,",  ts_hdrp->tsh_tte[i].tte_disk_processor_start); 
	    fprintf(tsfp,"%d,",  ts_hdrp->tsh_tte[i].tte_disk_processor_end); 
	    fprintf(tsfp,"%llu,",(unsigned long long)disk_start_ts);  
	    fprintf(tsfp,"%llu,",(unsigned long long)disk_end_ts); 
	    fprintf(tsfp,"%15.5f,",disk_fio_time/1000000000.0); 
	    fprintf(tsfp,"%15.5f,",disk_irate);
	    fprintf(tsfp,"%15.5f,",frelative_time/1000000000.0); 
	    fprintf(tsfp,"%15.5f,",floop_time/1000000000.0);
	    if (tdp->td_target_options & TO_ENDTOEND) {
		fprintf(tsfp,"%lld,",(long long)ts_hdrp->tsh_tte[i].tte_net_xfer_size); 
		fprintf(tsfp,"%d,",  ts_hdrp->tsh_tte[i].tte_net_processor_start); 
		fprintf(tsfp,"%d,",  ts_hdrp->tsh_tte[i].tte_net_processor_end); 
		fprintf(tsfp,"%llu,",(unsigned long long)net_start_ts);  
		fprintf(tsfp,"%llu,",(unsigned long long)net_end_ts); 
		fprintf(tsfp,"%15.5f,",net_fio_time/1000000000.0); 
		fprintf(tsfp,"%15.3f",net_irate);
	    }
	    fprintf(tsfp,"\n");
	    fflush(tsfp);
	}
    } /* end of FOR loop that scans time stamp table */

    /* Free the arrays with timestamp data */
    free(distance);
    free(disk_io_time);
    free(net_io_time);
    
    if (tsp->ts_options & TS_DETAILED)  /* Print the detailed report trailer */
	fprintf(tsfp,"End of DETAILED Time Stamp Report\n");
    fflush(tsfp);
    if (tsp->ts_options & TS_SUMMARY) {  /* Generate just the summary report */
	if (ts_hdrp->tsh_numents == 0) {
	    fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a numents of zero for target %d, skipping\n",
					xgp->progname, tdp->td_target_number);
	    fflush(xgp->errout);
	    mean_distance = -1;
	    mean_iotime = -1;
	} else {
	    mean_distance = (uint64_t)(total_distance / ts_hdrp->tsh_numents);
	    mean_iotime = total_time / ts_hdrp->tsh_numents;
	}
	fprintf(tsfp, "Start of SUMMARY Time Stamp Report\n");
	fflush(tsfp);
	
	/* display the results */
	if (ts_hdrp->tsh_blocksize > 0) {
	    fprintf(tsfp,"Average seek distance in %d byte blocks, %lld, request size in blocks, %d\n",
		    ts_hdrp->tsh_blocksize, 
		    (long long)mean_distance,
		    ts_hdrp->tsh_reqsize/ts_hdrp->tsh_blocksize);
	    fflush(tsfp);
	} else {
	    fprintf(tsfp,"No average seek distance with 0 byte blocks\n");
	}
	
	fprintf(tsfp,"Range:  Longest seek distance in blocks, %lld, shortest seek distance in blocks, %lld\n",
		(long long)hi_dist, 
		(long long)lo_dist);
	fflush(tsfp);
	fmean_iotime = (double)mean_iotime;
	fprintf(tsfp,"Average I/O time in milliseconds, %15.5f\n",
				fmean_iotime/1000000000.0);
	fhi_time = (double)hi_time;
	flo_time = (double)lo_time;
	fprintf(tsfp,"I/O Time Range:  Longest I/O time in milliseconds, %15.5f, shortest I/O time in milliseconds, %15.5f\n",
		fhi_time/1000000000.0, flo_time/1000000000.0);
	fprintf(tsfp, "End of SUMMARY Time Stamp Report\n");
    } /* end of IF clause that prints SUMMARY */
    /* close the ts output file if necessary */
    fflush(tsfp);
    if (tsfp != stdout)
	fclose(tsfp);
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
