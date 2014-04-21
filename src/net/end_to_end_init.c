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
 * This file contains the subroutines necessary to perform initialization
 * for the end-to-end option.
 */
#include "xint.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_target_init() - init socket library
 * This routine is called during target initialization to initialize the
 * socket library.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_target_init(target_data_t *tdp) {
	xint_restart_t	*rp;	// pointer to a restart structure
	int status;

	// Perform XNI initialization if required
	xdd_plan_t *planp = tdp->td_planp;
	if (PLAN_ENABLE_XNI & planp->plan_options) {
		xint_e2e_xni_init(tdp);
	}
	else {
	
		// Init the sockets - This is actually just for Windows that requires some additional initting
		status = xdd_sockets_init();
		if (status == -1) {
			fprintf(xgp->errout,"%s: xdd_e2e_target_init: could not initialize sockets for e2e target\n",xgp->progname);
			return(-1);
		}
	}
	
	// Restart processing if necessary
	if ((tdp->td_target_options & TO_RESTART_ENABLE) && (tdp->td_restartp)) { // Check to see if restart was requested
		// Set the last_committed_byte_offset to 0
		rp = tdp->td_restartp;
		rp->last_committed_byte_offset = rp->byte_offset;
		rp->last_committed_length = 0;
	}

	return(0);
}

/*----------------------------------------------------------------------*/
/* xdd_e2e_worker_init() - init source and destination sides
 * This routine is called during Worker Thread initialization to initialize
 * a source or destination Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_worker_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	int status;
	in_addr_t addr;

	tdp = wdp->wd_tdp;
	wdp->wd_e2ep->e2e_sr_time = 0;

	if(wdp->wd_e2ep->e2e_dest_hostname == NULL) {
		fprintf(xgp->errout,"%s: xdd_e2e_worker_init: Target %d Worker Thread %d: No DESTINATION host name or IP address specified for this end-to-end operation.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		fprintf(xgp->errout,"%s: xdd_e2e_worker_init: Target %d Worker Thread %d: Use the '-e2e destination' option to specify the DESTINATION host name or IP address.\n",
				xgp->progname,
				tdp->td_target_number,
				wdp->wd_worker_number);
		return(-1);
	}

	// Get the IP address of the destination host
	status = xint_lookup_addr(wdp->wd_e2ep->e2e_dest_hostname, 0, &addr);
	if (status) {
		fprintf(xgp->errout, "%s: xdd_e2e_worker_init: unable to identify host '%s'\n",
				xgp->progname, wdp->wd_e2ep->e2e_dest_hostname);
		return(-1);
	}

	// Convert to host byte order
	wdp->wd_e2ep->e2e_dest_addr = ntohl(addr);

	if (tdp->td_target_options & TO_E2E_DESTINATION) { // This is the Destination side of an End-to-End
		status = xdd_e2e_dest_init(wdp);
	} else if (tdp->td_target_options & TO_E2E_SOURCE) { // This is the Source side of an End-to-End
		status = xdd_e2e_src_init(wdp);
	} else { // Should never reach this point
		status = -1;
	}

	return status;
} // xdd_e2e_worker_init()

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
xdd_e2e_src_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	xint_e2e_t		*e2ep;		// Pointer to the E2E data struct
	int  		status; // status of various function calls 


	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;
	
	// Check to make sure that the source target is actually *reading* the data from a file or device
	if (tdp->td_rwratio < 1.0) { // Something is wrong - the source file/device is not 100% read
		fprintf(xgp->errout,"%s: xdd_e2e_src_init: Target %d Worker Thread %d: Error - E2E source file '%s' is not being *read*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			tdp->td_target_full_pathname,
			tdp->td_rwratio);
		return(-1);
	}

	/* Only setup sockets if not using XNI */
	xdd_plan_t *planp = tdp->td_planp;
	if (!(PLAN_ENABLE_XNI & planp->plan_options)) {
		status = xdd_e2e_setup_src_socket(wdp);
		if (status == -1){
			xdd_e2e_err(wdp,"xdd_e2e_src_init","could not setup sockets for e2e source\n");
			return(-1);
		}
	}
	
	// Init the relevant variables 
	e2ep->e2e_msg_sent = 0;
	e2ep->e2e_msg_sequence_number = 0;
	e2ep->e2e_header_size = (int)(sizeof(xdd_e2e_header_t));

	// Init the message header
 	// For End-to-End operations, the buffer pointers are as follows:
    //	+----------------+-----------------------------------------------------+
    //	|<----1 page---->|  transfer size (td_xfer_size) rounded up to N pages |
    //	|<-task_bufp     |<-task_datap                                         |
    //	|     |          |                                                     |
    //	|     |<-Header->|  E2E data buffer                                    |
    //	+-----*----------*-----------------------------------------------------+
    //	      ^          ^
    //	      ^          +-e2e_datap 
    //	      +-e2e_hdrp 
	//
	e2ep->e2e_hdrp->e2eh_worker_thread_number = 0;
	e2ep->e2e_hdrp->e2eh_sequence_number = 0;
	e2ep->e2e_hdrp->e2eh_send_time = 0;
	e2ep->e2e_hdrp->e2eh_recv_time = 0;
	e2ep->e2e_hdrp->e2eh_byte_offset = 0;
	e2ep->e2e_hdrp->e2eh_data_length = 0;


	return(0);

} /* end of xdd_e2e_src_init() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_setup_src_socket() - set up the source side
 * This subroutine is called by xdd_e2e_src_init() and is passed a
 * pointer to the Data Struct of the requesting Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_setup_src_socket(worker_data_t *wdp) {
	int  	status; /* status of send/recv function calls */
	int 	type;
	static const int connect_try_limit = 4;
	int i;
	
	// The socket type is SOCK_STREAM because this is a TCP connection 
	type = SOCK_STREAM;

	// Create the socket 
	wdp->wd_e2ep->e2e_sd = socket(AF_INET, type, IPPROTO_TCP);
	if (wdp->wd_e2ep->e2e_sd < 0) {
		xdd_e2e_err(wdp,"xdd_e2e_setup_src_socket","ERROR: error openning socket\n");
		return(-1);
	}
	(void) xdd_e2e_set_socket_opts (wdp,wdp->wd_e2ep->e2e_sd);

	/* Now build the "name" of the DESTINATION machine socket thingy and connect to it. */
	(void) memset(&wdp->wd_e2ep->e2e_sname, 0, sizeof(wdp->wd_e2ep->e2e_sname));
	wdp->wd_e2ep->e2e_sname.sin_family = AF_INET;
	wdp->wd_e2ep->e2e_sname.sin_addr.s_addr = htonl(wdp->wd_e2ep->e2e_dest_addr);
	wdp->wd_e2ep->e2e_sname.sin_port = htons(wdp->wd_e2ep->e2e_dest_port);
	wdp->wd_e2ep->e2e_snamelen = sizeof(wdp->wd_e2ep->e2e_sname);

	// Attempt to connect to the server for roughly 10 seconds
	i = 0;
	status = 1;
	while (i < connect_try_limit && 0 != status) {

	    /* If this is a retry, sleep for 3 seconds before retrying */
	    if (i > 0) {
		struct timespec req;
		memset(&req, 0, sizeof(req));
		req.tv_sec = 3;
		fprintf(xgp->errout,
			"Socket connection error, retrying in %d seconds: %d\n",
			(int)req.tv_sec, status);
		nanosleep(&req, (struct timespec *)NULL);
	    }
	    
	    status = connect(wdp->wd_e2ep->e2e_sd,
			     (struct sockaddr *) &wdp->wd_e2ep->e2e_sname,
			     sizeof(wdp->wd_e2ep->e2e_sname));
	    i++;
	}
	
	if (0 != status) {
	    xdd_e2e_err(wdp,"xdd_e2e_setup_src_socket","error connecting to socket for E2E destination\n");
	    return(-1);
	}

	return(0);

} /* end of xdd_e2e_setup_src_socket() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_dest_init() - init the destination side 
 * This routine is called by a Worker Thread on the "destination" side of an
 * end_to_end operation and is passed a pointer to the Data Struct of the 
 * requesting Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_dest_init(worker_data_t *wdp) {
	target_data_t	*tdp;
	int  		status; // status of various function calls 


	tdp = wdp->wd_tdp;
	// Check to make sure that the destination target is actually *writing* the data it receives to a file or device
	if (tdp->td_rwratio > 0.0) { // Something is wrong - the destination file/device is not 100% write
		fprintf(xgp->errout,"%s: xdd_e2e_dest_init: Target %d Worker Thread %d: Error - E2E destination file/device '%s' is not being *written*: rwratio=%5.2f is not valid\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number,
			tdp->td_target_full_pathname,
			tdp->td_rwratio);
		return(-1);
	}
	
	/* Only setup sockets if not using XNI */
	xdd_plan_t *planp = tdp->td_planp;
	if (!(PLAN_ENABLE_XNI & planp->plan_options)) {
		status = xdd_e2e_setup_dest_socket(wdp);
		if (status == -1){
			xdd_e2e_err(wdp,"xdd_e2e_dest_init","could not setup sockets for e2e destination\n");
			return(-1);
		}

		// Set up the descriptor table for the select() call
		// This section is used when we are using TCP 
		/* clear out the csd table */
		for (wdp->wd_e2ep->e2e_current_csd = 0; wdp->wd_e2ep->e2e_current_csd < FD_SETSIZE; wdp->wd_e2ep->e2e_current_csd++)
			wdp->wd_e2ep->e2e_csd[wdp->wd_e2ep->e2e_current_csd] = 0;

		// Set the current and next csd indices to 0
		wdp->wd_e2ep->e2e_current_csd = wdp->wd_e2ep->e2e_next_csd = 0;

		/* Initialize the socket sets for select() */
		FD_ZERO(&wdp->wd_e2ep->e2e_readset);
		FD_SET(wdp->wd_e2ep->e2e_sd, &wdp->wd_e2ep->e2e_readset);
		wdp->wd_e2ep->e2e_active = wdp->wd_e2ep->e2e_readset;
		wdp->wd_e2ep->e2e_current_csd = wdp->wd_e2ep->e2e_next_csd = 0;

		/* Find out how many sockets are in each set */
		wdp->wd_e2ep->e2e_nd = FD_SETSIZE;
	}
	
	// Initialize the message counter and sequencer to 0
	wdp->wd_e2ep->e2e_msg_recv = 0;
	wdp->wd_e2ep->e2e_msg_sequence_number = 0;

	return(0);

} /* end of xdd_e2e_dest_init() */

/*----------------------------------------------------------------------*/
/* xdd_e2e_setup_dest_socket() - Set up the socket on the Destination side
 * This subroutine is called by xdd_e2e_dest_init() and is passed a
 * pointer to the Data Struct of the requesting Worker Thread.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t
xdd_e2e_setup_dest_socket(worker_data_t *wdp) {
	int  	status;
	int 	type;
	char 	msg[256];


	// Set the "type" of socket being requested: for TCP, type=SOCK_STREAM 
	type = SOCK_STREAM;

	// Create the socket 
	wdp->wd_e2ep->e2e_sd = socket(AF_INET, type, IPPROTO_TCP);
	if (wdp->wd_e2ep->e2e_sd < 0) {
		xdd_e2e_err(wdp,"xdd_e2e_setup_dest_socket","ERROR: error openning socket\n");
		return(-1);
	}

	(void) xdd_e2e_set_socket_opts (wdp, wdp->wd_e2ep->e2e_sd);

	/* Bind the name to the socket */
	(void) memset(&wdp->wd_e2ep->e2e_sname, 0, sizeof(wdp->wd_e2ep->e2e_sname));
	wdp->wd_e2ep->e2e_sname.sin_family = AF_INET;
	wdp->wd_e2ep->e2e_sname.sin_addr.s_addr = htonl(wdp->wd_e2ep->e2e_dest_addr);
	wdp->wd_e2ep->e2e_sname.sin_port = htons(wdp->wd_e2ep->e2e_dest_port);
	wdp->wd_e2ep->e2e_snamelen = sizeof(wdp->wd_e2ep->e2e_sname);
	if (bind(wdp->wd_e2ep->e2e_sd, (struct sockaddr *) &wdp->wd_e2ep->e2e_sname, wdp->wd_e2ep->e2e_snamelen)) {
		sprintf(msg,"Error binding name to socket - addr=0x%08x, port=0x%08x, specified as %d \n",
			wdp->wd_e2ep->e2e_sname.sin_addr.s_addr, 
			wdp->wd_e2ep->e2e_sname.sin_port,
			wdp->wd_e2ep->e2e_dest_port);
		xdd_e2e_err(wdp,"xdd_e2e_setup_dest_socket",msg);
		return(-1);
	}

	/* All set; prepare to accept connection requests */
	if (type == SOCK_STREAM) { // If this is a stream socket then we need to listen for incoming data
		status = listen(wdp->wd_e2ep->e2e_sd, SOMAXCONN);
		if (status) {
			xdd_e2e_err(wdp,"xdd_e2e_setup_dest_socket","ERROR: bad status starting LISTEN on socket\n");
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
xdd_e2e_set_socket_opts(worker_data_t *wdp, int skt) {
	target_data_t	*tdp;
	int status;
	int level = SOL_SOCKET;
	xdd_plan_t* planp = wdp->wd_tdp->td_planp;
	
#if WIN32
	char  optionvalue;
#else
	int  optionvalue;
#endif

	tdp = wdp->wd_tdp;
	/* Create the socket and set some options */
	optionvalue = 1;
	status = setsockopt(skt, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
	if (status != 0) {
		xdd_e2e_err(wdp,"xdd_e2e_set_socket_opts","Error setting TCP_NODELAY \n");
	}
	status = setsockopt(skt,level,SO_SNDBUF,(char *)&planp->e2e_TCP_Win,sizeof(planp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d Worker Thread %d: WARNING: on setsockopt SO_SNDBUF: status %d: %s\n", 
			xgp->progname, 
			tdp->td_target_number, 
			wdp->wd_worker_number, 
			status, 
			strerror(errno));
	}
	status = setsockopt(skt,level,SO_RCVBUF,(char *)&planp->e2e_TCP_Win,sizeof(planp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d Worker Thread %d: WARNING: on setsockopt SO_RCVBUF: status %d: %s\n", 
			xgp->progname, 
			tdp->td_target_number, 
			wdp->wd_worker_number, 
			status, 
			strerror(errno));
	}
	status = setsockopt(skt,level,SO_REUSEADDR,(char *)&planp->e2e_TCP_Win,sizeof(planp->e2e_TCP_Win));
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_e2e_set_socket_opts: Target %d Worker Thread %d: WARNING: on setsockopt SO_REUSEPORT: status %d: %s\n", 
			xgp->progname, 
			tdp->td_target_number, 
			wdp->wd_worker_number, 
			status, 
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
xdd_e2e_prt_socket_opts(int skt) {
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
xdd_e2e_err(worker_data_t *wdp, char const *whence, char const *fmt, ...) {
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
	fprintf(xgp->errout,"\n%s: %s: Target %d Worker Thread %d: ",
		xgp->progname,
		whence,
		wdp->wd_tdp->td_target_number,
		wdp->wd_worker_number);
	fprintf(xgp->errout, "%s", fmt);
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
int32_t xdd_sockets_init(void) {
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

/*----------------------------------------------------------------------------*/
/* xdd_get_e2ep() - return a pointer to the xdd_e2e Data Structure 
 */
xint_e2e_t *
xdd_get_e2ep(void) {
	xint_e2e_t	*e2ep;
	
	e2ep = malloc(sizeof(xint_e2e_t));
	if (e2ep == NULL) {
		fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for E2E data structure \n",
		xgp->progname, (int)sizeof(xint_e2e_t));
		return(NULL);
	}
	memset(e2ep, 0, sizeof(xint_e2e_t));

	return(e2ep);
} /* End of xdd_get_e2ep() */

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
