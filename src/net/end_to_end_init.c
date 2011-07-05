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
 * This file contains the subroutines necessary to perform initialization
 * for the end-to-end option.
 */
#include "xdd.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_target_init() - init socket library
 * This routine is called during target initialization to initialize the
 * socket library.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_target_init(ptds_t *p)
{
	int status;

	// Init the sockets - This is actually just for Windows that requires some additional initting
	status = xdd_sockets_init();
	if (status == -1) {
		xdd_e2e_err(p,"xdd_e2e_target_init","could not initialize sockets for e2e target\n");
		return(-1);
	}

	return(0);
}

/*----------------------------------------------------------------------*/
/* xdd_e2e_qthread_init() - init source and destination sides
 * This routine is called during QThread initialization to initialize
 * a source or destination QThread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_qthread_init(ptds_t *qp)
{
	int status;
	struct xdd_network_address addr;

	qp->e2e_sr_time = 0;

	if(qp->e2e_dest_hostname == NULL) {
		fprintf(xgp->errout,"%s: xdd_e2e_qthread_init: Target %d QThread %d: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number);
		fprintf(xgp->errout,"%s: xdd_e2e_qthread_init: Target %d QThread %d: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number);
		return(-1);
	}

	// Get the IP address of the destination host
	status = xdd_lookup_addr(qp->e2e_dest_hostname, qp->e2e_addrtype,
							 0, &addr);
	if (status) {
		fprintf(xgp->errout, "%s: xdd_e2e_qthread_init: unable to identify host '%s'\n",
				xgp->progname, qp->e2e_dest_hostname);
		return(-1);
	}

	qp->e2e_dest_addr = addr;

	if (qp->target_options & TO_E2E_DESTINATION) { // This is the Destination side of an End-to-End
		status = xdd_e2e_dest_init(qp);
	} else if (qp->target_options & TO_E2E_SOURCE) { // This is the Source side of an End-to-End
		status = xdd_e2e_src_init(qp);
	} else { // Should never reach this point
		status = -1;
	}

	return status;
}

/*----------------------------------------------------------------------*/
/* xdd_e2e_src_init() - init the source side 
 * This routine is called from the xdd io thread before all the action 
 * starts. When calling this routine, it is because this thread is on the
 * "source" side of an End-to-End test. Hence, this routine needs
 * to set up a socket on the appropriate port 
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_src_init(ptds_t *qp) {
	int  		status; // status of various function calls 
	restart_t	*rp;	// pointer to a restart structure


	// Check to make sure that the source target is actually *reading* the data from a file or device
	if (qp->rwratio < 1.0) { // Something is wrong - the source file/device is not 100% read
		fprintf(xgp->errout,"%s: xdd_e2e_src_init: Target %d QThread %d: Error - E2E source file '%s' is not being *read*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname,
			qp->rwratio);
		return(-1);
	}

	status = xdd_e2e_setup_src_socket(qp);
	if (status == -1){
		xdd_e2e_err(qp,"xdd_e2e_src_init","could not setup sockets for e2e source\n");
		return(-1);
	}
	// Init the relevant variables in the ptds
	qp->e2e_msg_sent = 0;
	qp->e2e_msg_sequence_number = 0;
	// Init the message header
	qp->e2e_header.sequence = 0;
	qp->e2e_header.sendtime = 0;
	qp->e2e_header.recvtime = 0;
	qp->e2e_header.location = 0;
	qp->e2e_header.sendqnum = 0;

	// Restart processing if necessary
	if ((qp->target_options & TO_RESTART_ENABLE) && (qp->restartp)) { // Check to see if restart was requested
		// Set the last_committed_byte_location to 0
		rp = qp->restartp;
		rp->last_committed_byte_location = rp->byte_offset;
		rp->last_committed_length = 0;
	}

	return(0);

} /* end of xdd_e2e_src_init() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_setup_src_socket() - set up the source side
 * This subroutine is called by xdd_e2e_src_init() and is passed a
 * pointer to the PTDS of the requesting QThread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_setup_src_socket(ptds_t *qp) {
	int  	status; /* status of send/recv function calls */
	char 	msg[256];
	int 	domain, type;

	domain = (qp->e2e_dest_addr.type == XDD_ADDRESS_INET6 ? AF_INET6 : AF_INET);
	// The socket type is SOCK_STREAM because this is a TCP connection 
	type = SOCK_STREAM;

	// Create the socket 
	qp->e2e_sd = socket(domain, type, IPPROTO_TCP);
	if (qp->e2e_sd < 0) {
		xdd_e2e_err(qp,"xdd_e2e_setup_src_socket","ERROR: error openning socket\n");
		return(-1);
	}
	(void) xdd_e2e_set_socket_opts (qp,msg,qp->e2e_sd);

	/* Now build the "name" of the DESTINATION machine socket thingy and connect to it. */
	(void) memset(&qp->e2e_sname, 0, sizeof(qp->e2e_sname));
	if (domain == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)&qp->e2e_sname;

		sa6->sin6_family = AF_INET6;
		sa6->sin6_addr = qp->e2e_dest_addr.u.in6addr;
		sa6->sin6_port = htons(qp->e2e_dest_port);
		qp->e2e_snamelen = sizeof(*sa6);
	} else {  // AF_INET
		struct sockaddr_in *sa4 = (struct sockaddr_in*)&qp->e2e_sname;

		sa4->sin_family = AF_INET;
		sa4->sin_addr = qp->e2e_dest_addr.u.in4addr;
		sa4->sin_port = htons(qp->e2e_dest_port);
		qp->e2e_snamelen = sizeof(qp->e2e_sname);
	}

	// Connecting to the server....
	status = connect(qp->e2e_sd, (struct sockaddr *) &qp->e2e_sname, sizeof(qp->e2e_sname));
	if (status) {
		xdd_e2e_err(qp,"xdd_e2e_setup_src_socket","error connecting to socket for E2E destination\n");
		return(-1);
	}

	return(0);

} /* end of xdd_e2e_setup_src_socket() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_init() - init the destination side 
 * This routine is called by a QThread on the "destination" side of an
 * end_to_end operation and is passed a pointer to the PTDS of the 
 * requesting QThread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_dest_init(ptds_t *qp) {
	int  		status; // status of various function calls 
	restart_t	*rp;	// Pointer to a restart structure used by the restart_monitor()

	// Check to make sure that the destination target is actually *writing* the data it receives to a file or device
	if (qp->rwratio > 0.0) { // Something is wrong - the destination file/device is not 100% write
		fprintf(xgp->errout,"%s: xdd_e2e_dest_init: Target %d QThread %d: Error - E2E destination file/device '%s' is not being *written*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname,
			qp->rwratio);
		return(-1);
	}
	
	/* Set up the destination-side sockets */
	status = xdd_e2e_setup_dest_socket(qp);
	if (status == -1) {
		xdd_e2e_err(qp,"xdd_e2e_dest_init","could not init e2e destination socket\n");
		return(-1);
	}

	// Set up the descriptor table for the select() call
	// This section is used when we are using TCP 
	/* clear out the csd table */
	for (qp->e2e_current_csd = 0; qp->e2e_current_csd < FD_SETSIZE; qp->e2e_current_csd++)
		qp->e2e_csd[qp->e2e_current_csd] = 0;

	// Set the current and next csd indices to 0
	qp->e2e_current_csd = qp->e2e_next_csd = 0;

	/* Initialize the socket sets for select() */
	FD_ZERO(&qp->e2e_readset);
	FD_SET(qp->e2e_sd, &qp->e2e_readset);
	qp->e2e_active = qp->e2e_readset;
	qp->e2e_current_csd = qp->e2e_next_csd = 0;

	/* Find out how many sockets are in each set (berkely only) */
#if (IRIX || WIN32 )
	qp->e2e_nd = getdtablehi();
#endif
#if (LINUX || OSX)
	qp->e2e_nd = getdtablesize();
#endif
#if (AIX)
	qp->e2e_nd = FD_SETSIZE;
#endif
#if (SOLARIS )
	qp->e2e_nd = FD_SETSIZE;
#endif

	// Initialize the message counter and sequencer to 0
	qp->e2e_msg_recv = 0;
	qp->e2e_msg_sequence_number = 0;

	// Check to see if restart was requested
	if ((qp->target_options & TO_RESTART_ENABLE) && (qp->restartp)) { 
		// Set the last_committed_byte_location to 0
		rp = qp->restartp;
		rp->last_committed_byte_location = rp->byte_offset;
		rp->last_committed_length = 0;
	}

	return(0);

} /* end of xdd_e2e_dest_init() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_setup_dest_socket() - Set up the socket on the Destination side
 * This subroutine is called by xdd_e2e_dest_init() and is passed a
 * pointer to the PTDS of the requesting QThread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_setup_dest_socket(ptds_t *qp) {
	int  	status;
	char 	msg[256];
	char	prettyaddr[64];
	int 	domain, type;


	domain = (qp->e2e_dest_addr.type == XDD_ADDRESS_INET6 ? AF_INET6 : AF_INET);
	// Set the "type" of socket being requested: for TCP, type=SOCK_STREAM 
	type = SOCK_STREAM;

	// Create the socket 
	qp->e2e_sd = socket(domain, type, IPPROTO_TCP);
	if (qp->e2e_sd < 0) {
		xdd_e2e_err(qp,"xdd_e2e_setup_dest_socket","ERROR: error openning socket\n");
		return(-1);
	}

	(void) xdd_e2e_set_socket_opts (qp,msg,qp->e2e_sd);

	/* Bind the name to the socket */
	(void) memset(&qp->e2e_sname, 0, sizeof(qp->e2e_sname));
	if (domain == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)&qp->e2e_sname;

		sa6->sin6_family = AF_INET6;
		sa6->sin6_addr = qp->e2e_dest_addr.u.in6addr;
		sa6->sin6_port = htons(qp->e2e_dest_port);
		qp->e2e_snamelen = sizeof(*sa6);
		inet_ntop(AF_INET6, &sa6->sin6_addr, prettyaddr, 64);
	} else {  // AF_INET
		struct sockaddr_in *sa4 = (struct sockaddr_in*)&qp->e2e_sname;

		sa4->sin_family = AF_INET;
		sa4->sin_addr = qp->e2e_dest_addr.u.in4addr;
		sa4->sin_port = htons(qp->e2e_dest_port);
		qp->e2e_snamelen = sizeof(*sa4);
		inet_ntop(AF_INET, &sa4->sin_addr, prettyaddr, 64);
	}
	if (bind(qp->e2e_sd, (struct sockaddr *) &qp->e2e_sname, qp->e2e_snamelen)) {
		snprintf(msg, 256, "Error binding name to socket - addr=%s, port=%u\n",
				 prettyaddr,
				 (unsigned int)qp->e2e_dest_port);
		xdd_e2e_err(qp,"xdd_e2e_setup_dest_socket",msg);
		return(-1);
	}

	/* All set; prepare to accept connection requests */
	if (type == SOCK_STREAM) { // If this is a stream socket then we need to listen for incoming data
		status = listen(qp->e2e_sd, SOMAXCONN);
		if (status) {
			xdd_e2e_err(qp,"xdd_e2e_setup_dest_socket","ERROR: bad status starting LISTEN on socket\n");
			return(-1);
		}
	}

	return(0);

} /* end of xdd_e2e_setup_dest_socket() */

/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_set_socket_opts() - set the options for specified socket.
 *
 */
void 
xdd_e2e_set_socket_opts(ptds_t *qp, char *sktname, int skt) {
	int status;
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
		xdd_e2e_err(qp,"xdd_e2e_set_socket_opts","Error setting TCP_NODELAY \n");
	}
	status = setsockopt(skt,level,SO_SNDBUF,(char *)&xgp->e2e_TCP_Win,sizeof(xgp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d QThread %d: ERROR: on setsockopt SO_SNDBUF: status %d: %s\n", 
			xgp->progname, 
			qp->my_target_number, qp->my_qthread_number, status, 
			strerror(errno));
	}
	status = setsockopt(skt,level,SO_RCVBUF,(char *)&xgp->e2e_TCP_Win,sizeof(xgp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d QThread %d: ERROR: on setsockopt SO_RCVBUF: status %d: %s\n", 
			xgp->progname, 
			qp->my_target_number, qp->my_qthread_number, status, 
			strerror(errno));
	}
	status = setsockopt(skt,level,SO_REUSEADDR,(char *)&xgp->e2e_TCP_Win,sizeof(xgp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d QThread %d: ERROR: on setsockopt SO_REUSEPORT: status %d: %s\n", 
			xgp->progname, 
			qp->my_target_number, qp->my_qthread_number, status, 
			strerror(errno));
	}

} // End of xdd_e2e_set_socket_opts()
/*----------------------------------------------------------------------*/
/*
 * xdd_e2e_prt_socket_opts() - print the currnet options for specified 
 *   socket.
 *
 */
void 
xdd_e2e_prt_socket_opts(char *sktname, int skt) {
	int 		level = SOL_SOCKET;
	int 		sockbuf_sizs;
	int			sockbuf_sizr;
	int			reuse_addr;
	socklen_t	optlen;


	optlen = sizeof(sockbuf_sizs);
	getsockopt(skt,level,SO_SNDBUF,(char *)&sockbuf_sizs,&optlen);
	optlen = sizeof(sockbuf_sizr);
	getsockopt(skt,level,SO_RCVBUF,(char *)&sockbuf_sizr,&optlen);
	optlen = sizeof(reuse_addr);
	getsockopt(skt,level,SO_REUSEADDR,(char *)&reuse_addr,&optlen);
} // End of xdd_e2e_prt_socket_opts()

/*----------------------------------------------------------------------*/
/* xdd_e2e_err(fmt...)
 *
 * General-purpose error handling routine.  Prints the short message
 * provided to standard error, along with some boilerplate information
 * such as the program name and errno value.  Any remaining arguments
 * are used in printed message (the usage here takes the same form as
 * printf()).
 */
void
xdd_e2e_err(ptds_t *qp, char const *whence, char const *fmt, ...) {
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
	fprintf(xgp->errout,"\n%s: %s: Target %d QThread %d: ",
		xgp->progname,
		whence,
		qp->my_target_number,
		qp->my_qthread_number);
	fprintf(xgp->errout, fmt);
	perror(" Reason");
	return;
} /* end of xdd_e2e_err() */

/*----------------------------------------------------------------------*/
/* xdd_sockets_init() - Windows Only
 *
 * Windows requires the WinSock startup routine to be run
 * before running a bunch of socket routines.  We encapsulate
 * that here in case some other environment needs something similar.
 *
 * Return values: 0 is good - init successful, -1 is bad
 *
 * The sample code I based this on happened to be requesting
 * (and verifying) a WinSock DLL version 2.2 environment was
 * present, and it worked, so I kept it that way.
 */
int32_t
xdd_sockets_init(void) {
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
		return(-1);
	}
#endif

	return(0);

} /* end of xdd_sockets_init() */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
