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
 * This file contains the subroutines necessary the end-to-end
 * Send and Receive functions. 
 */
#include "xdd.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_src_send() - send the data from source to destination 
 * This subroutine will take the message header from the PTDS and 
 * update it with:
 *     - senders QThread number
 *     - time stamp when this packet is being sent normalized to global time
 * The message header is placed after the end of the user data in the
 * message buffer (thus making it a trailer rather than a header).
 * 
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_e2e_src_send(ptds_t *qp) {
	int  	status; /* status of various function calls */
	int 	sent;
	int		sendsize; 
	int		maxmit;
	ptds_t	*p;


	p = qp->target_ptds;
	qp->e2e_header.sendqnum = qp->my_qthread_number;
	pclk_now(&qp->e2e_header.sendtime);
	qp->e2e_header.sendtime += xgp->gts_delta;
	qp->e2e_header.sequence = qp->e2e_msg_sequence_number;
	qp->e2e_header.location = qp->my_current_byte_location;
	qp->e2e_header.length = qp->my_current_io_size;
	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"\nxdd_e2e_src_send: Target %d QThread %d: DEBUG: sending msg seq# %lld, len %lld, loc %lld, magic %08x\n",
			qp->my_target_number,
			qp->my_qthread_number,
			(long long)qp->e2e_header.sequence,
			(long long)qp->e2e_header.length,
			(long long)qp->e2e_header.location,
			qp->e2e_header.magic);
	}

	// The message header for this data packet is placed after the user data in the buffer
	memcpy(qp->rwbuf+qp->iosize,&qp->e2e_header, sizeof(qp->e2e_header));

	// Note: the qp->e2e_iosize is the size of the data field plus the size of the header
	maxmit = MAXMIT_TCP;
	sent = 0;
	pclk_now(&qp->my_current_net_start_time);
	while (sent < qp->e2e_iosize) {
		sendsize = (qp->e2e_iosize-sent) > maxmit ? maxmit : (qp->e2e_iosize-sent);
		status = sendto(qp->e2e_sd,(char *)qp->rwbuf+sent, sendsize, 0, (struct sockaddr *)&qp->e2e_sname, sizeof(struct sockaddr_in));
		sent += status;
	}
	pclk_now(&qp->my_current_net_end_time);
	// Time stamp if requested
	if (p->ts_options & (TS_ON | TS_TRIGGERED)) {
		p->ttp->tte[qp->ts_current_entry].net_start = qp->my_current_net_start_time;
		p->ttp->tte[qp->ts_current_entry].net_end = qp->my_current_net_end_time;
	}

	if (sent != qp->e2e_iosize) {
		xdd_e2e_err(qp,"xdd_e2e_src_send","ERROR: could not send data from e2e source\n");
		return(-1);
	}

	return(0);

} /* end of xdd_e2e_src_send() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_recv() - Receive data from the source side
 * This subroutine will block until data is received or until the
 * connection is broken in which case an error is returned.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_dest_recv(ptds_t *qp) {
	int 	status; 	// status of send/recv function calls 
	int 	rcvd;		// Data Received indicator
	int		recvsize;	// Size of data received
	int		maxmit;		// Max transmit size (for TCP)
	pclk_t 	e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived
	int		errno_save;	// A copy of the errno
	ptds_t	*p;


	p = qp->target_ptds;
	// The following uses strictly TCP
	if (!qp->e2e_wait_1st_msg) 
		pclk_now(&e2e_wait_1st_msg_start_time);

	maxmit = MAXMIT_TCP;

#if (IRIX || WIN32 )
	qp->e2e_nd = getdtablehi();
#endif

	if (xgp->global_options & GO_DEBUG ) {
		fprintf(xgp->errout,"\nxdd_e2e_dest_recv: Target %d QThread %d: DEBUG: Waiting for source to send data... calling select\n",
			qp->my_target_number,
			qp->my_qthread_number);
	}
	status = select(qp->e2e_nd, &qp->e2e_readset, NULL, NULL, NULL);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
	 * client sockets. We first check to see if the sd is in the readset.
	 * If so, this means that a client is trying to make a new connection
	 * in which case we need to issue an accept to establish the connection
	 * and obtain a new Client Socket Descriptor (csd).
	 */
	if (FD_ISSET(qp->e2e_sd, &qp->e2e_readset)) { /* Process an incoming connection */
		qp->e2e_current_csd = qp->e2e_next_csd;

		qp->e2e_csd[qp->e2e_current_csd] = accept(qp->e2e_sd, (struct sockaddr *)&qp->e2e_rname,&qp->e2e_rnamelen);

		FD_SET(qp->e2e_csd[qp->e2e_current_csd], &qp->e2e_active); /* Mark this fd as active */
		FD_SET(qp->e2e_csd[qp->e2e_current_csd], &qp->e2e_readset); /* Put in readset so that it gets processed */

		/* Find the next available csd close to the beginning of the CSD array */
		qp->e2e_next_csd = 0;
		while (qp->e2e_csd[qp->e2e_next_csd] != 0) {
			qp->e2e_next_csd++;
			if (qp->e2e_next_csd == FD_SETSIZE) {
				qp->e2e_next_csd = 0;
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: no csd entries left\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number);
				break;
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	/* This section will check to see which of the Client Socket Descriptors
	 * are in the readset. For those csd's that are ready, a recv is issued to 
	 * receive the incoming data. The clock is then read from pclk() and the
	 * new clock value is sent back to the client.
	 */
	for (qp->e2e_current_csd = 0; qp->e2e_current_csd < FD_SETSIZE; qp->e2e_current_csd++) { // Process all CSDs that are ready
		if (FD_ISSET(qp->e2e_csd[qp->e2e_current_csd], &qp->e2e_readset)) { /* Process this csd */
			/* Receive the destination's current location and length.
		 	 * When the destination closes the socket we get a read
		 	 * indication.  Treat any short send or receive as
		 	 * a failed connection and silently clean up.
		 	 */
			rcvd = 0;
			recvsize = 0;
			pclk_now(&qp->my_current_net_start_time);
			while (rcvd < qp->e2e_iosize) {
				recvsize = (qp->e2e_iosize-rcvd) > maxmit ? maxmit : (qp->e2e_iosize-rcvd);
				status = recvfrom(qp->e2e_csd[qp->e2e_current_csd], (char *) qp->rwbuf+rcvd, recvsize, MSG_WAITALL,NULL,NULL);
				if (status < 0) { 
					fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: Error on recvfrom, status=%d\n",
						xgp->progname,
						qp->my_target_number,
						qp->my_qthread_number,
						status);
					break;
				} else if (status == 0) 
					break;
				rcvd += status;
			} // End of WHILE loop that received incoming data from the source machine
			pclk_now(&qp->my_current_net_end_time);
	 		qp->e2e_header.recvtime = qp->my_current_net_end_time;
			if (!qp->e2e_wait_1st_msg) 
				qp->e2e_wait_1st_msg = qp->e2e_header.recvtime - e2e_wait_1st_msg_start_time;
			qp->e2e_header.recvtime += xgp->gts_delta;
			// Time stamp if requested
			if (p->ts_options & (TS_ON | TS_TRIGGERED)) {
				p->ttp->tte[qp->ts_current_entry].net_start = qp->my_current_net_start_time;
				p->ttp->tte[qp->ts_current_entry].net_end = qp->my_current_net_end_time;
			}

			// Check the status of the last recvfrom()
			// The normal condition where the recvfrom() call returns the expected amount of data (recvsize)
			// which is equal to the size of the e2e_header plus the size of the user payload (iosize)
			if (status == recvsize) { 
				/* Copy meta data into destinations e2e_header struct */
				memcpy(&qp->e2e_header, qp->rwbuf+qp->iosize, sizeof(qp->e2e_header));
				if (xgp->global_options & GO_DEBUG) {
					fprintf(xgp->errout,"\nxdd_e2e_dest_recv: Target %d QThread %d: DEBUG: Receive Status %d: hdr seq# %lld, hdr len %lld, hdr loc %lld, hdr magic %08x e2e_iosize %d, my seq# %lld\n",
						qp->my_target_number,
						qp->my_qthread_number,
						status,
						(long long)qp->e2e_header.sequence,
						(long long)qp->e2e_header.length,
						(long long)qp->e2e_header.location,
						qp->e2e_header.magic, 
						qp->e2e_iosize,
						(long long)qp->e2e_msg_sequence_number);
				}
			}

			// A status of 0 means that the source side shut down unexpectedly - essentially and Enf-Of-File
			if (status == 0) {
				fprintf(xgp->errout,"\n%s: e2e_dest_recv: Target %d QThread %d: ERROR: Connection closed prematurely by Source, op number %lld, location %lld\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					(long long int)qp->target_op_number,
					(long long int)qp->my_current_byte_location);
	  	 	 	// At this point we need to clear out this csd 
	  	 	 	// "Deactivate" the socket. 
	  			FD_CLR(qp->e2e_csd[qp->e2e_current_csd], &qp->e2e_active);
	  			(void) closesocket(qp->e2e_csd[qp->e2e_current_csd]);
	  			qp->e2e_csd[qp->e2e_current_csd] = 0;
				return(-1);
			}

			// A status less than 0 indicates some kind of error.
			if (status < 0) {
				errno_save = errno;
				fprintf(xgp->errout,"\n%s: e2e_dest_recv: Target %d QThread %d: ERROR: recvfrom returned -1, errno %d, op number %lld, location %lld\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					errno,
					(long long int)qp->target_op_number,
					(long long int)qp->my_current_byte_location);

				// Restore the errno and display the reason for the error
				errno = errno_save;
				perror("Reason");
		  		// At this point, if recv returned an error the connection
	  	 	 	// was most likely closed and we need to clear out this csd 
	  	 	 	// "Deactivate" the socket. 
	  			FD_CLR(qp->e2e_csd[qp->e2e_current_csd], &qp->e2e_active);
	  			(void) closesocket(qp->e2e_csd[qp->e2e_current_csd]);
	  			qp->e2e_csd[qp->e2e_current_csd] = 0;
  	  			return(-1);
			} // End of processing a bad receive event

			// Received data but let's check to see what we actually got...
			if ((qp->e2e_header.magic != PTDS_E2E_MAGIC) && 
			    (qp->e2e_header.magic != PTDS_E2E_MAGIQ)) { // Magic numbers don't match!!!
					fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: Bad magic number 0x%08x on recv %d\n",
						xgp->progname,
						qp->my_target_number,
						qp->my_qthread_number,
						qp->e2e_header.magic, 
						qp->e2e_msg_recv);
					return(-1);
			}
			if ((qp->e2e_header.recvtime < qp->e2e_header.sendtime) &&
			    (xgp->gts_time > 0) ) { // Send and recv times look strange!!!
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: WARNING: Possible msg %lld recv time before send time by %llu picoseconds\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					(long long)qp->e2e_header.sequence,
					qp->e2e_header.sendtime-qp->e2e_header.recvtime);
			}

			return(0);

		} // End of IF stmnt that processes a CSD 

	} // End of FOR loop that processes all CSDs that were ready

	qp->e2e_readset = qp->e2e_active;  /* Prepare for the next select */

	return(0);

} /* end of xdd_e2e_dest_recv() */
