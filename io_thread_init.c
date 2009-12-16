/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *  I/O Perofrmance, Inc.
 */
/*
   xdd - a basic i/o performance test
*/
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_io_thread() - control of the I/O for the specified reqsize      
 * Notes: Two thread_barriers are used due to an observed race-condition
 * in the xdd_barrier() code, most likely in the semaphore op,
 * that causes some threads to be woken up prematurely and getting the
 * intended synchronization out of sync hanging subsequent threads.
 */
int32_t
xdd_io_thread_init(ptds_t *p) {
	int32_t  i,j;  /* k? random variables */
	int32_t  barrierindx; /* Thread barrier index */
	pclk_t  CurrentLocalTime; /* The time */
	pclk_t  TimeDelta; /* The difference between now and the Actual Local Start Time */
	uint32_t sleepseconds; /* Number of seconds to sleep while waiting for the global time to start */
	int32_t  status;
	char errmsg[256];
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
	HANDLE tmphandle;
#endif

#if (AIX)
	p->mythreadid = thread_self();
#elif (LINUX)
	p->mythreadid = syscall(SYS_gettid);
#else
	p->mythreadid = p->mypid;
#endif
#ifdef WIN32
	/* This next section is used to open the Mutex locks that are used by the semaphores.
	 * This is a Windows-only thing to circumvent a bug in their Mutex inheritence
	 */
	/* If TS is on, then init the ts mutex for later */
	if (p->ts_options & TS_ON) {
		tmphandle = OpenMutex(SYNCHRONIZE,TRUE,xgp->ts_serializer_mutex_name);
			if (tmphandle == NULL) {
				fprintf(stderr,"%s: io_thread_init: Cannot open ts serializer mutex for Target %d, qthread %d\n",
					xgp->progname,p->my_target_number,p->my_qthread_number);
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
			} else p->ts_serializer_mutex = tmphandle;
	}
#endif // Section that deals with MUTEX inheritence 

	/* Now lets get down to business... */
	p->iosize = p->reqsize*p->block_size;
	if (p->iosize == 0) {
		fprintf(xgp->errout,"%s: io_thread_init: ALERT! iothread for target %d queue %d has an iosize of 0, reqsize of %d, blocksize of %d\n",
			xgp->progname, p->my_target_number, p->my_qthread_number, p->reqsize, p->block_size);
		fflush(xgp->errout);
	}
	if (p->numreqs != 0) 
		p->bytes_to_xfer_per_pass = (uint64_t)(p->numreqs * p->iosize);
	else if (p->mbytes > 0)
		p->bytes_to_xfer_per_pass = (uint64_t)p->mbytes * ONEMEG;
	else if (p->kbytes > 0)
		p->bytes_to_xfer_per_pass = (uint64_t)p->kbytes * 1024;
	else if (p->bytes > 0)
		p->bytes_to_xfer_per_pass = (uint64_t)p->bytes;
	/* This calculates the number of iosize (or smaller) operations that need to be performed. 
	 * In the event the number of bytes to transfer is not an integer number of iosize requests then 
	 * the total number of ops is incremented by 1 and the last I/O op will be the residual.
	 */
	p->total_ops = p->bytes_to_xfer_per_pass / p->iosize;
	if (p->bytes_to_xfer_per_pass % p->iosize) {
		p->total_ops++;
		p->last_iosize = p->bytes_to_xfer_per_pass % p->iosize;
	}
	/* Assign me to the proper processor if I have a processor number 0 or greater. */
	if (p->processor != -1)
		xdd_processor(p);

	/* open the target device */
	p->fd = xdd_open_target(p);
	if ((unsigned int)p->fd == -1) { /* error openning target */
		fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to open failure\n",xgp->progname,p->my_target_number);
		fflush(xgp->errout);
		xgp->abort_io = 1;
		/* Enter the serializer barrier so that the next thread can start */
		xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
		return(FAILED);
	}

	/* set up the seek list */
	p->seekhdr.seek_total_ops = p->total_ops;
	p->seekhdr.seeks = (seek_t *)calloc((int32_t)p->seekhdr.seek_total_ops,sizeof(seek_t));
	if (p->seekhdr.seeks == 0) {
		fprintf(xgp->errout,"%s: io_thread_init: cannot allocate memory for seek list\n",xgp->progname);
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
		xgp->abort_io = 1;
		return(FAILED);
	}
	xdd_init_seek_list(p);
	/* allocate a buffer and re-align it if necessary */
	p->rwbuf = xdd_init_io_buffers(p);
	if (p->rwbuf == NULL) { /* error allocating memory for I/O buffer */
		fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to I/O memory buffer allocation failure\n",xgp->progname,p->my_target_number);
		fflush(xgp->errout);
		xgp->abort_io = 1;
		/* Enter the serializer barrier so that the next thread can start */
		xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
		return(FAILED);
	}
	p->rwbuf_save = p->rwbuf; 
    xdd_pattern_buffer(p); // Put the correct data pattern in the buffer

	if (p->mem_align != getpagesize()) 
			p->rwbuf +=  p->mem_align;
	/* set up the timestamp table */
	xdd_ts_setup(p);
	/* set up for the big loop */
	if (xgp->max_errors == 0) 
		xgp->max_errors = p->total_ops;

	/* If we are synchronizing to a Global Clock, let's synchronize
	 * here so that we all start at *roughly* the same time
	 */
	if (xgp->gts_addr) {
		pclk_now(&CurrentLocalTime);
		while (CurrentLocalTime < xgp->ActualLocalStartTime) {
		    TimeDelta = ((xgp->ActualLocalStartTime - CurrentLocalTime)/TRILLION);
	    	    if (TimeDelta > 2) {
			sleepseconds = TimeDelta - 2;
			sleep(sleepseconds);
		    }
		    pclk_now(&CurrentLocalTime);
		}
	}
	if (xgp->global_options & GO_TIMER_INFO) {
		fprintf(xgp->errout,"Starting now...\n");
		fflush(xgp->errout);
	}

	/* This section will initialize the slave side of the lock step mutex and barriers */
	if (p->ls_master >= 0) { /* This means that this target has a master and therefore must be a slave */
		/* init the task counter mutex and the lock step barrier */
		status = pthread_mutex_init(&p->ls_mutex, 0);
		if (status) {
			sprintf(errmsg,"%s: io_thread_init:Error initializing lock step slave target %d task counter mutex",
				xgp->progname,p->my_target_number);
			perror(errmsg);
			fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
				xgp->progname,p->my_target_number);
			fflush(xgp->errout);
			xgp->abort_io = 1;
			/* Enter the serializer barrier so that the next thread can start */
			xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
			return(FAILED);
		}
		for (i=0; i<=1; i++) {
				sprintf(p->Lock_Step_Barrier[i].name,"LockStep_T%d_%d",p->ls_master,p->my_target_number);
				xdd_init_barrier(&p->Lock_Step_Barrier[i], 2, p->Lock_Step_Barrier[i].name);
		}
		p->Lock_Step_Barrier_Slave_Index = 0;
		p->Lock_Step_Barrier_Master_Index = 0;
	} else { /* Make sure these are uninitialized */
		for (i=0; i<=1; i++) {
				p->Lock_Step_Barrier[i].initialized  = FALSE;
		}
		p->Lock_Step_Barrier_Slave_Index = 0;
		p->Lock_Step_Barrier_Master_Index = 0;
	}
	/* Check to see if we will be waiting for a start trigger. 
	 * If so, then init the start trigger barrier.
	 */
	if (p->target_options & TO_WAITFORSTART) {
		for (i=0; i<=1; i++) {
				sprintf(p->Start_Trigger_Barrier[i].name,"StartTrigger_T%d_%d",p->my_target_number,p->my_qthread_number);
				xdd_init_barrier(&p->Start_Trigger_Barrier[i], 2, p->Start_Trigger_Barrier[i].name);
		}
		p->Start_Trigger_Barrier_index = 0;
		p->run_status = 0;
	}
	/* This section will check to see if we are doing a read-after-write and initialize as needed.
	 * If we are a reader then we need to init the socket and listen for the writer.
	 * If we are the writer, then we assume that the reader has already been started on another
	 * machine and all we need to do is connect to that reader and say Hello.
	 */
	if (p->target_options & TO_READAFTERWRITE) {
		if (p->rwratio == 1.0 )
			xdd_raw_reader_init(p);
		else xdd_raw_writer_init(p);
	}
	/* --------------------------------------------------------------------------------------------
	* This section will check to see if we are doing a write-after-read and initialize as needed.
 	* If we are a writer then we need to init the socket and listen for the reader.
 	* If we are the reader, then we assume that the writer has already been started on another
 	* machine and all we need to do is connect to that writer and say Hello.
 	* -------------------------------------------------------------------------------------------- */
	if (p->target_options & TO_ENDTOEND) {
		p->e2e_sr_time = 0;
		if (p->target_options & TO_E2E_DESTINATION) { // This is the Destination side of an End-to-End
			status = xdd_e2e_dest_init(p); 
		} else if (p->target_options & TO_E2E_SOURCE) { // This is the Source side of an End-to-End
			status = xdd_e2e_src_init(p); 
		} else { // Not sure which side of the E2E this target is supposed to be....
			fprintf(xgp->errout,"%s: io_thread_init: Cannot determine which side of the E2E operation this target <%d> is supposed to be.\n",
				xgp->progname,p->my_target_number);
			fprintf(xgp->errout,"%s: io_thread_init: Check to be sure that the '-e2e issource' or '-e2e isdestination' was specified for this target.\n",
				xgp->progname);
			fflush(xgp->errout);
			return(FAILED);
		}
		if (status == FAILED) {
			fprintf(xgp->errout,"%s: io_thread_init: E2E %s initialization failed for target %d - terminating.\n",
				xgp->progname,
				(p->target_options & TO_E2E_DESTINATION) ? "DESTINATION":"SOURCE", 
				p->my_target_number);
			return(FAILED);
		}
	}
	/* If I am the last thread to start (my_target_number == number_of_targets-1)
	 * then set the estimated end time if a -runtime was specified.
	 */
	if ((p->my_target_number == (xgp->number_of_targets-1)) && (p->my_qthread_number == (p->queue_depth-1))) {
		xgp->run_ring = 0; /* This is the alarm that goes off when the run time is up */
		if (xgp->runtime > 0) {
			pclk_now(&xgp->estimated_end_time);
			xgp->estimated_end_time += ((int64_t)(xgp->runtime*TRILLION)); 
                }
		else xgp->estimated_end_time = 0;
	}
    
    /* Display all the information related to this target */
    xdd_target_info(xgp->output, p);
	if (xgp->csvoutput) 
		xdd_target_info(xgp->csvoutput, p);

    return(SUCCESS);
} // end of xdd_io_thread_init()
 
