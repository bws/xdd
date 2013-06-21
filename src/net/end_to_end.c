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
#include "xint.h"

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
xdd_e2e_src_send(worker_data_t *wdp) {
	int 	sent;
	int		sendsize; 
	int		sentcalls; 
	int		maxmit;


	wdp->wd_e2ep->e2e_header.sendqnum = wdp->wd_thread_number;
	wdp->wd_e2ep->e2e_header.sequence = wdp->wd_current_op_number;
	wdp->wd_e2ep->e2e_header.location = wdp->wd_current_byte_location;
	wdp->wd_e2ep->e2e_header.length = wdp->wd_current_io_size;

	// The message header for this data packet is placed after the user data in the buffer
	memcpy(wdp->wd_current_rwbuf+wdp->wd_current_io_size,&wdp->wd_e2ep->e2e_header, sizeof(wdp->wd_e2ep->e2e_header));
#ifdef ndef
	//FIXME
	if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) 
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_start = xdd_get_processor();
#endif

	// Note: the wdp->wd_e2ep->e2e_io_size is the size of the data field plus the size of the header
	maxmit = MAXMIT_TCP;
	sent = 0;
        sentcalls = 0;
	nclk_now(&wdp->wd_current_net_start_time);
	while (sent < wdp->wd_e2ep->e2e_io_size) {
		sendsize = (wdp->wd_e2ep->e2e_io_size-sent) > maxmit ? maxmit : (wdp->wd_e2ep->e2e_io_size-sent);
		wdp->wd_e2ep->e2e_send_status = sendto(wdp->wd_e2ep->e2e_sd,(char *)wdp->wd_current_rwbuf+sent, sendsize, 0, (struct sockaddr *)&wdp->wd_e2ep->e2e_sname, sizeof(struct sockaddr_in));
		sent += wdp->wd_e2ep->e2e_send_status;
                sentcalls++;
fprintf(stderr,"E2E_SOURCE_SIDE_SEND: Sent %d bytes\n",wdp->wd_e2ep->e2e_send_status);
	}
	nclk_now(&wdp->wd_current_net_end_time);
	// Time stamp if requested
#ifdef ndef
	// FIXME
	if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) {
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_size = wdp->wd_e2ep->e2e_io_size;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_start = wdp->wd_current_net_start_time;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_end = wdp->wd_current_net_end_time;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_end = xdd_get_processor();
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_calls = sentcalls;
	}
#endif
	
	// Calculate the Send/Receive time by the time it took the last sendto() to run
	wdp->wd_e2ep->e2e_sr_time = (wdp->wd_current_net_end_time - wdp->wd_current_net_start_time);

	if (sent != wdp->wd_e2ep->e2e_io_size) {
		xdd_e2e_err(wdp,"xdd_e2e_src_send","ERROR: could not send data from e2e source\n");
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
xdd_e2e_dest_recv(worker_data_t *wdp) {
	target_data_t	*tdp;
	int 	rcvd_so_far;	// Cumulative number of bytes received if multiple recvfrom() invocations are necessary
	int		recvsize; 		// The number of bytes to receive for this invocation of recvfrom()
	int		recvcalls; 		// The number of calls to recvfrom() to receive recvsize bytes
	int		headersize; 	// Size of the E2E Header
	int		maxmit;			// Maximum TCP transmission size
	nclk_t 	e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived
	int		errno_save;		// A copy of the errno


	tdp = wdp->wd_tdp;
	// The following uses strictly TCP
	nclk_now(&e2e_wait_1st_msg_start_time);

	maxmit = MAXMIT_TCP;
	headersize = sizeof(xdd_e2e_header_t);

#if (IRIX || WIN32 )
	wdp->wd_e2ep->e2e_nd = getdtablehi();
#endif

	select(wdp->wd_e2ep->e2e_nd, &wdp->wd_e2ep->e2e_readset, NULL, NULL, NULL);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
	 * client sockets. We first check to see if the sd is in the readset.
	 * If so, this means that a client is trying to make a new connection
	 * in which case we need to issue an accept to establish the connection
	 * and obtain a new Client Socket Descriptor (csd).
	 */
	if (FD_ISSET(wdp->wd_e2ep->e2e_sd, &wdp->wd_e2ep->e2e_readset)) { /* Process an incoming connection */
		wdp->wd_e2ep->e2e_current_csd = wdp->wd_e2ep->e2e_next_csd;

		wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd] = accept(wdp->wd_e2ep->e2e_sd, (struct sockaddr *)&wdp->wd_e2ep->e2e_rname,&wdp->wd_e2ep->e2e_rnamelen);

		FD_SET(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], &wdp->wd_e2ep->e2e_active); /* Mark this fd as active */
		FD_SET(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], &wdp->wd_e2ep->e2e_readset); /* Put in readset so that it gets processed */

		/* Find the next available csd close to the beginning of the CSD array */
		wdp->wd_e2ep->e2e_next_csd = 0;
		while (wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_next_csd] != 0) {
			wdp->wd_e2ep->e2e_next_csd++;
			if (wdp->wd_e2ep->e2e_next_csd == FD_SETSIZE) {
				wdp->wd_e2ep->e2e_next_csd = 0;
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: no csd entries left\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_thread_number);
				break;
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	/* This section will check to see which of the Client Socket Descriptors
	 * are in the readset. For those csd's that are ready, a recv is issued to 
	 * receive the incoming data. 
	 */
	for (wdp->wd_e2ep->e2e_current_csd = 0; wdp->wd_e2ep->e2e_current_csd < FD_SETSIZE; wdp->wd_e2ep->e2e_current_csd++) { // Process all CSDs that are ready
		if (FD_ISSET(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], &wdp->wd_e2ep->e2e_readset)) { /* Process this csd */
#ifdef ndef
			// FIXME
			if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) 
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_start = xdd_get_processor();
#endif
			rcvd_so_far = 0;
                        recvcalls = 0;
			nclk_now(&wdp->wd_current_net_start_time);
			while (rcvd_so_far < wdp->wd_e2ep->e2e_io_size) {
				recvsize = (wdp->wd_e2ep->e2e_io_size-rcvd_so_far) > maxmit ? maxmit : (wdp->wd_e2ep->e2e_io_size-rcvd_so_far);
				wdp->wd_e2ep->e2e_recv_status = recvfrom(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], (char *) wdp->wd_current_rwbuf+rcvd_so_far, recvsize, 0, NULL,NULL);
				// Check for other conditions that will get us out of this loop
fprintf(stderr,"E2E_DEST_SIDE_RECEIVE: Recv %d bytes\n",wdp->wd_e2ep->e2e_recv_status);
				if (wdp->wd_e2ep->e2e_recv_status <= 0)
					break;
				// Otherwise, figure out how much data we got and go back for more if necessary
				rcvd_so_far += wdp->wd_e2ep->e2e_recv_status;
				recvcalls++;
			} // End of WHILE loop that received incoming data from the source machine

			// bws: BAND-AID PATCH: EOF message padded to io_size, header must be sent at end;
			// however, later code expects the header to be at the beginning of the buffer for EOF
			if (rcvd_so_far == wdp->wd_e2ep->e2e_io_size && 
				((struct xdd_e2e_header*)(wdp->wd_current_rwbuf+wdp->wd_current_io_size))->magic == PTDS_E2E_MAGIQ) {
				memmove(wdp->wd_current_rwbuf, wdp->wd_current_rwbuf+wdp->wd_current_io_size, sizeof(struct xdd_e2e_header));
fprintf(stderr,"E2E_DEST_SIDE_RECEIVE: RECEIVED an E2E_MAGIQ\n");
				wdp->wd_e2ep->e2e_recv_status = headersize;
			} else if (wdp->wd_e2ep->e2e_recv_status > 0) {
				wdp->wd_e2ep->e2e_recv_status = rcvd_so_far;
			}

			nclk_now(&wdp->wd_current_net_end_time);

			// This will record the amount of time that we waited from the time we started until we got the first packet
			if (!wdp->wd_e2ep->e2e_wait_1st_msg) 
				wdp->wd_e2ep->e2e_wait_1st_msg = wdp->wd_current_net_end_time - e2e_wait_1st_msg_start_time;

			// Timestamp this operation if requested
#ifdef ndef
			// FIXME
			if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) {
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_start = wdp->wd_current_net_start_time;
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_end = wdp->wd_current_net_end_time;
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_end = xdd_get_processor();
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_calls = recvcalls;
			}
#endif

			// If this is the first packet received by this QThread then record the *end* of this operation as the
			// *start* of this pass. The reason is that the initial recvfrom() may have been issued long before the
			// Source side started sending data and we need to ignore that startup delay. 
			if (wdp->wd_first_pass_start_time == LONGLONG_MAX)  { // This is an indication that this is the fist recvfrom() that has completed
				wdp->wd_first_pass_start_time = wdp->wd_current_net_end_time;
				wdp->wd_pass_start_time =  wdp->wd_current_net_end_time;
				wdp->wd_e2ep->e2e_sr_time = 0; // The first Send/Receive time is zero.
			} else { // Calculate the Send/Receive time by the time it took the last recvfrom() to run
				wdp->wd_e2ep->e2e_sr_time = (wdp->wd_current_net_end_time - wdp->wd_current_net_start_time);
			}
			// Check the status of the last recvfrom()
			// The normal condition where the recvfrom() call returns the expected amount of data (recvsize)
			// which is equal to the size of the e2e_header plus the size of the user payload (io_size)
			// There are two cases: 
			//    (1) Transfer of a normal packet of data plus the header 
			//    (2) Transfer of just the header with no data - this should be an End-of-File packet from the Source side
			//        and will have the header.magic number set to MAGIQ indicating that it is an EOF packet.
			if (wdp->wd_e2ep->e2e_recv_status == wdp->wd_e2ep->e2e_io_size) {  // This is the total amount of data we should have received (data+header)
				/* Copy meta data into destinations e2e_header struct */
				memcpy(&wdp->wd_e2ep->e2e_header, wdp->wd_current_rwbuf+wdp->wd_current_io_size, sizeof(wdp->wd_e2ep->e2e_header));
	 			wdp->wd_e2ep->e2e_header.recvtime = wdp->wd_current_net_end_time; // This needs to be the net_end_time from this side of the operation
			} else if (wdp->wd_e2ep->e2e_recv_status == headersize) { // This should be an EOF packet from the Source Side but check the magic number to be certain
				memcpy(&wdp->wd_e2ep->e2e_header, wdp->wd_current_rwbuf, sizeof(wdp->wd_e2ep->e2e_header)); // In this case the header is at the very beginning of the RW Buffer because there is no data
	 			wdp->wd_e2ep->e2e_header.recvtime = wdp->wd_current_net_end_time; // This needs to be the net_end_time from this side of the operation
			} else if (wdp->wd_e2ep->e2e_recv_status == 0) { // A status of 0 means that the source side shut down unexpectedly - essentially and Enf-Of-File
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: Connection closed prematurely by Source, op number %lld, location %lld\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_thread_number,
					(long long int)wdp->wd_current_op_number,
					(long long int)wdp->wd_current_byte_location);
	  	 	 	// At this point we need to clear out this csd and "Deactivate" the socket. 
	  			FD_CLR(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], &wdp->wd_e2ep->e2e_active);
	  			(void) closesocket(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd]);
	  			wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd] = 0;
				return(-1);
			} else if (wdp->wd_e2ep->e2e_recv_status < 0) { // A status less than 0 indicates some kind of error.
				errno_save = errno;
				fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: recvfrom returned -1, errno %d, op number %lld, location %lld\n",
					xgp->progname,
					tdp->td_target_number,
					wdp->wd_thread_number,
					errno,
					(long long int)wdp->wd_current_op_number,
					(long long int)wdp->wd_current_byte_location);

				// Restore the errno and display the reason for the error
				errno = errno_save;
				perror("Reason");
		  		// At this point, if recv returned an error the connection
	  	 	 	// was most likely closed and we need to clear out this csd 
	  	 	 	// "Deactivate" the socket. 
	  			FD_CLR(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd], &wdp->wd_e2ep->e2e_active);
	  			(void) closesocket(wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd]);
	  			wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd] = 0;
  	  			return(-1);
			} // End of processing a bad receive event

			// Received data but let's check to see what we actually got...
			if ((wdp->wd_e2ep->e2e_header.magic != PTDS_E2E_MAGIC) &&  
				(wdp->wd_e2ep->e2e_header.magic != PTDS_E2E_MAGIQ)) { // Magic number is not correct!
					fprintf(xgp->errout,"\n%s: xdd_e2e_dest_recv: Target %d QThread %d: ERROR: Bad magic number 0x%08x on recv %d - should be either 0x%08x or 0x%08x\n",
						xgp->progname,
						tdp->td_target_number,
						wdp->wd_thread_number,
						wdp->wd_e2ep->e2e_header.magic, 
						wdp->wd_e2ep->e2e_msg_recv,
						PTDS_E2E_MAGIC, 
						PTDS_E2E_MAGIQ);
					return(-1);
			} 

   			// If time stamping is on then we need to reset these values
#ifdef ndef
			// FIXME
   			if ((p->tsp->ts_options & (TS_ON|TS_TRIGGERED))) {
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_size = wdp->wd_e2ep->e2e_recv_status;
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].byte_location = wdp->wd_e2ep->e2e_header.location;
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].disk_xfer_size = wdp->wd_e2ep->e2e_header.length;
				p->ttp->tte[wdp->wd_tsp->ts_current_entry].op_number = wdp->wd_e2ep->e2e_header.sequence;
				if (wdp->wd_e2ep->e2e_header.magic == PTDS_E2E_MAGIQ)
					 p->ttp->tte[wdp->wd_tsp->ts_current_entry].op_type = SO_OP_EOF;
				else p->ttp->tte[wdp->wd_tsp->ts_current_entry].op_type = SO_OP_WRITE;
			}
#endif
			return(0);

		} // End of IF stmnt that processes a CSD 

	} // End of FOR loop that processes all CSDs that were ready

	wdp->wd_e2ep->e2e_readset = wdp->wd_e2ep->e2e_active;  /* Prepare for the next select */

	return(0);

} /* end of xdd_e2e_dest_recv() */
/*----------------------------------------------------------------------*/
/* xdd_e2e_eof_source_side() - End-Of-File processing for Source 
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_e2e_eof_source_side(worker_data_t *wdp) {
	int 		sent;		// Cumulative number of bytes sent if multiple sendto() invocations are necessary
	int 		sentcalls;	// Cumulative number of calls      if multiple sendto() invocations are necessary
	int		sendsize; 	// The number of bytes to send for this invocation of sendto()
	int		maxmit;		// Maximum TCP transmission size


	maxmit = MAXMIT_TCP;

	nclk_now(&wdp->wd_current_net_start_time);
	wdp->wd_e2ep->e2e_header.sendqnum = wdp->wd_thread_number;
	wdp->wd_e2ep->e2e_header.sequence = (wdp->wd_current_op_number + wdp->wd_thread_number); // This is an EOF packet header
	wdp->wd_e2ep->e2e_header.location = -1; // NA
	wdp->wd_e2ep->e2e_header.length = 0;	// NA - no data being sent other than the header
	wdp->wd_e2ep->e2e_header.magic = PTDS_E2E_MAGIQ;

	// bws: BAND-AID PATCH: convert application protocol to use fixed-size messages
	// previously an EOF message was exactly the size of a header,
	// now it must be placed at the end to conform with data message protocol
	memcpy(wdp->wd_current_rwbuf+wdp->wd_current_io_size, &wdp->wd_e2ep->e2e_header, sizeof(wdp->wd_e2ep->e2e_header));

#ifdef ndef
//FIXME
	if (p->tsp->ts_options & (TS_ON | TS_TRIGGERED)) 
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_start = xdd_get_processor();
#endif
	// This will send the E2E Header to the Destination
	sent = 0;
	sentcalls = 0;
	while (sent < wdp->wd_e2ep->e2e_io_size) {
		sendsize = (wdp->wd_e2ep->e2e_io_size-sent) > maxmit ? maxmit : (wdp->wd_e2ep->e2e_io_size-sent);

		wdp->wd_e2ep->e2e_send_status = sendto(wdp->wd_e2ep->e2e_sd,((char *)wdp->wd_current_rwbuf)+sent, sendsize, 0, (struct sockaddr *)&wdp->wd_e2ep->e2e_sname, sizeof(struct sockaddr_in));
		if (wdp->wd_e2ep->e2e_send_status <= 0) {
			xdd_e2e_err(wdp,"xdd_e2e_eof_source_side","ERROR: error sending EOF to destination\n");
			return(-1);
		}
fprintf(stderr,"E2E_EOF_SOURCE_SIDE: Sent %d bytes\n",wdp->wd_e2ep->e2e_send_status);
		sent += wdp->wd_e2ep->e2e_send_status;
		sentcalls++;
	}
	nclk_now(&wdp->wd_current_net_end_time);
	
	// Calculate the Send/Receive time by the time it took the last sendto() to run
	wdp->wd_e2ep->e2e_sr_time = (wdp->wd_current_net_end_time - wdp->wd_current_net_start_time);
	// If time stamping is on then we need to reset these values
#ifdef ndef
//FIXME
   	if ((p->tsp->ts_options & (TS_ON|TS_TRIGGERED))) {
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_start = wdp->wd_current_net_start_time;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_end = wdp->wd_current_net_end_time;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_processor_end = xdd_get_processor();
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_size = sent;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].net_xfer_calls = sentcalls;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].byte_location = -1;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].disk_xfer_size = 0;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].op_number = wdp->wd_e2ep->e2e_header.sequence;
		p->ttp->tte[wdp->wd_tsp->ts_current_entry].op_type = OP_TYPE_EOF;
	}
#endif


	if (sent != wdp->wd_e2ep->e2e_io_size) {
		xdd_e2e_err(wdp,"xdd_e2e_eof_source_side","ERROR: could not send EOF to destination\n");
		return(-1);
	}

	return(0);
} /* end of xdd_e2e_eof_source_side() */

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
