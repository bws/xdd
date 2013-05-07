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
#include "xint.h"

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
	if ((qp->tgtstp->my_current_io_size % pagesize == 0) && 
		(qp->tgtstp->my_current_byte_location % pagesize) == 0) {
		return;
	}

	// Otherwise, it is necessary to open this target file with DirectIO disabaled
	qp->tgtstp->my_current_pass_number = qp->target_ptds->tgtstp->my_current_pass_number;
	qp->target_options &= ~TO_DIO;
#if (SOLARIS || WIN32)
	// In this OS it is necessary to close the file descriptor before reopening in BUFFERED I/O Mode
	close(qp->fd);
	qp->fd = 0;
#endif
	status = xdd_target_open(qp);
	if (status != 0 ) { /* error opening target */
		fprintf(xgp->errout,"%s: xdd_dio_before_io_op: ERROR: Target %d QThread %d: Reopen of target '%s' failed\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname);
		fflush(xgp->errout);
		xgp->canceled = 1;
	}

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
#elif (LINUX || DARWIN || FREEBSD)
	struct stat	statbuf;
#endif

#if (LINUX || IRIX || SOLARIS || AIX || DARWIN || FREEBSD)
		if ((qp->target_options & TO_READAFTERWRITE) && (qp->target_options & TO_RAW_READER)) { 
// fprintf(stderr,"Reader: RAW check - dataready=%lld, trigger=%x\n",(long long)data_ready,p->rawp->raw_trigger);
			/* Check to see if we can read more data - if not see where we are at */
			if (qp->rawp->raw_trigger & PTDS_RAW_STAT) { /* This section will continually poll the file status waiting for the size to increase so that it can read more data */
				while (qp->rawp->raw_data_ready < qp->iosize) {
					/* Stat the file so see if there is data to read */
#if (LINUX || DARWIN || FREEBSD)
					status = fstat(qp->fd,&statbuf);
#else
					status = fstat64(qp->fd,&statbuf);
#endif
					if (status < 0) {
						fprintf(xgp->errout,"%s: RAW: Error getting status on file\n", xgp->progname);
						qp->rawp->raw_data_ready = qp->iosize;
					} else { /* figure out how much more data we can read */
						qp->rawp->raw_data_ready = statbuf.st_size - qp->tgtstp->my_current_byte_location;
						if (qp->rawp->raw_data_ready < 0) {
							/* The result of this should be positive, otherwise, the target file
							* somehow got smaller and there is a problem. 
							* So, fake it and let this loop exit 
							*/
							fprintf(xgp->errout,"%s: RAW: Something is terribly wrong with the size of the target file...\n",xgp->progname);
							qp->rawp->raw_data_ready = qp->iosize;
						}
					}
				}
			} else { /* This section uses a socket connection to the Destination and waits for the Source to tell it to receive something from its socket */
				while (qp->rawp->raw_data_ready < qp->iosize) {
					/* xdd_raw_read_wait() will block until there is data to read */
					status = xdd_raw_read_wait(qp);
					if (qp->rawp->raw_msg.length != qp->iosize) 

						fprintf(stderr,"error on msg recvd %d loc %lld, length %lld\n",
							qp->rawp->raw_msg_recv-1, 
							(long long)qp->rawp->raw_msg.location,  
							(long long)qp->rawp->raw_msg.length);
					if (qp->rawp->raw_msg.sequence != qp->rawp->raw_msg_last_sequence) {

						fprintf(stderr,"sequence error on msg recvd %d loc %lld, length %lld seq num is %lld should be %lld\n",
							qp->rawp->raw_msg_recv-1, 
							(long long)qp->rawp->raw_msg.location,  
							(long long)qp->rawp->raw_msg.length, 
							(long long)qp->rawp->raw_msg.sequence, 
							(long long)qp->rawp->raw_msg_last_sequence);
					}
					if (qp->rawp->raw_msg_last_sequence == 0) { /* this is the first message so prime the prev_loc and length with the current values */
						qp->rawp->raw_prev_loc = qp->rawp->raw_msg.location;
						qp->rawp->raw_prev_len = 0;
					} else if (qp->rawp->raw_msg.location <= qp->rawp->raw_prev_loc) 
						/* this message is old and can be discgarded */
						continue;
					qp->rawp->raw_msg_last_sequence++;
					/* calculate the amount of data to be read between the end of the last location and the end of the current one */
					qp->rawp->raw_data_length = ((qp->rawp->raw_msg.location + qp->rawp->raw_msg.length) - (qp->rawp->raw_prev_loc + qp->rawp->raw_prev_len));
					qp->rawp->raw_data_ready += qp->rawp->raw_data_length;
					if (qp->rawp->raw_data_length > qp->iosize) 
						fprintf(stderr,"msgseq=%lld, loc=%lld, len=%lld, data_length is %lld, data_ready is now %lld, iosize=%d\n",
							(long long)qp->rawp->raw_msg.sequence, 
							(long long)qp->rawp->raw_msg.location, 
							(long long)qp->rawp->raw_msg.length, 
							(long long)qp->rawp->raw_data_length, 
							(long long)qp->rawp->raw_data_ready, 
							qp->iosize );
					qp->rawp->raw_prev_loc = qp->rawp->raw_msg.location;
					qp->rawp->raw_prev_len = qp->rawp->raw_data_length;
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

	qp->e2ep->e2e_data_recvd = 0; // This will record how much data is recvd in this routine

	// Lets read a packet of data from the Source side
	// xdd_e2e_dest_recv() will block until there is data to read 
	qp->tgtstp->my_current_state |= CURRENT_STATE_DEST_RECEIVE;

fprintf(stderr,"E2E_BEFORE_IO_OP: qp=%p, calling xdd_e2e_dest_recv...\n",qp);
	status = xdd_e2e_dest_recv(qp);
fprintf(stderr,"E2E_BEFORE_IO_OP: qp=%p, returned from xdd_e2e_dest_recv...\n",qp);

	qp->tgtstp->my_current_state &= ~CURRENT_STATE_DEST_RECEIVE;

	// If status is "-1" then soemthing happened to the connection - time to leave
	if (status == -1) 
		return(-1);
		
	// Check to see of this is the last message in the transmission
	if (qp->e2ep->e2e_header.magic == PTDS_E2E_MAGIQ)  { // This must be the End of the File
		return(0);
	}

	// Use the hearder.location as the new my_current_byte_location and the e2e_header.length as the new my_current_io_size for this op
	// This will allow for the use of "no ordering" on the source side of an e2e operation
	qp->tgtstp->my_current_byte_location = qp->e2ep->e2e_header.location;
	qp->tgtstp->my_current_io_size = qp->e2ep->e2e_header.length;
	qp->tgtstp->my_current_op_number = qp->e2ep->e2e_header.sequence;
	// Record the amount of data received 
	qp->e2ep->e2e_data_recvd = qp->e2ep->e2e_header.length;

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
	nclk_t   sleep_time;         /* This is the number of nano seconds to sleep between I/O ops */
	int32_t  sleep_time_dw;     /* This is the amount of time to sleep in milliseconds */
	nclk_t	now;


	if (qp->throtp->throttle <= 0.0)
		return;

	/* If this is a 'throttled' operation, check to see what time it is relative to the start
	 * of this pass, compare that to the time that this operation was supposed to begin, and
	 * go to sleep for how ever many milliseconds is necessary until the next I/O needs to be
	 * issued. If we are past the issue time for this operation, just issue the operation.
	 */
	if (qp->throtp->throttle > 0.0) {
		nclk_now(&now);
		if (qp->throtp->throttle_type & XINT_THROTTLE_DELAY) {
			sleep_time = qp->throtp->throttle*1000000;
		} else { // Process the throttle for IOPS or BW
			now -= qp->tgtstp->my_pass_start_time;
			if (now < qp->seekhdr.seeks[qp->tgtstp->my_current_op_number].time1) { /* Then we may need to sleep */
				sleep_time = (qp->seekhdr.seeks[qp->tgtstp->my_current_op_number].time1 - now) / BILLION; /* sleep time in milliseconds */
				if (sleep_time > 0) {
					sleep_time_dw = sleep_time;
#ifdef WIN32
					Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || DARWIN || FREEBSD) /* Change this line to use usleep */
					if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX )
						sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || DARWIN || FREEBSD) /* Change this line to use usleep */
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

	// Read-After_Write Processing
	xdd_raw_before_io_op(qp);

	// End-to-End Processing
	status = xdd_e2e_before_io_op(qp);
	if (status == -1)  // Error occurred...
		return(-1);

	// DirectIO Handling
	xdd_dio_before_io_op(qp);

	// Throttle Processing
	xdd_throttle_before_io_op(qp);

	return(0);

} // End of xdd_qthread_ttd_before_io_op()

 
