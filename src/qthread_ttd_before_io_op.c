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
#include "xdd.h"

//******************************************************************************
// Things the QThread has to do before each I/O Operation is issued
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_dio_before_io_op - This subroutine will check several conditions to 
 * make sure that DIO will work for this particular I/O operation. 
 * If any of the DIO conditions are not met then DIO is turned off for this 
 * operation and all subsequent operations by this QThread. 
 *
 * This subroutine is called under the context of a QThread.
 *
 */
void
xdd_dio_before_io_op(ptds_t *qp) {
	int		pagesize;
	int		status;


	// Check to see if DIO is enable for this I/O - return if no DIO required
	if (!(qp->target_options & TO_DIO)) 
		return;

	// If this is an SG device with DIO turned on for whatever reason then just exit
	if (qp->target_options & TO_SGIO)
		return;

	// Check to see if this I/O location is aligned on the proper boundary
	pagesize = getpagesize();

	// If the current I/O transfer size is an integer multiple of the page size *AND*
	// if the current byte location (aka offset into the file/device) is an integer multiple
	// of the page size then this I/O operation is fine - just return.
	if ((qp->my_current_io_size % pagesize == 0) && 
		(qp->my_current_byte_location % pagesize) == 0) {
		return;
	}

	// Otherwise, it is necessary to close and reopen this target file with DirectIO disabaled
	qp->target_options &= ~TO_DIO;
	close(qp->fd);
	qp->fd = 0;
	status = xdd_target_open(qp);
	if (status != 0 ) { /* error openning target */
		fprintf(xgp->errout,"%s: xdd_dio_before_io_op: ERROR: Target %d QThread %d: Reopen of target '%s' failed\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname);
		fflush(xgp->errout);
		xgp->canceled = 1;
	}
	// Actually turn DIO back on in case there are more passes
	if (xgp->passes > 1) 
		qp->target_options |= TO_DIO;

} // End of xdd_dio_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_raw_before_io_op(ptds_t *p) {
 *
 * Return Values: 0 is good
 *               -1 indicates an error occured
 */
void
xdd_raw_before_io_op(ptds_t *qp) {
	int	status;

#if (IRIX || SOLARIS || AIX )
	struct stat64	statbuf;
#elif (LINUX || OSX || FREEBSD)
	struct stat	statbuf;
#endif

#if (LINUX || IRIX || SOLARIS || AIX || OSX || FREEBSD)
		if ((qp->target_options & TO_READAFTERWRITE) && (qp->target_options & TO_RAW_READER)) { 
// fprintf(stderr,"Reader: RAW check - dataready=%lld, trigger=%x\n",(long long)data_ready,p->raw_trigger);
			/* Check to see if we can read more data - if not see where we are at */
			if (qp->raw_trigger & PTDS_RAW_STAT) { /* This section will continually poll the file status waiting for the size to increase so that it can read more data */
				while (qp->raw_data_ready < qp->iosize) {
					/* Stat the file so see if there is data to read */
#if (LINUX || OSX || FREEBSD)
					status = fstat(qp->fd,&statbuf);
#else
					status = fstat64(qp->fd,&statbuf);
#endif
					if (status < 0) {
						fprintf(xgp->errout,"%s: RAW: Error getting status on file\n", xgp->progname);
						qp->raw_data_ready = qp->iosize;
					} else { /* figure out how much more data we can read */
						qp->raw_data_ready = statbuf.st_size - qp->my_current_byte_location;
						if (qp->raw_data_ready < 0) {
							/* The result of this should be positive, otherwise, the target file
							* somehow got smaller and there is a problem. 
							* So, fake it and let this loop exit 
							*/
							fprintf(xgp->errout,"%s: RAW: Something is terribly wrong with the size of the target file...\n",xgp->progname);
							qp->raw_data_ready = qp->iosize;
						}
					}
				}
			} else { /* This section uses a socket connection to the Destination and waits for the Source to tell it to receive something from its socket */
				while (qp->raw_data_ready < qp->iosize) {
					/* xdd_raw_read_wait() will block until there is data to read */
					status = xdd_raw_read_wait(qp);
					if (qp->raw_msg.length != qp->iosize) 

						fprintf(stderr,"error on msg recvd %d loc %lld, length %lld\n",
							qp->raw_msg_recv-1, 
							(long long)qp->raw_msg.location,  
							(long long)qp->raw_msg.length);
					if (qp->raw_msg.sequence != qp->raw_msg_last_sequence) {

						fprintf(stderr,"sequence error on msg recvd %d loc %lld, length %lld seq num is %lld should be %lld\n",
							qp->raw_msg_recv-1, 
							(long long)qp->raw_msg.location,  
							(long long)qp->raw_msg.length, 
							(long long)qp->raw_msg.sequence, 
							(long long)qp->raw_msg_last_sequence);
					}
					if (qp->raw_msg_last_sequence == 0) { /* this is the first message so prime the prev_loc and length with the current values */
						qp->raw_prev_loc = qp->raw_msg.location;
						qp->raw_prev_len = 0;
					} else if (qp->raw_msg.location <= qp->raw_prev_loc) 
						/* this message is old and can be discgarded */
						continue;
					qp->raw_msg_last_sequence++;
					/* calculate the amount of data to be read between the end of the last location and the end of the current one */
					qp->raw_data_length = ((qp->raw_msg.location + qp->raw_msg.length) - (qp->raw_prev_loc + qp->raw_prev_len));
					qp->raw_data_ready += qp->raw_data_length;
					if (qp->raw_data_length > qp->iosize) 
						fprintf(stderr,"msgseq=%lld, loc=%lld, len=%lld, data_length is %lld, data_ready is now %lld, iosize=%d\n",
							(long long)qp->raw_msg.sequence, 
							(long long)qp->raw_msg.location, 
							(long long)qp->raw_msg.length, 
							(long long)qp->raw_data_length, 
							(long long)qp->raw_data_ready, 
							qp->iosize );
					qp->raw_prev_loc = qp->raw_msg.location;
					qp->raw_prev_len = qp->raw_data_length;
				}
			}
		} /* End of dealing with a read-after-write */
#endif
} // xdd_raw_before_io_op()
/*----------------------------------------------------------------------------*/
/* xdd_e2e_before_io_op() - This subroutine only does something if this
 * is the Destination side of an End-to-End operation.
 * If this is the Destination side of an End-to-End operation then this 
 * subroutine will call xdd_dest_recv_data() to receive data from the Source side
 * of the End-to-End operation. That call will block until data has been received.
 * at that point, a number of sanity checks are performed and then control
 * is passed back to the caller along with appropriate status. 
 *
 * This subroutine is called under the context of a QThread.
 *
 * Return values:  0 is good
 *                -1 indicates an error
 */
int32_t	
xdd_e2e_before_io_op(ptds_t *qp) {
	pclk_t	beg_time_tmp; 	// Beginning time of a data xfer
	pclk_t	end_time_tmp; 	// End time of a data xfer
	int32_t	status;			// Status of subroutine calls


	// If there is no end-to-end operation then just skip all this...
	if (!(qp->target_options & TO_ENDTOEND)) 
		return(0); 
	// We are the Source side - nothing to do - leave now...
	if (qp->target_options & TO_E2E_SOURCE)
		return(0);

	/* ------------------------------------------------------ */
	/* Start of destination's dealing with an End-to-End op   */
	/* This section uses a socket connection to the source    */
	/* to wait for data from the source, which it then writes */
	/* to the target file associated with this rarget 	  */
	/* ------------------------------------------------------ */
	// We are the Destination side of an End-to-End op
	if (xgp->global_options & GO_DEBUG) {
		fprintf(stderr,"\ne2e_before_io_op: Target %d QThread %d: DEBUG: my_current_op=%lld, my_current_byte_location=%lld, my_current_io_size=%d, my e2e_msg_sequence_number=%lld\n",
			qp->my_target_number,
			qp->my_qthread_number,
			(long long)qp->my_current_op_number,
			(long long)qp->my_current_byte_location,
			qp->my_current_io_size,
			(long long)qp->e2e_msg_sequence_number);
	}

	qp->e2e_data_recvd = 0; // This is how much data is recvd each time we call xdd_e2e_dest_recv()

	// Lets read all the data from the source 
	// xdd_e2e_dest_recv() will block until there is data to read 
	qp->my_current_state |= CURRENT_STATE_DEST_RECEIVE;
	pclk_now(&beg_time_tmp);
	status = xdd_e2e_dest_recv(qp);
	pclk_now(&end_time_tmp);
	qp->e2e_sr_time = (end_time_tmp - beg_time_tmp); // Time spent reading from the source machine
	qp->my_current_state &= ~CURRENT_STATE_DEST_RECEIVE;

	// If status is "-1" then soemthing happened to the connection - time to leave
	if (status == -1) 
		return(-1);
		
	// Check the sequence number of the received msg to what we expect it to be
	// Also note that the msg magic number should not be MAGIQ (end of transmission)
	if ((qp->e2e_header.sequence != qp->e2e_msg_sequence_number) && (qp->e2e_header.magic != PTDS_E2E_MAGIQ )) {
		fprintf(xgp->errout,"\n%s: e2e_before_io_op: Target %d QThread %d: ERROR: Sequence Error on msg recvd loc %lld, length %lld seq num is %lld should be %lld\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number, 
			(long long)qp->e2e_header.location,  
			(long long)qp->e2e_header.length, 
			(long long)qp->e2e_header.sequence, 
			(long long)qp->e2e_msg_sequence_number);
		return(-1);
	}

	// Check to see of this is the last message in the transmission
	// If so, then set the "my_pass_ring" so that we exit gracefully
	if (qp->e2e_header.magic == PTDS_E2E_MAGIQ) 
		qp->my_pass_ring = 1;

	// Display some useful information if we are debugging this thing
	if (xgp->global_options & GO_DEBUG) {
		fprintf(stderr, "\ne2e_before_io_op: Target %d QThread %d: DEBUG: header.sequence=%lld, header.location=%lld, header.length=%lld\n",
			qp->my_target_number, qp->my_qthread_number, (long long)qp->e2e_header.sequence, (long long)qp->e2e_header.location, (long long)qp->e2e_header.length);
		fprintf(stderr, "\ne2e_before_io_op: Target %d QThread %d: DEBUG: e2e_msg_sequence_number=%lld, my_current_byte_location=%lld, my_current_io_size=%d\n",
			qp->my_target_number, qp->my_qthread_number, (long long)qp->e2e_msg_sequence_number, (long long int)qp->my_current_byte_location, qp->my_current_io_size);
	}
	// Check to see which message we are on and set up the msg counters properly
	if (qp->e2e_header.location != qp->my_current_byte_location) {
		fprintf(xgp->errout,"\n%s: e2e_before_io_op: Target %d QThread %d: ERROR: Byte offset location of msg received is %lld but my current byte location is %lld\n", 
			xgp->progname,
			qp->my_target_number, 
			qp->my_qthread_number,
			(long long int)qp->e2e_header.location,
			(long long int)qp->my_current_byte_location); 
		return(-1);
	}

	// Check to see which message we are on and set up the msg counters properly
	if (qp->e2e_header.length != qp->my_current_io_size) {
		fprintf(xgp->errout,"\n%s: e2e_before_io_op: Target %d QThread %d: WARNING: Length of msg received is %lld but my current io size is %lld\n", 
			xgp->progname,
			qp->my_target_number, 
			qp->my_qthread_number,
			(long long int)qp->e2e_header.length,
			(long long int)qp->my_current_io_size); 
	}
	// Record the amount of data received 
	qp->e2e_data_recvd = qp->e2e_header.length;

	return(0);

} // xdd_e2e_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_throttle_before_io_op() - This subroutine implements the throttling
 * mechanism which is essentially a delay before the next I/O operation such
 * that the overall bandwdith or IOP rate meets the throttled value.
 * This subroutine is called in the context of a QThread
 */
void	
xdd_throttle_before_io_op(ptds_t *qp) {
	pclk_t   sleep_time;         /* This is the number of pico seconds to sleep between I/O ops */
	int32_t  sleep_time_dw;     /* This is the amount of time to sleep in milliseconds */
	pclk_t	now;


	if (qp->throttle <= 0.0)
		return;

	/* If this is a 'throttled' operation, check to see what time it is relative to the start
	 * of this pass, compare that to the time that this operation was supposed to begin, and
	 * go to sleep for how ever many milliseconds is necessary until the next I/O needs to be
	 * issued. If we are past the issue time for this operation, just issue the operation.
	 */
	if (qp->throttle > 0.0) {
		pclk_now(&now);
		if (qp->throttle_type & PTDS_THROTTLE_DELAY) {
			sleep_time = qp->throttle*1000000;
		} else { // Process the throttle for IOPS or BW
			now -= qp->my_pass_start_time;
			if (now < qp->seekhdr.seeks[qp->my_current_op_number].time1) { /* Then we may need to sleep */
				sleep_time = (qp->seekhdr.seeks[qp->my_current_op_number].time1 - now) / BILLION; /* sleep time in milliseconds */
				if (sleep_time > 0) {
					sleep_time_dw = sleep_time;
#ifdef WIN32
					Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || OSX || FREEBSD) /* Change this line to use usleep */
					if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX )
						sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || OSX || FREEBSD) /* Change this line to use usleep */
						usleep(sleep_time_dw*1000);
#endif
#endif
				}
			}
		}
	}
} // xdd_throttle_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_qthread_ttd_before_io_op() - This subroutine will do all the stuff 
 * needed to be done before an I/O operation is issued.
 *
 * This subroutine is called in the context of a QThread
 *
 * Return Values: 0 is good
 *               -1 indicates an error occured
 */
int32_t
xdd_qthread_ttd_before_io_op(ptds_t *qp) {
	int32_t	status;	// Return status from various subroutines

	if (xgp->canceled)
		return(0);

	/* init the error number and break flag for good luck */
	errno = 0;
	qp->my_error_break = 0;

	// DirectIO Handling
	xdd_dio_before_io_op(qp);

	// Read-After_Write Processing
	xdd_raw_before_io_op(qp);

	// End-to-End Processing
	status = xdd_e2e_before_io_op(qp);
	if (status == -1)  // Error occurred...
		return(-1);

	// Throttle Processing
	xdd_throttle_before_io_op(qp);

	return(0);

} // End of xdd_qthread_ttd_before_io_op()

 
