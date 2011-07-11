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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_ts_overhead() - determine time stamp overhead
 */
void
xdd_ts_overhead(struct tthdr *ttp) { 
	int32_t  i;
	pclk_t  tv[101];
	ttp->timer_oh = 0;
	for (i = 0; i < 101; i++) {
		pclk_now(&tv[i]);
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
xdd_ts_setup(ptds_t *p) {
	pclk_t		cycleval; /* resolution of the clock in picoseconds per ticl */
	time_t 		t;  /* Time */
	int64_t 	tt_entries; /* number of entries inthe time stamp table */
	int32_t 	tt_bytes; /* size of time stamp table in bytes */
	int32_t		ts_filename_size; // Number of bytes in the size of the file name


	/* check to make sure we really need to do this */
	if (!(p->ts_options & TS_ON) && !(xgp->global_options & GO_DESKEW))
		return;

	/* If DESKEW is TRUE but the TS option was not requested, then do a DESKEW ts setup */
	if ((xgp->global_options & GO_DESKEW) && !(p->ts_options & TS_ON)) {
		p->ts_options |= (TS_ON | TS_ALL | TS_ONESHOT | TS_SUPPRESS_OUTPUT);
		p->ts_size = (xgp->passes + 1) * p->queue_depth;
	} else p->ts_size = (xgp->passes * p->target_ops) + p->queue_depth;
	if (p->ts_options & (TS_TRIGTIME | TS_TRIGOP)) 
		p->ts_options &= ~TS_ALL; /* turn off the "time stamp all operations" flag if a trigger was requested */
	if (p->ts_options & TS_TRIGTIME) { /* adjust the trigger time to an actual local time */
		p->ts_trigtime *= TRILLION;
		p->ts_trigtime += xgp->ActualLocalStartTime;
	}

	/* Calculate size of the time stamp table and malloc it */
	tt_entries = p->ts_size; 
	if (tt_entries < ((xgp->passes * p->target_ops) + p->queue_depth)) { /* Display a NOTICE message if ts_wrap or ts_oneshot have not been specified to compensate for a short time stamp buffer */
		if (((p->ts_options & TS_WRAP) == 0) &&
			((p->ts_options & TS_ONESHOT) == 0) &&
			(!(xgp->global_options & GO_DESKEW))) {
			fprintf(xgp->errout,"%s: ***NOTICE*** The size specified for timestamp table for target %d is too small - enabling time stamp wrapping to compensate\n",xgp->progname,p->my_target_number);
			fflush(xgp->errout);
			p->ts_options |= TS_WRAP;
		}
	}
	/* calculate the total size in bytes of the time stamp table */
	tt_bytes = (int)((sizeof(struct tthdr)) + (tt_entries * sizeof(struct tte)));
#if (LINUX || SOLARIS || AIX || OSX)
	p->ttp = (struct tthdr *)valloc(tt_bytes);
#else
	p->ttp = (struct tthdr *)malloc(tt_bytes);
#endif
	if (p->ttp == 0) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp table\n",
			xgp->progname,p->my_target_number, tt_bytes);
		fflush(xgp->errout);
		perror("Reason");
		p->ts_options &= ~TS_ON;
		return;
	}
	/* Lock the time stamp table in memory */
	xdd_lock_memory((unsigned char *)p->ttp, tt_bytes, "TIMESTAMP");
	/* clear everything out of the trace table */
	memset(p->ttp,tt_bytes, 0);
	/* get access to the high-res clock*/
	pclk_initialize(&cycleval);
	if (cycleval == PCLK_BAD) {
		fprintf(xgp->errout,"%s: Could not initialize high-resolution clock\n",
			xgp->progname);
		fflush(xgp->errout);
		p->ts_options &= ~TS_ON;
	}
	xdd_ts_overhead(p->ttp);

        /* Set the XDD Version into the timestamp header */
        p->ttp->magic = 0xDEADBEEF;
        p->ttp->version = PACKAGE_STRING;
        
	/* init entries in the trace table header */
	p->ttp->res = cycleval;
	p->ttp->reqsize = p->iosize;
	p->ttp->blocksize = p->block_size;
	strcpy(p->ttp->id, xgp->id); 
	p->ttp->range = p->seekhdr.seek_range;
	p->ttp->start_offset = p->start_offset;
	p->ttp->target_offset = xgp->target_offset;
	p->ttp->global_options = xgp->global_options;
	p->ttp->target_options = p->target_options;
	t = time(NULL);
	strcpy(p->ttp->td, ctime(&t));
	p->ttp->tt_size = tt_entries;
	p->ttp->trigtime = p->ts_trigtime;
	p->ttp->trigop = p->ts_trigop;
	p->ttp->tt_bytes = tt_bytes;
	p->ttp->tte_indx = 0;
	p->ttp->delta = xgp->gts_delta;
	p->ts_current_entry = 0;

	// Generate the name(s) of the ASCII and/or binary output files

	// First we do the binary output file name
	ts_filename_size = sizeof(xgp->ts_binary_filename_prefix) + 32;
    p->ts_binary_filename = malloc(ts_filename_size);
	if (p->ts_binary_filename == NULL) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp binary output filename\n",
			xgp->progname,p->my_target_number, ts_filename_size);
		fflush(xgp->errout);
		perror("Reason");
		p->ts_options &= ~TS_ON;
		return;
	}
	sprintf(p->ts_binary_filename,"%s.target.%04d.bin",xgp->ts_binary_filename_prefix,p->my_target_number);

	// Now do the ASCII output file name
	ts_filename_size = sizeof(xgp->ts_output_filename_prefix) + 32;
    p->ts_output_filename = malloc(ts_filename_size);
	if (p->ts_output_filename == NULL) {
		fprintf(xgp->errout,"%s: xdd_ts_setup: Target %d: ERROR: Cannot allocate %d bytes of memory for timestamp output filename\n",
			xgp->progname,p->my_target_number, ts_filename_size);
		fflush(xgp->errout);
		perror("Reason");
		p->ts_options &= ~TS_ON;
		return;
	}
	sprintf(p->ts_output_filename,"%s.target.%04d.csv",xgp->ts_output_filename_prefix,p->my_target_number);
	return;
} /* end of xdd_ts_setup() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_write() - write the timestamp entried to a file. 
 */
void
xdd_ts_write(ptds_t *p) {
	int32_t i;   /* working variable */
	int32_t ttfd;   /* file descriptor for the timestamp file */
	int64_t newsize;  /* new size of the time stamp table */
	tthdr_t *ttp;   /* pointer to the time stamp table header */


	ttp = p->ttp;
	if ((p->ts_options & TS_DUMP) == 0)  /* dump only if DUMP was specified */
		return;
	ttfd = open(p->ts_binary_filename,O_WRONLY|O_CREAT,0666);
	if (ttfd < 0) {
		fprintf(xgp->errout,"%s: cannot open timestamp table binary output file %s\n", xgp->progname,p->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
		return;
	}
	newsize = sizeof(struct tthdr) + (sizeof(struct tte) * ttp->numents);
	i = write(ttfd,ttp,newsize);
	if (i != newsize) {
		fprintf(xgp->errout,"(%d) %s: cannot write timestamp table binary output file %s\n", p->my_target_number, xgp->progname,p->ts_binary_filename);
		fflush(xgp->errout);
		perror("reason");
	}
	fprintf(xgp->output,"Timestamp table written to %s - %lld entries, %lld bytes\n",
		p->ts_binary_filename, (long long)ttp->numents, (long long)newsize);

	close(ttfd);
} /* end of xdd_ts_write() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_cleanup() - Free up ts tables and stuff
 */
void
xdd_ts_cleanup(struct tthdr *ttp) {
// xdd_unlock_memory((unsigned char *)ttp, ttp->tt_bytes, "TimeStampTable");
// free(ttp);
} /* end of xdd_ts_cleanup() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_reports() - Generate the time stamp reports.    
 *    In this section disk_io_time is the time from the start of
 *    I/O to the end of the I/O. The Relative I/O time is 
 *    the time from time 0 to the end of the current I/O.
 *    The Dead Time is the time from the end of an I/O to
 *    the start of the next I/O - time between I/O.
 * IMPORTANT NOTE: All times are in pclk units of pico seconds.
 *    
 */
void
xdd_ts_reports(ptds_t *p) {
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
	tthdr_t  *ttp;  /* pointer to the time stamp table header */
	char  *opc;  /* pointer to the operation string */
	char  opc2[8];
#ifdef WIN32 /* This is required to circumvent the problem of mulitple streams to multiple files */
	/* We need to wait for the previous thread to finish writing its ts report and close the output stream before we can continue */
	WaitForSingleObject(p->ts_serializer_mutex,INFINITE);
#endif
	if(p->ts_options & TS_SUPPRESS_OUTPUT)
		return;
	if (!(p->my_current_state & CURRENT_STATE_PASS_COMPLETE)) {
		fprintf(xgp->errout,"%s: ALERT! ts_reports: target %d has not yet completed! Results beyond this point are unpredictable!\n",
						xgp->progname, p->my_target_number);
		fflush(xgp->errout);
	}
		ttp = p->ttp;
		/* Open the correct output file */
		if (xgp->ts_output_filename_prefix != 0) {
			if (p->ts_options & TS_APPEND)
				p->tsfp = fopen(p->ts_output_filename, "a");
			else p->tsfp = fopen(p->ts_output_filename, "w");
			if (p->tsfp == NULL)  {
				fprintf(xgp->errout,"Cannot open file '%s' as output for time stamp reports - using stdout\n", p->ts_output_filename);
				fflush(xgp->errout);
				p->tsfp = stdout;
			}
		} else p->tsfp = stdout;
		/* Print the information in the TS header if this is not STDOUT */
			if (p->tsfp != stdout ) {
				fprintf(p->tsfp,"Target number for this report, %d\n",p->my_target_number);
				xdd_options_info(p->tsfp);
				fflush(p->tsfp);
				xdd_system_info(p->tsfp);
				fflush(p->tsfp);
				xdd_target_info(p->tsfp,p);
				fflush(p->tsfp);
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
		if (p->ts_options & TS_DETAILED) { /* Generate the detailed and summary report */
			if ((p->ts_options & TS_WRAP) || (p->ts_options & TS_ONESHOT)) {
				if (ttp->tte_indx == 0) 
					indx = ttp->tt_size - 1;
				else indx = ttp->tte_indx - 1;
				fprintf(p->tsfp, "Last time stamp table entry, %lld\n",
					(long long)indx);
			}

	fprintf(p->tsfp,"Start of DETAILED Time Stamp Report, Number of entries, %lld, Picoseconds per clock tick, %lld, delta, %lld\n",
		(long long)ttp->numents,
		(long long)ttp->res,
		(long long)ttp->delta);

	// Print a header line with the Quantities as they appear across the page
	fprintf(p->tsfp,"QThread");
	fprintf(p->tsfp,",Op");
	fprintf(p->tsfp,",Pass");
	fprintf(p->tsfp,",OP");
	fprintf(p->tsfp,",Location");
	fprintf(p->tsfp,",Distance");
	fprintf(p->tsfp,",IOSize");
	fprintf(p->tsfp,",DiskCPUStart");
	fprintf(p->tsfp,",DiskCPUEnd");
	fprintf(p->tsfp,",DiskStart");
	fprintf(p->tsfp,",DiskEnd");
	fprintf(p->tsfp,",DiskIOTime");
	fprintf(p->tsfp,",DiskRate");
	fprintf(p->tsfp,",RelativeTime");
	fprintf(p->tsfp,",LoopTime");
	if (p->target_options & TO_ENDTOEND) {
		fprintf(p->tsfp,",NetIOSize");
		fprintf(p->tsfp,",NetCPUStart");
		fprintf(p->tsfp,",NetCPUEnd");
		fprintf(p->tsfp,",NetStart");
		fprintf(p->tsfp,",NetEnd");
		fprintf(p->tsfp,",NetTime");
		fprintf(p->tsfp,",NetRate");
	}
	fprintf(p->tsfp,"\n");

	// Print the UNITS of the above quantities
	fprintf(p->tsfp,"Number");
	fprintf(p->tsfp,",Type");
	fprintf(p->tsfp,",Number");
	fprintf(p->tsfp,",Number");
	fprintf(p->tsfp,",Bytes");
	fprintf(p->tsfp,",Blocks");
	fprintf(p->tsfp,",Bytes");
	fprintf(p->tsfp,",Number");
	fprintf(p->tsfp,",Number");
	fprintf(p->tsfp,",TimeStamp");
	fprintf(p->tsfp,",TimeStamp");
	fprintf(p->tsfp,",milliseconds");
	fprintf(p->tsfp,",MBytes/sec");
	fprintf(p->tsfp,",milliseconds");
	fprintf(p->tsfp,",milliseconds");
	if (p->target_options & TO_ENDTOEND) {
		fprintf(p->tsfp,",Bytes");
		fprintf(p->tsfp,",Number");
		fprintf(p->tsfp,",Number");
		fprintf(p->tsfp,",TimeStamp");
		fprintf(p->tsfp,",TimeStamp");
		fprintf(p->tsfp,",milliseconds");
		fprintf(p->tsfp,",MBytes/sec");
	}
	fprintf(p->tsfp,"\n");
	fflush(p->tsfp);
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
						xgp->progname, p->my_target_number, p->block_size);
					fflush(xgp->errout);
					ttp->blocksize = p->block_size;
				}
				if (ttp->tte[i].byte_location  > ttp->tte[i-1].byte_location) {
					distance[i] = (ttp->tte[i].byte_location - 
						(ttp->tte[i-1].byte_location + 
						(ttp->reqsize)));
				} else {
					distance[i] = (ttp->tte[i-1].byte_location - 
						(ttp->tte[i].byte_location + 
						(ttp->reqsize)));
				}
				loop_time = ttp->tte[i].disk_end - ttp->tte[i-1].disk_end;
			}
			disk_io_time[i] = ttp->tte[i].disk_end - ttp->tte[i].disk_start;
			net_io_time[i] = ttp->tte[i].net_end - ttp->tte[i].net_start;
			relative_time = ttp->tte[i].disk_start - ttp->tte[0].disk_start;
			if (i > 0) {
				if (ttp->tte[i].pass_number > ttp->tte[i-1].pass_number) {
					fprintf(p->tsfp,"\n");
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
				disk_irate = ((ttp->reqsize)/(disk_fio_time / TRILLION))/1000000.0;
			else disk_irate = 0.0;
			net_fio_time = (double)net_io_time[i];
			if (net_fio_time > 0.0) 
				net_irate = ((ttp->reqsize)/(net_fio_time / TRILLION))/1000000.0;
			else net_irate = 0.0;
			if (p->ts_options & TS_DETAILED) { /* Print the detailed report */
				disk_start_ts = ttp->tte[i].disk_start + ttp->delta;
				disk_end_ts = ttp->tte[i].disk_end + ttp->delta;
				net_start_ts = ttp->tte[i].net_start + ttp->delta;
				net_end_ts = ttp->tte[i].net_end + ttp->delta;
				switch (ttp->tte[i].op_type) {
					case SO_OP_READ: 
					case OP_TYPE_READ: 
						opc="r"; 
						break;

					case SO_OP_WRITE: 
					case OP_TYPE_WRITE: 
						opc="w"; 
						break;
					case SO_OP_WRITE_VERIFY: 
						opc="v"; 
						break;
					case SO_OP_NOOP: 
					case OP_TYPE_NOOP: 
						opc="n"; 
						break;
					case SO_OP_EOF: 
					case OP_TYPE_EOF: 
						opc="e"; 
						break;
					default: 
						sprintf(opc2,"0x%02x",ttp->tte[i].op_type); 
						opc=opc2; 
						break;
				}

				fprintf(p->tsfp,"%d,",ttp->tte[i].qthread_number);
				fprintf(p->tsfp,"%s,",opc);
				fprintf(p->tsfp,"%d,",ttp->tte[i].pass_number); 
				fprintf(p->tsfp,"%lld,",(long long)ttp->tte[i].op_number); 
				fprintf(p->tsfp,"%lld,",(long long)ttp->tte[i].byte_location); 
				fprintf(p->tsfp,"%lld,",(long long)distance[i]);  
				fprintf(p->tsfp,"%lld,",(long long)ttp->tte[i].disk_xfer_size); 
				fprintf(p->tsfp,"%d,",  ttp->tte[i].disk_processor_start); 
				fprintf(p->tsfp,"%d,",  ttp->tte[i].disk_processor_end); 
				fprintf(p->tsfp,"%llu,",(unsigned long long)disk_start_ts);  
				fprintf(p->tsfp,"%llu,",(unsigned long long)disk_end_ts); 
				fprintf(p->tsfp,"%15.5f,",disk_fio_time/1000000000.0); 
				fprintf(p->tsfp,"%15.5f,",disk_irate);
				fprintf(p->tsfp,"%15.5f,",frelative_time/1000000000.0); 
				fprintf(p->tsfp,"%15.5f,",floop_time/1000000000.0);
				if (p->target_options & TO_ENDTOEND) {
					fprintf(p->tsfp,"%lld,",(long long)ttp->tte[i].net_xfer_size); 
					fprintf(p->tsfp,"%d,",  ttp->tte[i].net_processor_start); 
					fprintf(p->tsfp,"%d,",  ttp->tte[i].net_processor_end); 
					fprintf(p->tsfp,"%llu,",(unsigned long long)net_start_ts);  
					fprintf(p->tsfp,"%llu,",(unsigned long long)net_end_ts); 
					fprintf(p->tsfp,"%15.5f,",net_fio_time/1000000000.0); 
					fprintf(p->tsfp,"%15.3f",net_irate);
				}
				fprintf(p->tsfp,"\n");
				fflush(p->tsfp);
			}
		} /* end of FOR loop that scans time stamp table */
			if (p->ts_options & TS_DETAILED)  /* Print the detailed report trailer */
				fprintf(p->tsfp,"End of DETAILED Time Stamp Report\n");
			fflush(p->tsfp);
		if (p->ts_options & TS_SUMMARY) {  /* Generate just the summary report */
			if (ttp->numents == 0) {
				fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a numents of zero for target %d, skipping\n",
					xgp->progname, p->my_target_number);
				fflush(xgp->errout);
				mean_distance = -1;
				mean_iotime = -1;
			} else {
				mean_distance = (uint64_t)(total_distance / ttp->numents);
				mean_iotime = total_time / ttp->numents;
			}
			fprintf(p->tsfp, "Start of SUMMARY Time Stamp Report\n");
			fflush(p->tsfp);

			/* display the results */
			fprintf(p->tsfp,"Average seek distance in %d byte blocks, %lld, request size in blocks, %d\n",
				ttp->blocksize, 
				(long long)mean_distance,
				ttp->reqsize/ttp->blocksize);
			fflush(p->tsfp);

			fprintf(p->tsfp,"Range:  Longest seek distance in blocks, %lld, shortest seek distance in blocks, %lld\n",
				(long long)hi_dist, 
				(long long)lo_dist);
			fflush(p->tsfp);
			fmean_iotime = (double)mean_iotime;
			fprintf(p->tsfp,"Average I/O time in milliseconds, %15.5f\n",
				fmean_iotime/1000000000.0);
			fhi_time = (double)hi_time;
			flo_time = (double)lo_time;
			fprintf(p->tsfp,"I/O Time Range:  Longest I/O time in milliseconds, %15.5f, shortest I/O time in milliseconds, %15.5f\n",
				fhi_time/1000000000.0, flo_time/1000000000.0);
			fprintf(p->tsfp, "End of SUMMARY Time Stamp Report\n");
		} /* end of IF clause that prints SUMMARY */
		/* close the ts output file if necessary */
		fflush(p->tsfp);
		if (p->tsfp != stdout)
			fclose(p->tsfp);
#ifdef WIN32 /* Allow the next thread to write its file */
	ReleaseMutex(p->ts_serializer_mutex);
#endif
} /* end of xdd_ts_reports() */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
