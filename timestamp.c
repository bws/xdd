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
	pclk_t cycleval; /* resolution of the clock in picoseconds per ticl */
	time_t t;  /* Time */
	int64_t tt_entries; /* number of entries inthe time stamp table */
	int32_t tt_bytes; /* size of time stamp table in bytes */
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
#endif
	/* check to make sure we really need to do this */
	if (!(p->ts_options & TS_ON) && !(xgp->global_options & GO_DESKEW))
		return;
	/* If DESKEW is TRUE but the TS option was not requested, then do a DESKEW ts setup */
	if ((xgp->global_options & GO_DESKEW) && !(p->ts_options & TS_ON)) {
		p->ts_options |= (TS_ON | TS_ALL | TS_ONESHOT | TS_SUPPRESS_OUTPUT);
		p->ts_size = xgp->passes * p->total_threads;
	} else p->ts_size = xgp->passes * p->target_ops;
	if (p->ts_options & (TS_TRIGTIME | TS_TRIGOP)) 
		p->ts_options &= ~TS_ALL; /* turn off the "time stamp all operations" flag if a trigger was requested */
	if (p->ts_options & TS_TRIGTIME) { /* adjust the trigger time to an actual local time */
		p->ts_trigtime *= TRILLION;
		p->ts_trigtime += xgp->ActualLocalStartTime;
	}
	/* calculate size of the time stamp table and malloc it */
	if (xgp->global_options & GO_DESKEW) { /* This is a case where the target has time stamping already enabled as well as deskew */
		/* Make sure the ts table is large enough for the deskew operation */
		if (p->target_ops < p->total_threads) /* not big enough - make it BIGGER */
			tt_entries = xgp->passes * p->total_threads;
		else tt_entries = xgp->passes * p->target_ops; /* calculate the size */
	} else {
		tt_entries = p->ts_size; 
		if (tt_entries < (xgp->passes * p->target_ops)) { /* Display a NOTICE message if ts_wrap or ts_oneshot have not been specified to compensate for a short time stamp buffer */
			if (((p->ts_options & TS_WRAP) == 0) &&
				((p->ts_options & TS_ONESHOT) == 0) &&
				(!(xgp->global_options & GO_DESKEW))) {
				fprintf(xgp->errout,"%s: ***NOTICE*** The size specified for ime stamp table for target %d is too small - enabling time stamp wrapping to compensate\n",xgp->progname,p->my_target_number);
				fflush(xgp->errout);
				p->ts_options |= TS_WRAP;
			}
		}
	}
	/* calculate the total size in bytes of the time stamp table */
	tt_bytes = (int)((sizeof(struct tthdr)) + (tt_entries * sizeof(struct tte)));
#if (IRIX || SOLARIS || AIX || OSX)
	p->ttp = (struct tthdr *)valloc(tt_bytes);
#else
	p->ttp = (struct tthdr *)malloc(tt_bytes);
#endif
	if (p->ttp == 0) {
fprintf(xgp->errout,"%s: cannot allocate %d bytes of memory for timestamp table for target %d\n",
			xgp->progname,tt_bytes,p->my_target_number);
		fflush(xgp->errout);
#ifdef WIN32 
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"Reason:%s",lpMsgBuf);
		fflush(xgp->errout);
#else 
		perror("Reason");
#endif
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
	p->timestamps = 0;
	return;
} /* end of xdd_ts_setup() */
/*----------------------------------------------------------------------------*/
/* xdd_ts_write() - write the timestamp entried to a file. 
 */
void
xdd_ts_write(ptds_t *p) {
	int32_t i;   /* working variable */
	int32_t ttfd;   /* file descriptor for the timestamp file */
	int32_t newsize;  /* new size of the time stamp table */
	tthdr_t *ttp;   /* pointer to the time stamp table header */
	char tsfilename[1024]; /* name of the timestamp file */


	ttp = p->ttp;
	if ((p->ts_options & TS_DUMP) == 0)  /* dump only if DUMP was specified */
		return;
	sprintf(tsfilename,"%s.target.%04d.qthread.%04d.bin",xgp->tsoutput_filename,p->my_target_number,p->my_qthread_number);
	ttfd = open(tsfilename,O_WRONLY|O_CREAT,0666);
	if (ttfd < 0) {
		fprintf(xgp->errout,"%s: cannot open timestamp table output file %s\n", xgp->progname,tsfilename);
		fflush(xgp->errout);
		perror("reason");
		return;
	}
	newsize = sizeof(struct tthdr) + (sizeof(struct tte) * ttp->numents);
	i = write(ttfd,ttp,newsize);
	if (i != newsize) {
		fprintf(xgp->errout,"(%d) %s: cannot write timestamp table output file %s\n", p->my_target_number, xgp->progname,tsfilename);
		fflush(xgp->errout);
		perror("reason");
	}
	fprintf(xgp->output,"Timestamp table written to %s - %d entries, %d bytes\n",
		tsfilename, ttp->numents, newsize);
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
 *    In this section io_time is the time from the start of
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
	uint64_t block_location; /* current block location */
	int64_t  mean_distance; /* average distance */
	int64_t  mean_iotime; /* average iotime */
	int64_t  start_ts; /* starting timestamp to print*/
	int64_t  end_ts;  /* ending timestamp to print*/
	double  fmean_iotime; /* average iotime */
	int64_t  mean_dead; /* average dead time */
	uint64_t *io_time; /* io time in ms for a single operation */
	double  fio_time; /* io time in floating point */
	int64_t  relative_time; /* relative time in ms from the beginning of the pass */
	double  frelative_time; /* same thing in floating point */
	uint64_t hi_time, lo_time; /* high and low times for io times */
	double  fhi_time, flo_time; /* high and low times for io times */
	uint64_t hi_dead, lo_dead; /* high and low times for dead times */
	double  fhi_dead, flo_dead; /* high and low times for dead times */
	uint64_t total_time; /* Sum of all io times */
	uint64_t total_dead; /* Sum of all dead times */
	uint64_t *dead_time; /* time between the end of last I/O and start of this one */
	double  fdead_time; /* same thing in floating point */
	int64_t  loop_time; /* time between the end of last I/O and end of this one */
	double  floop_time; /* same thing in floating point */
	double  irate; /* instantaneous data rate */
	char  filename[1024]; /* tsoutput filename */
	int64_t  indx; /* Current TS index */
	tthdr_t  *ttp;  /* pointer to the time stamp table header */
	char  *opc;  /* pointer to the operation string */
#ifdef WIN32
	LPVOID lpMsgBuf;
#endif
#ifdef WIN32 /* This is required to circumvent the problem of mulitple streams to multiple files */
	/* We need to wait for the previous thread to finish writing its ts report and close the output stream before we can continue */
	WaitForSingleObject(p->ts_serializer_mutex,INFINITE);
#endif
	if(p->ts_options & TS_SUPPRESS_OUTPUT)
		return;
	if ((p->pass_complete == 0) && (p->run_complete == 0)) {
		fprintf(xgp->errout,"%s: ALERT! ts_reports: target %d thread %d has not yet completed! Results beyond this point are unpredicatable!\n",
						xgp->progname, p->my_target_number, p->my_qthread_number);
		fflush(xgp->errout);
	}
		ttp = p->ttp;
		/* Open the correct output file */
		if (xgp->tsoutput_filename != 0) {
			sprintf(filename,"%s.target.%04d.qthread.%04d.csv",xgp->tsoutput_filename,p->my_target_number,p->my_qthread_number);
			if (p->ts_options && TS_APPEND)
				p->tsfp = fopen(filename, "a");
			else p->tsfp = fopen(filename, "w");
			if (p->tsfp == NULL)  {
				fprintf(xgp->errout,"Cannot open file '%s' as output for time stamp reports - using stdout\n", filename);
				fflush(xgp->errout);
				p->tsfp = stdout;
			}
		} else p->tsfp = stdout;
		/* Print the information in the TS header if this is not STDOUT */
			if (p->tsfp != stdout ) {
				fprintf(p->tsfp,"Target and Qthread number for this report, %d, %d\n",p->my_target_number,p->my_qthread_number);
				xdd_options_info(p->tsfp);
				fflush(p->tsfp);
				xdd_system_info(p->tsfp);
				fflush(p->tsfp);
				xdd_target_info(p->tsfp,p);
				fflush(p->tsfp);
			}
		/* allocate some temporary arrays */
		distance = (int64_t *)calloc(ttp->numents,sizeof(int64_t));
		io_time = (uint64_t *)calloc(ttp->numents,sizeof(uint64_t));
		dead_time = (uint64_t *)calloc(ttp->numents,sizeof(uint64_t));
		if ((distance == NULL) || (io_time == NULL) || (dead_time == NULL)) {
			fprintf(xgp->errout,"%s: cannot allocate memory for temporary buffers to analyze timestamp info\n",
				xgp->progname);
			fflush(xgp->errout);
#ifdef WIN32 
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(xgp->errout,"Reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			ReleaseMutex(p->ts_serializer_mutex);
#else 
			perror("Reason");
#endif
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

	fprintf(p->tsfp,"Start of DETAILED Time Stamp Report, Number of entries, %d, Picoseconds per clock tick, %lld, delta, %lld\n",
		ttp->numents,
		(long long)ttp->res,
		(long long)ttp->delta);

	fprintf(p->tsfp,"Target, RWV Op, Pass, OP Number, Block Location, Distance, StartTS, EndTS, IO_Time_ms, Relative_ms, Delta_us, Loop_ms, Inst MB/sec\n");
	fflush(p->tsfp);
		}
		/* Scan the time stamp table and calculate the numbers */
		count = 0;
		hi_dist = 0;
		hi_time = 0;
		hi_dead = 0;
#if (WIN32)
		lo_dist = 1000000000000;
		lo_time = 1000000000000;
		lo_dead = 1000000000000;
#else
		lo_dist = 1000000000000LL;
		lo_time = 1000000000000LL;
		lo_dead = 1000000000000LL;
#endif
		total_distance = 0;
		total_time = 0;
		total_dead = 0;
		for (i = 0; i < ttp->numents; i++) {
			/* print out the detailed information */
			if (i == 0) {
				distance[i] = 0;
				dead_time[i] = 0;
				loop_time = 0;
			} else {
				if (ttp->blocksize == 0) {
					fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a blocksize of zero for target %d thread %d, setting it to %d\n",
						xgp->progname, p->my_target_number, p->my_qthread_number, p->block_size);
					fflush(xgp->errout);
					ttp->blocksize = p->block_size;
				}
				if (ttp->tte[i].byte_location  > ttp->tte[i-1].byte_location) {
					distance[i] = ((ttp->tte[i].byte_location - 
						(ttp->tte[i-1].byte_location + 
						(ttp->reqsize))) / 
						ttp->blocksize);
				} else {
					distance[i] = ((ttp->tte[i-1].byte_location - 
						(ttp->tte[i].byte_location + 
						(ttp->reqsize))) / 
						ttp->blocksize);
				}
				dead_time[i] = ttp->tte[i].start - ttp->tte[i-1].end;
				loop_time = ttp->tte[i].end - ttp->tte[i-1].end;
			}
			block_location = ttp->tte[i].byte_location/ttp->blocksize; /* normalize the location in blocksized-units */
			io_time[i] = ttp->tte[i].end - ttp->tte[i].start;
			relative_time = ttp->tte[i].start - ttp->tte[0].start;
			if (i > 0) {
				if (ttp->tte[i].pass > ttp->tte[i-1].pass) {
					fprintf(p->tsfp,"\n");
				}
			} else { 
				total_distance += distance[i];
				if (hi_dist < distance[i]) hi_dist = distance[i];
				if (lo_dist > distance[i]) lo_dist = distance[i];
				total_time += io_time[i];
				if (hi_time < io_time[i]) hi_time = io_time[i];
				if (lo_time > io_time[i]) lo_time = io_time[i];
				total_dead += dead_time[i];
				if (hi_dead < dead_time[i]) hi_dead = dead_time[i];
				if (lo_dead > dead_time[i]) lo_dead = dead_time[i];
				count++;
			}
			fio_time = (double)io_time[i];
			frelative_time = (double)relative_time;
			fdead_time = (double)dead_time[i];
			floop_time = (double)loop_time;
			if (fio_time > 0.0) 
				irate = ((ttp->reqsize)/(fio_time / TRILLION))/1000000.0;
			else irate = 0.0;
			if (p->ts_options & TS_DETAILED) { /* Print the detailed report */
				start_ts = ttp->tte[i].start + ttp->delta;
				end_ts = ttp->tte[i].end + ttp->delta;
				switch (ttp->tte[i].rwvop) {
					case SO_OP_WRITE: opc="w"; break;
					case SO_OP_WRITE_VERIFY: opc="v"; break;
					default: opc="r"; break;
				}

	fprintf(p->tsfp,"%d,%s,%d,%d,%lld,%lld,%llu,%llu,%15.5f,%15.5f,%15.5f,%15.5f,%12.3f\n",
				p->my_target_number, 
				opc,
				ttp->tte[i].pass, 
				ttp->tte[i].opnumber, 
				(long long)block_location, 
				(long long)distance[i],  
				(unsigned long long)start_ts,  
				(unsigned long long)end_ts, 
				fio_time/1000000000.0, 
				frelative_time/1000000000.0, 
				fdead_time/1000000.0, 
				floop_time/1000000000.0,
				irate);
			fflush(p->tsfp);
			}
		} /* end of FOR loop that scans time stamp table */
			if (p->ts_options & TS_DETAILED)  /* Print the detailed report trailer */
				fprintf(p->tsfp,"End of DETAILED Time Stamp Report\n");
			fflush(p->tsfp);
		if (p->ts_options & TS_SUMMARY) {  /* Generate just the summary report */
			if (ttp->numents == 0) {
				fprintf(xgp->errout,"%s: ALERT! ts_reports encounterd a numents of zero for target %d thread %d, skipping\n",
					xgp->progname, p->my_target_number, p->my_qthread_number);
				fflush(xgp->errout);
				mean_distance = -1;
				mean_iotime = -1;
			} else {
				mean_distance = (uint64_t)(total_distance / ttp->numents);
				mean_iotime = total_time / ttp->numents;
			}
			if (ttp->numents > 1)
				mean_dead = total_dead / (ttp->numents-1);
			else mean_dead = 0;
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
			fdead_time = (double)mean_dead;
			fprintf(p->tsfp,"Average I/O time in milliseconds, %15.5f, average dead time in microseconds, %15.5f\n",
				fmean_iotime/1000000000.0, fdead_time/1000000.0);
			fhi_time = (double)hi_time;
			flo_time = (double)lo_time;
			fprintf(p->tsfp,"I/O Time Range:  Longest I/O time in milliseconds, %15.5f, shortest I/O time in milliseconds, %15.5f\n",
				fhi_time/1000000000.0, flo_time/1000000000.0);
			fhi_dead = (double)hi_dead;
			flo_dead = (double)lo_dead;
			fprintf(p->tsfp,"Dead Time Range:  Longest dead time in microseconds, %15.5f, shortest dead time in microseconds, %15.5f\n",
				fhi_dead/1000000.0, flo_dead/1000000.0);
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
 
 
