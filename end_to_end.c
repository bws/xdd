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
 * This file contains the subroutines necessary to perform an end-to-end
 * performance test aka write-after-read operation across two different 
 * machines running xdd.
 */
#include "xdd.h"

#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE 128
#endif

#define MAXMIT_TCP     (1<<28)
#define MAXMIT_UDP      65504

void xdd_e2e_set_socket_opts(char *sktname, int skt);
void xdd_e2e_prt_socket_opts(char *sktname, int skt);
void xdd_e2e_err(char const *fmt, ...);
void xdd_e2e_gethost_error(char *str);
void xdd_e2e_error_check(int val, char *str);
int32_t xdd_e2e_dest_wait_UDP(ptds_t *p);

static int xxxTCPWin = 10000000;

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_set_socket_opts() - set the options for specified socket.
 *
 */
void xdd_e2e_set_socket_opts(char *sktname, int skt) {
	int status;
	int newTCPWin = xxxTCPWin;
	int level = SOL_SOCKET;
#if WIN32
	char  optionvalue;
#else
	int  optionvalue;
#endif

	/* Create the socket and set some options */
	optionvalue = 1;
	status = setsockopt(skt, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
	if (status != 0) {
		xdd_e2e_err("Error setting TCP_NODELAY \n");
	}
	status = setsockopt(skt,level,SO_SNDBUF,(char *)&newTCPWin,sizeof(newTCPWin));
	(void) xdd_e2e_error_check ( status, "xdd_e2e_set_socket_opts: setsockopt SO_SNDBUF");
	status = setsockopt(skt,level,SO_RCVBUF,(char *)&newTCPWin,sizeof(newTCPWin));
	(void) xdd_e2e_error_check ( status, "xdd_e2e_set_socket_opts: setsockopt SO_RCVBUF");
	status = setsockopt(skt,level,SO_REUSEADDR,(char *)&newTCPWin,sizeof(newTCPWin));
	(void) xdd_e2e_error_check ( status, "xdd_e2e_set_socket_opts: setsockopt SO_REUSEPORT");
	if (xgp->global_options & GO_DEBUG) {
			fprintf(xgp->errout,"%7s set socket buffer sizes| send %11d, | recv %11d | reuse_addr 1\n",
				sktname, newTCPWin, newTCPWin); 
	}

} // End of xdd_e2e_set_socket_opts()
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_prt_socket_opts() - print the currnet options for specified 
 *   socket.
 *
 */
void xdd_e2e_prt_socket_opts(char *sktname, int skt) {
	int level = SOL_SOCKET;
	//  int level = IPPROTO_UDP;
	int 	sockbuf_sizs;
	int		sockbuf_sizr;
	int		reuse_addr;
	int		optlen;


	optlen = sizeof(sockbuf_sizs);
	getsockopt(skt,level,SO_SNDBUF,(char *)&sockbuf_sizs,&optlen);
	optlen = sizeof(sockbuf_sizr);
	getsockopt(skt,level,SO_RCVBUF,(char *)&sockbuf_sizr,&optlen);
	optlen = sizeof(reuse_addr);
	getsockopt(skt,level,SO_REUSEADDR,(char *)&reuse_addr,&optlen);
	if (xgp->global_options & GO_DEBUG ) {
		fprintf(xgp->errout,"%7s get socket buffer sizes| send %11d, | recv %11d | reuse_addr %5d\n",
			sktname, sockbuf_sizs, sockbuf_sizr, reuse_addr);
	}
} // End of xdd_e2e_prt_socket_opts()

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_gethost_error() - This subroutine will display an error message
 * based on the value of h_errno which is the equivalent of errno 
 * but strictly for gethostbyname and gethostbyaddr operations.
 * The input parameter is a point to the associated string to print before
 * official explanation.
 */
void 
xdd_e2e_gethost_error(char *str) {
	char *errmsg;

	switch (h_errno) {
		case HOST_NOT_FOUND:
			errmsg="The specified host is unknown.";
			break;
 		case NO_ADDRESS: // aka NO_DATA which are the same value of h_errno
			errmsg="The requested name is valid but does not have an IP address <NO_ADDRESS>.";
			break;
		case NO_RECOVERY:
			errmsg="A non-recoverable name server error occurred.";
			break;
		case TRY_AGAIN:
			errmsg="A temporary error occurred on an authoritative name server.  Try again later.";
			break;
		default:
			errmsg="Unknown h_errno code.";
			break;
	}

	fprintf(xgp->errout,"%s: %s: Reason: <h_errno: %d> %s\n", xgp->progname, str, h_errno, errmsg);

} // End of xdd_e2e_error_check()
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_error_check(fmt...)
 *
 */
void 
xdd_e2e_error_check(int val, char *str) {
	if (val < 0) {
		fprintf(xgp->errout,"%s: %s :%d: %s\n", xgp->progname, str, val, strerror(errno));
	}
} // End of xdd_e2e_error_check()

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_err(fmt...)
 *
 * General-purpose error handling routine.  Prints the short message
 * provided to standard error, along with some boilerplate information
 * such as the program name and errno value.  Any remaining arguments
 * are used in printed message (the usage here takes the same form as
 * printf()).
 */
void
xdd_e2e_err(char const *fmt, ...)
{
#ifdef WIN32
	LPVOID lpMsgBuf;
    fprintf(xgp->errout, "last error was %d\n", WSAGetLastError());
	FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"Reason: %s",lpMsgBuf);
#endif /* WIN32 */
	fprintf(xgp->errout,"%s: ",xgp->progname);
	fprintf(xgp->errout, fmt);
	perror(" Reason");
	return;
} /* end of xdd_e2e_err() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_sockets_init() - for e2e operations - Windows Only
 *
 * Windows requires the WinSock startup routine to be run
 * before running a bunch of socket routines.  We encapsulate
 * that here in case some other environment needs something similar.
 *
 * Returns true if startup was successful, false otherwise.
 *
 * The sample code I based this on happened to be requesting
 * (and verifying) a WinSock DLL version 2.2 environment was
 * present, and it worked, so I kept it that way.
 */
int32_t
xdd_e2e_sockets_init(void) {
#ifdef WIN32
	WSADATA wsaData; /* Data structure used by WSAStartup */
	int wsastatus; /* status returned by WSAStartup */
	char *reason;
	wsastatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsastatus != 0) { /* Error in starting the network */
		switch (wsastatus) {
		case WSASYSNOTREADY:
			reason = "Network is down";
			break;
		case WSAVERNOTSUPPORTED:
			reason = "Request version of sockets <2.2> is not supported";
			break;
		case WSAEINPROGRESS:
			reason = "Another Windows Sockets operation is in progress";
			break;
		case WSAEPROCLIM:
			reason = "The limit of the number of sockets tasks has been exceeded";
			break;
		case WSAEFAULT:
			reason = "Program error: pointer to wsaData is not valid";
			break;
		default:
			reason = "Unknown error code";
			break;
		};
		fprintf(xgp->errout,"%s: Error initializing network connection\nReason: %s\n",
			xgp->progname, reason);
		fflush(xgp->errout);
		WSACleanup();
		return(FAILED);
	}
#endif

	return(SUCCESS);

} /* end of xdd_e2e_sockets_init() */

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_setup_dest_socket() - Set up the socket on the Destination side
 *
 */
int32_t
xdd_e2e_setup_dest_socket(ptds_t *p) {
	int  	status;
	char 	msg[256];
	int 	type;
	struct 	timeval udp_timeout;  


	if (!p) { // Technically, this should *never* happen
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_dest_socket: Internal error - no ptds pointer!\n",
			xgp->progname,
			p->mythreadnum);
		return(FAILED);
	}

	// Init the sockets - This is actually just for Windows that requires some additional initting
	status = xdd_e2e_sockets_init(); 
	if (status == FAILED) {
		xdd_e2e_err("xdd_e2e_setup_dest_socket: could not initialize sockets for e2e destination\n");
		return(FAILED);
	}

	if(p->e2e_dest_hostname == NULL) {
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_dest_socket: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
			xgp->progname,
			p->mythreadnum);
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_dest_socket: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
			xgp->progname,
			p->mythreadnum);
		return(FAILED);
	}
	// Get the IP address of this host 
	p->e2e_dest_hostent = gethostbyname(p->e2e_dest_hostname);
	if (! p->e2e_dest_hostent) {
		xdd_e2e_gethost_error("xdd_e2e_setup_dest_socket: unable to identify host");
		return(FAILED);
	}

	// Translate the address to a 32-bit number
	p->e2e_dest_addr = ntohl(((struct in_addr *) p->e2e_dest_hostent->h_addr)->s_addr);
	p->e2e_dest_port += p->mythreadnum; // Add the thread number to the base port to handle queuedepths > 1.

	// Set the "type" of socket being requested: for TCP, type=SOCK_STREAM, for UDP, type=SOCK_DGRAM
	type = SOCK_STREAM;

	// Create the socket 
	p->e2e_sd = socket(AF_INET, type, IPPROTO_TCP);
	if (p->e2e_sd < 0) {
		xdd_e2e_err("error opening socket\n");
		return(FAILED);
	}
	sprintf(msg,"[mythreadnum %d]:xdd_e2e_setup_dest_socket: socket addr 0x%08x port %d created ",
		p->mythreadnum,
		p->e2e_dest_addr, 
		p->e2e_dest_port);

	(void) xdd_e2e_set_socket_opts (msg,p->e2e_sd);

	/* Bind the name to the socket */
	(void) memset(&p->e2e_sname, 0, sizeof(p->e2e_sname));
	p->e2e_sname.sin_family = AF_INET;
	p->e2e_sname.sin_addr.s_addr = htonl(p->e2e_dest_addr);
	p->e2e_sname.sin_port = htons(p->e2e_dest_port);
	p->e2e_snamelen = sizeof(p->e2e_sname);
	if (bind(p->e2e_sd, (struct sockaddr *) &p->e2e_sname, p->e2e_snamelen)) {
		sprintf(msg,"Error binding name to socket - addr=0x%08x, port=0x%08x, specified as %d plus mythread number %d\n",
			p->e2e_sname.sin_addr.s_addr, 
			p->e2e_sname.sin_port,
			p->e2e_dest_port,
			p->mythreadnum);
		xdd_e2e_err(msg);
		return(FAILED);
	}

	// Address is converted back at this point so that we can use it later for send/recv
	p->e2e_dest_addr = ntohl(p->e2e_sname.sin_addr.s_addr);
	p->e2e_dest_port = ntohs(p->e2e_sname.sin_port);

	/* All set; prepare to accept connection requests */
	if (type == SOCK_STREAM) { // If this is a stream socket then we need to listen for incoming data
		status = listen(p->e2e_sd, SOMAXCONN);
		if (status) {
			xdd_e2e_err("Error starting LISTEN on socket\n");
			return(FAILED);
		}
	}

	// Misc debugging information
	if (xgp->global_options & GO_DEBUG) {
		if (type == SOCK_STREAM) 
			strcpy(msg,"TCP:SOCK_STREAM");
		else strcpy(msg,"UDP:SOCK_DGRAM");
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_setup_dest_socket: Destination hostname is %s, Socket address is 0x%x Port %d <0x%x>, %s\n", 
			p->mythreadnum,
			p->e2e_dest_hostname, 
			p->e2e_dest_addr, 
			p->e2e_dest_port,
			p->e2e_dest_port,
			msg);
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_setup_dest_socket: Done preparing for connection request\n",p->mythreadnum);
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_setup_dest_socket: Listenning for connections.....\n",p->mythreadnum);
	}

	return(SUCCESS);

} /* end of xdd_e2e_setup_dest_socket() */

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_dest_init() - init the destination side 
 * This routine is called from the xdd io thread before all the action 
 * starts. When calling this routine, it is because this thread is on the
 * "destination" side of an End-to-End test. Hence, this routine needs
 * to set up a socket on the appropriate port and 
 *
 */
int32_t
xdd_e2e_dest_init(ptds_t *p) {
	int  status; /* status of various function calls */


	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"xdd_e2e_dest_init[mythread %d]: Initializing destination\n",p->mythreadnum);
	}

	// Check to make sure that the destination target is actually *writing* the data it receives to a file or device
	if (p->rwratio > 0.0) { // Something is wrong - the destination file/device is not 100% write
		fprintf(xgp->errout,"%s: xdd_e2e_dest_init: Error - E2E destination target[%d] file/device '%s' is not being *written*: rwratio=%5.2f\n",
			xgp->progname,
			p->my_target_number,
			p->target,
			p->rwratio);
		return(FAILED);
	}
	
	/* Set up the destination-side sockets */
	status = xdd_e2e_setup_dest_socket(p);
	if (status == FAILED) {
		xdd_e2e_err("xdd_e2e_dest_init: could not init e2e destination socket\n");
		return(FAILED);
	}

	// Set up the descriptor table for the select() call
	if (p->e2e_useUDP) { // This section is used to prepare for UDP
		fprintf(xgp->errout,"%s: xdd_e2e_dest_init: UDP not yet supported\n",xgp->progname);
		return(FAILED);
	} else { // This section is used when we are using TCP 
		/* clear out the csd table */
		for (p->e2e_current_csd = 0; p->e2e_current_csd < FD_SETSIZE; p->e2e_current_csd++)
			p->e2e_csd[p->e2e_current_csd] = 0;

		// Set the current and next csd indices to 0
		p->e2e_current_csd = p->e2e_next_csd = 0;

		/* Initialize the socket sets for select() */
		FD_ZERO(&p->e2e_readset);
		FD_SET(p->e2e_sd, &p->e2e_readset);
		p->e2e_active = p->e2e_readset;
		p->e2e_current_csd = p->e2e_next_csd = 0;
	
		/* Find out how many sockets are in each set (berkely only) */
#if (IRIX || WIN32 )
		p->e2e_nd = getdtablehi();
#endif
#if (LINUX || OSX)
		p->e2e_nd = getdtablesize();
#endif
#if (AIX)
		p->e2e_nd = FD_SETSIZE;
#endif
#if (SOLARIS )
		p->e2e_nd = FD_SETSIZE;
#endif
	} // Done setting up the table for select() <TCP>

	// Initialize the message counter and sequencer to 0
	p->e2e_msg_recv = 0;
	p->e2e_msg_last_sequence = 0;

	//  Say good night Dick
	return(SUCCESS);

} /* end of xdd_e2e_dest_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_dest_wait() - wait for data from the source side
 *
 */
int32_t
xdd_e2e_dest_wait(ptds_t *p) {
	int 	status; 	// status of send/recv function calls 
	int 	rcvd;		// Data Received indicator
	int		recvsize;	// Size of data received
	int		maxmit;		// Max transmit size (for TCP)
	pclk_t 	e2e_wait_1st_msg_start_time; // This is the time stamp of when the first message arrived


	// If this is a UDP transfer then that is handled elsewhere
	if (p->e2e_useUDP) {
		status = xdd_e2e_dest_wait_UDP(p);
		return(status);
	} 

	// The following uses strictly TCP

	if (!p->e2e_wait_1st_msg) 
		pclk_now(&e2e_wait_1st_msg_start_time);

	maxmit = MAXMIT_TCP;

#if (IRIX || WIN32 )
	p->e2e_nd = getdtablehi();
#endif

	if (xgp->global_options & GO_DEBUG ) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait: TCP: Waiting for source to send data... calling select\n",
			p->mythreadnum);
	}
	status = select(p->e2e_nd, &p->e2e_readset, NULL, NULL, NULL);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
	 * client sockets. We first check to see if the sd is in the readset.
	 * If so, this means that a client is trying to make a new connection
	 * in which case we need to issue an accept to establish the connection
	 * and obtain a new Client Socket Descriptor (csd).
	 */
	if (FD_ISSET(p->e2e_sd, &p->e2e_readset)) { /* Process an incoming connection */
		p->e2e_current_csd = p->e2e_next_csd;

		p->e2e_csd[p->e2e_current_csd] = accept(p->e2e_sd, (struct sockaddr *)&p->e2e_rname,&p->e2e_rnamelen);

		FD_SET(p->e2e_csd[p->e2e_current_csd], &p->e2e_active); /* Mark this fd as active */
		FD_SET(p->e2e_csd[p->e2e_current_csd], &p->e2e_readset); /* Put in readset so that it gets processed */

		/* Find the next available csd close to the beginning of the CSD array */
		p->e2e_next_csd = 0;
		while (p->e2e_csd[p->e2e_next_csd] != 0) {
			p->e2e_next_csd++;
			if (p->e2e_next_csd == FD_SETSIZE) {
				p->e2e_next_csd = 0;
				fprintf(xgp->errout,"%s: xdd_e2e_dest_wait: no csd entries left\n",xgp->progname);
				break;
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	/* This section will check to see which of the Client Socket Descriptors
	 * are in the readset. For those csd's that are ready, a recv is issued to 
	 * receive the incoming data. The clock is then read from pclk() and the
	 * new clock value is sent back to the client.
	 */
	for (p->e2e_current_csd = 0; p->e2e_current_csd < FD_SETSIZE; p->e2e_current_csd++) { // Process all CSDs that are ready
		if (FD_ISSET(p->e2e_csd[p->e2e_current_csd], &p->e2e_readset)) { /* Process this csd */
			/* Receive the destination's current location and length.
		 	 * When the destination closes the socket we get a read
		 	 * indication.  Treat any short send or receive as
		 	 * a failed connection and silently clean up.
		 	 */
			rcvd = 0;
			while (rcvd < p->e2e_iosize) {
				recvsize = (p->e2e_iosize-rcvd) > maxmit ? maxmit : (p->e2e_iosize-rcvd);
				status = recvfrom(p->e2e_csd[p->e2e_current_csd], (char *) p->rwbuf+rcvd, recvsize, MSG_WAITALL,NULL,NULL);
				if (status < 0) { 
					fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_dest_wait: error on recvfrom, status=%d\n",
						xgp->progname,
						p->mythreadnum,
						status);
					break;
				} else if (status == 0) 
					break;
				rcvd += status;
				status = 0;
			} // End of WHILE loop that received incoming data from the source machine

			if (status == 0) {
				/* copy meta data into destinations e2e_msg struct */
				memcpy(&p->e2e_msg, p->rwbuf+p->iosize, sizeof(p->e2e_msg));
				if (xgp->global_options & GO_DEBUG) {
					fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait: Successful Receive message %d, seq# %lld, len %lld, loc %lld, magic %08x e2e_iosize %d\n",
						p->mythreadnum,
						p->e2e_msg_recv,
						p->e2e_msg.sequence,
						p->e2e_msg.length,
						p->e2e_msg.location,
						p->e2e_msg.magic, 
						p->e2e_iosize);
				}
			}
			if (status < 0) {
		  		// At this point, if recv returned an error the connection
	  	 	 	// was most likely closed and we need to clear out this csd 
	  	 	 	// "Deactivate" the socket. 
	  			FD_CLR(p->e2e_csd[p->e2e_current_csd], &p->e2e_active);
	  			(void) closesocket(p->e2e_csd[p->e2e_current_csd]);
	  			p->e2e_csd[p->e2e_current_csd] = 0;
  	  			return(FAILED);
			} // End of processing a bad receive event

			// At this point the receive was good so lets proceed
			// Time stamp and normalize it to the global clock
	 		pclk_now(&p->e2e_msg.recvtime);
			if (!p->e2e_wait_1st_msg) 
				p->e2e_wait_1st_msg = p->e2e_msg.recvtime - e2e_wait_1st_msg_start_time;
			p->e2e_msg.recvtime += xgp->gts_delta;
			p->e2e_msg_recv++;

			// Received data but let's check to see what we actually got...
			if ((p->e2e_msg.magic != PTDS_E2E_MAGIC) && 
			    (p->e2e_msg.magic != PTDS_E2E_MAGIQ)) { // Magic numbers don't match!!!
					fprintf(xgp->errout,"%s: [mythreadnum %d]: xdd_e2e_dest_wait: Bad magic number 0x%08x on recv %d\n",
						xgp->progname,
						p->mythreadnum,
						p->e2e_msg.magic, 
						p->e2e_msg_recv);
					return(FAILED);
			}
			if ((p->e2e_msg.recvtime < p->e2e_msg.sendtime) &&
			    (xgp->gts_time > 0) ) { // Send and recv times look strange!!!
				fprintf(xgp->errout,"%s: [mythreadnum %d]: xdd_e2e_dest_wait: possible msg %lld recv time before send time by %llu picoseconds\n",
					xgp->progname,
					p->mythreadnum,
					p->e2e_msg.sequence,
					p->e2e_msg.sendtime-p->e2e_msg.recvtime);
				return(FAILED);
			}

			return(SUCCESS);

		} // End of IF stmnt that processes a CSD 

	} // End of FOR loop that processes all CSDs that were ready

	p->e2e_readset = p->e2e_active;  /* Prepare for the next select */

	return(SUCCESS);

} /* end of xdd_e2e_dest_wait() */

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_setup_src_socket() - set up the source side
 *
 */
int32_t
xdd_e2e_setup_src_socket(ptds_t *p) {
	int  	status; /* status of send/recv function calls */
	char 	msg[256];
	int 	type;
	char  	optionvalue; /* used to set the socket option */


	if (!p) { // Technically, this should *never* happen
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_src_socket: Internal error - no ptds pointer!\n",
			xgp->progname,
			p->mythreadnum);
		return(FAILED);
	}

	// Init the sockets - This is actually just for Windows that requires some additional initting
	status = xdd_e2e_sockets_init(); 
	if (status == FAILED) {
		xdd_e2e_err("xdd_e2e_setup_src_socket: could not initialize sockets for e2e source\n");
		return(FAILED);
	}

	if(p->e2e_dest_hostname == NULL) {
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_src_socket: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
			xgp->progname,
			p->mythreadnum);
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_src_socket: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
			xgp->progname,
			p->mythreadnum);
		return(FAILED);
	}

	/* Get the IP address of the destination host */
	p->e2e_dest_hostent = gethostbyname(p->e2e_dest_hostname);
	if (!(p->e2e_dest_hostent)) {
		sprintf(msg,"[mythreadnum %d]:xdd_e2e_setup_src_socket: Unable to identify the destination host <%s>for this e2e operation.",p->mythreadnum, p->e2e_dest_hostname);
		xdd_e2e_gethost_error(msg);
		return(FAILED);
	}

	p->e2e_dest_addr = ntohl(((struct in_addr *) p->e2e_dest_hostent->h_addr)->s_addr);
	p->e2e_dest_port += p->mythreadnum; // Add the thread number to the base port to handle queuedepths > 1.

	// This section is used when we are using TCP 
	// Set the "type" of socket being requested: for TCP, type=SOCK_STREAM, for UDP, type=SOCK_DGRAM
	type = SOCK_STREAM;

	// Create the socket 
	p->e2e_sd = socket(AF_INET, type, IPPROTO_TCP);
	if (p->e2e_sd < 0) {
		xdd_e2e_err("error opening socket\n");
		return(FAILED);
	}
	sprintf(msg,"[mythreadnum %d]:xdd_e2e_setup_src_socket: Just created socket, getting ready to bind",p->mythreadnum);
	(void) xdd_e2e_prt_socket_opts (msg,p->e2e_sd);
	(void) xdd_e2e_set_socket_opts (msg,p->e2e_sd);
	(void) xdd_e2e_prt_socket_opts (msg,p->e2e_sd);

	/* Now build the "name" of the DESTINATION machine socket thingy and connect to it. */
	(void) memset(&p->e2e_sname, 0, sizeof(p->e2e_sname));
	p->e2e_sname.sin_family = AF_INET;
	p->e2e_sname.sin_addr.s_addr = htonl(p->e2e_dest_addr);
	p->e2e_sname.sin_port = htons(p->e2e_dest_port);
	p->e2e_snamelen = sizeof(p->e2e_sname);
	if ((xgp->global_options & GO_DEBUG)) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_setup_src_socket: about to connect to Destination Hostname %s\tSocket address is 0x%x = %s\tPort %d <0x%x>, %s\n", 
			p->mythreadnum,
			p->e2e_dest_hostname,
			p->e2e_sname.sin_addr.s_addr, 
			inet_ntoa(p->e2e_sname.sin_addr),
			p->e2e_sname.sin_port,
			p->e2e_sname.sin_port,
			msg);
	}

	// Connecting to the server....
	if (type == SOCK_STREAM) {
		status = connect(p->e2e_sd, (struct sockaddr *) &p->e2e_sname, sizeof(p->e2e_sname));
		if (status) {
			xdd_e2e_err("xdd_e2e_setup_src_socket: error connecting to socket for E2E destination\n");
			return(FAILED);
		}
	}
	if (xgp->global_options & GO_DEBUG) {
		if (type == SOCK_STREAM) 
			strcpy(msg,"TCP:SOCK_STREAM");
		else	strcpy(msg,"UDP:SOCK_DGRAM");
		fprintf(xgp->errout,"%s: [mythreadnum %d]:xdd_e2e_setup_src_socket: Destination Hostname is %s\tSocket address is 0x%x = %s Port %d <0x%x>, %s\n", 
			xgp->progname,
			p->mythreadnum,
			p->e2e_dest_hostname,
			p->e2e_sname.sin_addr.s_addr, 
			inet_ntoa(p->e2e_sname.sin_addr),
			p->e2e_sname.sin_port,
			p->e2e_sname.sin_port,
			msg);
	}
	// Convert this back so that we retain the correct information to use with send/recv
	p->e2e_dest_addr = ntohl(p->e2e_sname.sin_addr.s_addr);
	p->e2e_dest_port = ntohs(p->e2e_sname.sin_port);

	return(SUCCESS);

} /* end of xdd_e2e_setup_src_socket() */
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_src_init() - init the source side 
 * This routine is called from the xdd io thread before all the action 
 * starts. When calling this routine, it is because this thread is on the
 * "source" side of an End-to-End test. Hence, this routine needs
 * to set up a socket on the appropriate port 
 *
 */
int32_t
xdd_e2e_src_init(ptds_t *p) {
	int  status; /* status of various function calls */


	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_src_init: Entering\n",p->mythreadnum);
	}

	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_src_init: Calling xdd_e2e_setup_src_socket\n",p->mythreadnum);
	}

	// Check to make sure that the source target is actually *reading* the data from a file or device
	if (p->rwratio < 1.0) { // Something is wrong - the source file/device is not 100% read
		fprintf(xgp->errout,"%s: xdd_e2e_src_init: Error - E2E source target[%d] file/device '%s' is not being *read*: rwratio=%5.2f\n",
			xgp->progname,
			p->my_target_number,
			p->target,
			p->rwratio);
		return(FAILED);
	}

	status = xdd_e2e_setup_src_socket(p);
	if (status == FAILED){
		xdd_e2e_err("xdd_e2e_src_init: could not setup sockets for e2e source\n");
		return(FAILED);
	}
	// Init the relevant variables in the ptds
	p->e2e_msg_sent = 0;
	p->e2e_msg_last_sequence = 0;
	// Init the message header
	p->e2e_msg.sequence = 0;
	p->e2e_msg.sendtime = 0;
	p->e2e_msg.recvtime = 0;
	p->e2e_msg.location = 0;
	p->e2e_msg.sendqnum = 0;

	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_src_init: Exiting\n",p->mythreadnum);
	}

	return(SUCCESS);

} /* end of xdd_e2e_src_init() */

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_src_send_msg() - send the data from the source to
 *      the destination 
 */
int32_t
xdd_e2e_src_send_msg(ptds_t *p) {
	int  	status; /* status of various function calls */
	int 	j;
	int 	sent;
	int	sendsize; 
	int	maxmit;


	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_src_send_msg: sending message %d, seq# %lld, len %lld, loc %lld, magic %08x...",
			p->mythreadnum,
			p->e2e_msg_sent,
			p->e2e_msg.sequence,
			p->e2e_msg.length,
			p->e2e_msg.location,
			p->e2e_msg.magic);
	}
	p->e2e_msg.sendqnum = p->mythreadnum;
	pclk_now(&p->e2e_msg.sendtime);
	p->e2e_msg.sendtime += xgp->gts_delta;

	/* copy meta data at end of data */
	memcpy(p->rwbuf+p->iosize,&p->e2e_msg, sizeof(p->e2e_msg));

	maxmit = p->e2e_useUDP ? MAXMIT_UDP : MAXMIT_TCP;
	sent = 0;
	while (sent < p->e2e_iosize) {
		sendsize = (p->e2e_iosize-sent) > maxmit ? maxmit : (p->e2e_iosize-sent);
		if (xgp->global_options & GO_DEBUG) {
			fprintf(xgp->errout,"e2e_iosize=%d, maxmit=%d, sendsize=%d, sent=%d...",
				p->e2e_iosize, maxmit, sendsize, sent);
		}
		status = sendto(p->e2e_sd,(char *)p->rwbuf+sent, sendsize, 0, (struct sockaddr *)&p->e2e_sname, sizeof(struct sockaddr_in));
		sent += status;
		if (xgp->global_options & GO_DEBUG) {
			fprintf(xgp->errout,"sent %d, status=%d\n",
				sent, status);
		}
	}

	if (sent != p->e2e_iosize) {
		xdd_e2e_err("xdd_e2e_src_send_msg: could not send message from e2e source\n");
		return(FAILED);
	}


	p->e2e_msg_sent++;
	p->e2e_msg.sequence++;

	return(SUCCESS);

} /* end of xdd_e2e_src_send_msg() */
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_dest_wait_UDP() - This is the Destination Wait using UDP
 * This routine is called only by xdd_e2e_dest_wait() which handles the 
 * TCP case.
 */
int32_t
xdd_e2e_dest_wait_UDP(ptds_t *p) {
	int status; /* status of send/recv function calls */
	int rcvd, recvsize, maxmit, got_e2e_msg;
	pclk_t e2e_wait_1st_msg_start_time;
	char ack[8];

	if (!p->e2e_wait_1st_msg) 
		pclk_now(&e2e_wait_1st_msg_start_time);

	maxmit = MAXMIT_UDP;

	if (xgp->global_options & GO_DEBUG ) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait_UDP:waiting for source to send data\n",p->mythreadnum);
	}

	got_e2e_msg = 0;
	p->e2e_timedout = 0;
	rcvd = 0;
	while (rcvd < p->e2e_iosize) {
		recvsize = (p->e2e_iosize-rcvd) > maxmit ? maxmit : (p->e2e_iosize-rcvd);
		status = recvfrom(p->e2e_sd, (char *) p->rwbuf+rcvd, recvsize, 0,(struct sockaddr *)&p->e2e_rname,&p->e2e_rnamelen);
		/* ---------------------------------------------------------- */
		/* recover from lost dgrams, i.e., timeout gracefully waiting */
		/* ---------------------------------------------------------- */
		if (status == -1 && p->e2e_wait_1st_msg > 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait_UDP: status=%d - timedout\n",p->mythreadnum,status);
			p->e2e_timedout = 1;
			strcpy(ack,"GOFORIT");
			if ( sendto(p->e2e_sd, ack, sizeof(ack), 0, (struct sockaddr *)&p->e2e_rname, p->e2e_rnamelen) < 0) {
				fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait_UDP: error sending GOFORIT signal to source\n",p->mythreadnum);
			}
  			return(-1);
		}
		/* ---------------------------------------------------------------- */
		/* last segment received out-of-order...retrieve e2e_msg            */
		/* only works for sure if last segment smaller than other segments, */
		/* but we will refrain from building a reliable layer around udp    */
		/* ---------------------------------------------------------------- */
		if (status < recvsize) {
			memcpy(&p->e2e_msg, p->rwbuf+rcvd+status-sizeof(p->e2e_msg), sizeof(p->e2e_msg));
			got_e2e_msg = 1;
		}
		rcvd += status;
	} // End of WHILE loop that receives data

	if (!got_e2e_msg)
		memcpy(&p->e2e_msg, p->rwbuf+p->iosize, sizeof(p->e2e_msg));

	if (xgp->global_options & GO_DEBUG)
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait_UDP:just finished receiving data from source: bytes rcvd=%d\n",p->mythreadnum,rcvd);

	/* copy meta data into destinations e2e_msg struct */
	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:xdd_e2e_dest_wait_UDP message %d, seq# %lld, len %ld, loc %ld, magic %08x sent %d\n",
			p->mythreadnum,
			p->e2e_msg_recv,
			p->e2e_msg.sequence,
			p->e2e_msg.length,
			p->e2e_msg.location,
			p->e2e_msg.magic, 
			p->e2e_iosize);
	}

	p->e2e_msg_recv++;
	pclk_now(&p->e2e_msg.recvtime);
	if (!p->e2e_wait_1st_msg) 
		p->e2e_wait_1st_msg = p->e2e_msg.recvtime - e2e_wait_1st_msg_start_time;
	p->e2e_msg.recvtime += xgp->gts_delta;
	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"[mythreadnum %d]:msg loc %lld, length %lld seq num is %d\n",
			p->mythreadnum, 
			p->e2e_msg.location,  
			p->e2e_msg.length, 
			p->e2e_msg.sequence);
	}

	/* Successful receive  - check to see if the data in the buffer is correct */
	if ((p->e2e_msg.magic != PTDS_E2E_MAGIC) && 
	    (p->e2e_msg.magic != PTDS_E2E_MAGIQ) || 
	    (p->e2e_msg.recvtime < p->e2e_msg.sendtime && xgp->gts_time > 0) ) {
		fprintf(xgp->errout,"xdd_e2e_dest_wait_UDP possible Bad magic number %08x on recv %d\n",
			p->e2e_msg.magic, 
			p->e2e_msg_recv);
		fprintf(xgp->errout,"xdd_e2e_dest_wait_UDP possible msg %lld recv time before send time by %llu picoseconds\n",
			p->e2e_msg.sequence,
			p->e2e_msg.sendtime-p->e2e_msg.recvtime);
		/* At this point, a recv returned an error in which case the connection
 		 * was most likely closed and we need to clear out this csd */
		/*"Deactivate" the socket. */
		close(p->e2e_sd);
			return(FAILED);
	}
	return(SUCCESS);
} // End of e2e_dest_wait_UDP() 
 
