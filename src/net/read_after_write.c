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
 * This file contains the subroutines necessary to perform a read-after-write
 * operation across two different machines running xdd.
 */
#include "xint.h"

#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE 128
#endif

/*----------------------------------------------------------------------*/
/*
 * xdd_raw_err(fmt...)
 *
 * General-purpose error handling routine.  Prints the short message
 * provided to standard error, along with some boilerplate information
 * such as the program name and errno value.  Any remaining arguments
 * are used in printed message (the usage here takes the same form as
 * printf()).
 */
void
xdd_raw_err(char const *fmt, ...) {
#ifdef ndef
#ifdef WIN32
	LPVOID lpMsgBuf;
#endif
    fputs(fmt,xgp->errout);
	perror("reason");
	return;
#ifdef WIN32 /* Windows environment, actually */
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
#endif // ndef
} /* end of raw_err() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_setup_reader_socket()
 *
 */
int32_t
xdd_raw_setup_reader_socket(target_data_t *tdp) {
#ifdef ndef
	xdd_raw_t	*rawp;


	rawp = p->rawp;
	/* Create the socket and set some options */
	if ((rawp->raw_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		xdd_raw_err("error opening socket");
		return(FALSE);
	}
    /* Bind the name to the socket */
	(void) memset(&rawp->raw_sname, 0, sizeof(rawp->raw_sname));
	rawp->raw_sname.sin_family = AF_INET;
	rawp->raw_sname.sin_addr.s_addr = htonl(rawp->raw_addr);
	rawp->raw_sname.sin_port = htons(rawp->raw_port);
	rawp->raw_snamelen = sizeof(rawp->raw_sname);
	if (bind(rawp->raw_sd, (struct sockaddr *) &rawp->raw_sname, rawp->raw_snamelen)) {
		xdd_raw_err("error binding name to socket");
		return(FALSE);
	}
	/* Get and display the name (in case "any" was used) */
	if (getsockname(rawp->raw_sd, (struct sockaddr *) &rawp->raw_sname, &rawp->raw_snamelen)) {
		xdd_raw_err("error getting socket name");
		return(FALSE);
	}
	rawp->raw_addr = ntohl(rawp->raw_sname.sin_addr.s_addr);
	rawp->raw_port = ntohs(rawp->raw_sname.sin_port);
	fprintf(stderr,"%s: Reader Hostname is %s\n\tSocket address is 0x%x = %s\n\tPort %d <0x%x>\n", 
		xgp->progname,
		rawp->raw_myhostname,
		rawp->raw_sname.sin_addr.s_addr, 
		inet_ntoa(rawp->raw_sname.sin_addr),
		rawp->raw_sname.sin_port,rawp->raw_port);
	/* All set; prepare to accept connection requests */
	if (listen(rawp->raw_sd, SOMAXCONN)) {
		xdd_raw_err("error starting listening on socket");
		return(FALSE);
	}
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_setup_reader_socket() */
/*----------------------------------------------------------------------*/
/* xdd_raw_sockets_init() - for read-after-write operations
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
xdd_raw_sockets_init(target_data_t *tdp) {
#ifdef ndef
	xdd_raw_t	*rawp;


	rawp = p->rawp;
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
		return(FALSE);
	}
#endif
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_sockets_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_reader_init() - init the reader side of a read-after-write
 *
 */
int32_t
xdd_raw_reader_init(target_data_t *tdp) {
#ifdef ndef
	int  status; /* status of various function calls */
	xdd_raw_t	*rawp;


	rawp = p->rawp;
	/* Set the target_options flag that identifies this target as a reader */
	p->target_options |= TO_RAW_READER;
	/* init the sockets */
	status = xdd_raw_sockets_init(p);
	if (status == FALSE) {
		xdd_raw_err("couldn't initialize sockets for RAW reader");
		return(FALSE);
	}
	/* Get the name of the machine that we are running on */
	status = gethostname(rawp->raw_myhostname, sizeof(rawp->raw_myhostname));
	if (status != 0 ) {
		xdd_raw_err("could not get hostname");
		return(FALSE);
	}
	/* Get the IP address of this host */
	rawp->raw_hostent = gethostbyname(rawp->raw_myhostname);
	if (! rawp->raw_hostent) {
		xdd_raw_err("unable to identify host");
		return(FALSE);
	}
	rawp->raw_addr = ntohl(((struct in_addr *) rawp->raw_hostent->h_addr)->s_addr);
	/* Set up the server sockets */
	status = xdd_raw_setup_reader_socket(p);
	if (status == FALSE) {
		xdd_raw_err("could not init raw reader socket");
		return(FALSE);
	}
	/* clear out the csd table */
	for (rawp->raw_current_csd = 0; rawp->raw_current_csd < FD_SETSIZE; rawp->raw_current_csd++)
		rawp->raw_csd[rawp->raw_current_csd] = 0;
	rawp->raw_current_csd = rawp->raw_next_csd = 0;
	/* Initialize the socket sets for select() */
    FD_ZERO(&rawp->raw_readset);
    FD_SET(rawp->raw_sd, &rawp->raw_readset);
	rawp->raw_active = rawp->raw_readset;
	rawp->raw_current_csd = rawp->raw_next_csd = 0;
	/* Find out how many sockets are in each set (berkely only) */
#if (IRIX || WIN32 )
    rawp->raw_nd = getdtablehi();
#endif
#if (LINUX || DARWIN)
    rawp->raw_nd = getdtablesize();
#endif
#if (AIX)
	rawp->raw_nd = FD_SETSIZE;
#endif
#if (SOLARIS )
	rawp->raw_nd = FD_SETSIZE;
#endif
	rawp->raw_msg_recv = 0;
	rawp->raw_msg_last_sequence = 0;
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_reader_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_read_wait() - wait for a message from the writer to do something
 *
 */
int32_t
xdd_raw_read_wait(worker_data_t *wdp) {
#ifdef ndef
	int 		status; /* status of send/recv function calls */
	unsigned int  	bytes_received;
	int  		bytes_remaining;
	xdd_raw_t	*rawp;


	rawp = p->rawp;
#if (IRIX || WIN32 )
	rawp->raw_nd = getdtablehi();
#endif
	status = select(rawp->raw_nd, &rawp->raw_readset, NULL, NULL, NULL);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
		* client sockets. We first check to see if the sd is in the readset.
		* If so, this means that a client is trying to make a new connection
		* in which case we need to issue an accept to establish the connection
		* and obtain a new Client Socket Descriptor (csd).
		*/
	if (FD_ISSET(rawp->raw_sd, &rawp->raw_readset)) { /* Process an incoming connection */
		rawp->raw_current_csd = rawp->raw_next_csd;
		rawp->raw_csd[rawp->raw_current_csd] = accept(rawp->raw_sd, NULL, NULL);
		FD_SET(rawp->raw_csd[rawp->raw_current_csd], &rawp->raw_active); /* Mark this fd as active */
		FD_SET(rawp->raw_csd[rawp->raw_current_csd], &rawp->raw_readset); /* Put in readset so that it gets processed */
		/* Find the next available csd close to the beginning of the CSD array */
		rawp->raw_next_csd = 0;
		while (rawp->raw_csd[rawp->raw_next_csd] != 0) {
			rawp->raw_next_csd++;
			if (rawp->raw_next_csd == FD_SETSIZE) {
					rawp->raw_next_csd = 0;
					fprintf(xgp->errout,"%s: xdd_raw_read_wait: no csd entries left\n",xgp->progname);
					break;
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	/* This section will check to see which of the Client Socket Descriptors
		* are in the readset. For those csd's that are ready, a recv is issued to 
		* receive the incoming data. The clock is then read from nclk() and the
		* new clock value is sent back to the client.
		*/
	for (rawp->raw_current_csd = 0; rawp->raw_current_csd < FD_SETSIZE; rawp->raw_current_csd++) {
		if (FD_ISSET(rawp->raw_csd[rawp->raw_current_csd], &rawp->raw_readset)) { /* Process this csd */
			/* Receive the writer's current location and length.
				*
				* When the writer closes the socket we get a read
				* indication.  Treat any short send or receive as
				* a failed connection and silently clean up.
				*/
			bytes_received = 0;
			while (bytes_received < sizeof(rawp->raw_msg)) {
				bytes_remaining = sizeof(rawp->raw_msg) - bytes_received;
				status = recv(rawp->raw_csd[rawp->raw_current_csd], (char *) &rawp->raw_msg+bytes_received, bytes_remaining, 0);
				if (status < 0) break;
				bytes_received += status;
			}
			rawp->raw_msg_recv++;
			nclk_now(&rawp->raw_msg.recvtime);
			//rawp->raw_msg.recvtime += xgp->gts_delta;
			if (status > 0) status = bytes_received;
			if (status == sizeof(rawp->raw_msg)) { /* Successful receive */
				if (rawp->raw_msg.magic != RAW_MAGIC) {
					fprintf(stderr,"xdd_raw_read_wait: Bad magic number %08x on recv %d\n",rawp->raw_msg.magic, rawp->raw_msg_recv);
				}
				if (rawp->raw_msg.recvtime < rawp->raw_msg.sendtime) {
					fprintf(stderr,"xdd_raw_read_wait: msg %lld recv time before send time by %llu nanoseconds\n",(long long)rawp->raw_msg.sequence,(long long)(rawp->raw_msg.sendtime-rawp->raw_msg.recvtime));
				}
				return(TRUE);
			} /* end of successful recv processing */
			else { /* error on the read operation */
				/* At this point, a recv returned an error in which case the connection
					* was most likely closed and we need to clear out this csd */
				/*"Deactivate" the socket. */
				FD_CLR(rawp->raw_csd[rawp->raw_current_csd], &rawp->raw_active);
				(void) closesocket(rawp->raw_csd[rawp->raw_current_csd]);
				rawp->raw_csd[rawp->raw_current_csd] = 0;
				return(FALSE);
				/* indicate that the writer is done and the reader should finish too */
			}
		} /* End of IF stmnt that processes a CSD */
	} /* End of FOR loop that processes all CSDs that were ready */
	rawp->raw_readset = rawp->raw_active;  /* Prepare for the next select */
#endif // ndef
    return(TRUE);
} /* end of xdd_raw_read_wait() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_setup_writer_socket()
 *
 */
int32_t
xdd_raw_setup_writer_socket(target_data_t *tdp) {
#ifdef ndef
	int  		status; 		/* status of send/recv function calls */
	char  		optionvalue; 	/* used to set the socket option */
	xdd_raw_t	*rawp;


	rawp = p->rawp;
    /* Create the socket and set some options */    
	rawp->raw_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (rawp->raw_sd < 0) {
		xdd_raw_err("error opening socket for RAW writer");
		return(FALSE);
	}
    /* Set the TCP_NODELAY option so that packets get sent instantly */
	optionvalue = 1;
	status = setsockopt(rawp->raw_sd, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
	if (status != 0) {
		xdd_raw_err("error setting TCP_NODELAY for RAW writer");
	}
    /* Now build the "name" of the server and connect to it. */
    (void) memset(&rawp->raw_sname, 0, sizeof(rawp->raw_sname));
    rawp->raw_sname.sin_family = AF_INET;
    rawp->raw_sname.sin_addr.s_addr = htonl(rawp->raw_addr);
    rawp->raw_sname.sin_port = htons(rawp->raw_port);
	status = connect(rawp->raw_sd, (struct sockaddr *) &rawp->raw_sname, sizeof(rawp->raw_sname));
    if (status) {
		xdd_raw_err("error connecting to socket for RAW writer");
		return(FALSE);
	}
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_setup_writer_socket() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_writer_init() - init the writer side of a read-after-write
 *
 */
int32_t
xdd_raw_writer_init(target_data_t *tdp) {
#ifdef ndef
	int  		status; 	/* status of various function calls */
	xdd_raw_t	*rawp;


	rawp = p->rawp;
	/* Set the target_options flag that identifies this target as a writer */
	p->target_options |= TO_RAW_WRITER;
	/* init the sockets */
	status = xdd_raw_sockets_init(p);
	if (status == FALSE) {
		xdd_raw_err("couldn't initialize sockets for RAW writer");
		return(FALSE);
	}
	/* Get the IP address of the reader host */
	rawp->raw_hostent = gethostbyname(rawp->raw_hostname);
	if (! rawp->raw_hostent) {
		xdd_raw_err("unable to identify the reader host");
		return(FALSE);
	}
	rawp->raw_addr = ntohl(((struct in_addr *) rawp->raw_hostent->h_addr)->s_addr);
	/* Set up the server sockets */
	status = xdd_raw_setup_writer_socket(p);
	if (status == FALSE){
		xdd_raw_err("couldn't setup sockets for RAW writer");
		return(FALSE);
	}
	rawp->raw_msg_sent = 0;
	rawp->raw_msg_last_sequence = 0;
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_writer_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_writer_send_msg() - send a message from fromt he writer to
 *      the reader for a read-after-write to be performed.
 */
int32_t
xdd_raw_writer_send_msg(worker_data_t *wdp) {
#ifdef ndef
	int  		status; 	/* status of various function calls */
	xdd_raw_t	*rawp;


	rawp = p->rawp;
	nclk_now(&rawp->raw_msg.sendtime);
	//rawp->raw_msg.sendtime += xgp->gts_delta;
	status = send(rawp->raw_sd, (char *) &rawp->raw_msg, sizeof(rawp->raw_msg),0);
	if (status != sizeof(rawp->raw_msg)) {
		xdd_raw_err("could not send message from RAW writer");
		return(FALSE);
	}
	rawp->raw_msg_sent++;
	rawp->raw_msg.sequence++;
#endif // ndef
	return(TRUE);
} /* end of xdd_raw_writer_send_msg() */
 
 
 
 
 
