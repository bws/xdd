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
 * This file contains the subroutines necessary to perform a read-after-write
 * operation across two different machines running xdd.
 */
#include "xdd.h"

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
xdd_raw_err(char const *fmt, ...)
{
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
} /* end of raw_err() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_setup_reader_socket()
 *
 */
int32_t
xdd_raw_setup_reader_socket(ptds_t *p)
{
	/* Create the socket and set some options */
	if ((p->rawp->raw_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		xdd_raw_err("error opening socket");
		return(FALSE);
	}
    /* Bind the name to the socket */
	(void) memset(&p->rawp->raw_sname, 0, sizeof(p->rawp->raw_sname));
	p->rawp->raw_sname.sin_family = AF_INET;
	p->rawp->raw_sname.sin_addr.s_addr = htonl(p->rawp->raw_addr);
	p->rawp->raw_sname.sin_port = htons(p->rawp->raw_port);
	p->rawp->raw_snamelen = sizeof(p->rawp->raw_sname);
	if (bind(p->rawp->raw_sd, (struct sockaddr *) &p->rawp->raw_sname, p->rawp->raw_snamelen)) {
		xdd_raw_err("error binding name to socket");
		return(FALSE);
	}
	/* Get and display the name (in case "any" was used) */
	if (getsockname(p->rawp->raw_sd, (struct sockaddr *) &p->rawp->raw_sname, &p->rawp->raw_snamelen)) {
		xdd_raw_err("error getting socket name");
		return(FALSE);
	}
	p->rawp->raw_addr = ntohl(p->rawp->raw_sname.sin_addr.s_addr);
	p->rawp->raw_port = ntohs(p->rawp->raw_sname.sin_port);
	fprintf(stderr,"%s: Reader Hostname is %s\n\tSocket address is 0x%x = %s\n\tPort %d <0x%x>\n", 
		xgp->progname,
		p->rawp->raw_myhostname,
		p->rawp->raw_sname.sin_addr.s_addr, 
		inet_ntoa(p->rawp->raw_sname.sin_addr),
		p->rawp->raw_sname.sin_port,p->rawp->raw_port);
	/* All set; prepare to accept connection requests */
	if (listen(p->rawp->raw_sd, SOMAXCONN)) {
		xdd_raw_err("error starting listening on socket");
		return(FALSE);
	}
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
xdd_raw_sockets_init(void)
{
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
	return(TRUE);
} /* end of xdd_raw_sockets_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_reader_init() - init the reader side of a read-after-write
 *
 */
int32_t
xdd_raw_reader_init(ptds_t *p) {
	int  status; /* status of various function calls */
	/* Set the target_options flag that identifies this target as a reader */
	p->target_options |= TO_RAW_READER;
	/* init the sockets */
	status = xdd_raw_sockets_init();
	if (status == FALSE) {
		xdd_raw_err("couldn't initialize sockets for RAW reader");
		return(FALSE);
	}
	/* Get the name of the machine that we are running on */
	status = gethostname(p->rawp->raw_myhostname, sizeof(p->rawp->raw_myhostname));
	if (status != 0 ) {
		xdd_raw_err("could not get hostname");
		return(FALSE);
	}
	/* Get the IP address of this host */
	p->rawp->raw_hostent = gethostbyname(p->rawp->raw_myhostname);
	if (! p->rawp->raw_hostent) {
		xdd_raw_err("unable to identify host");
		return(FALSE);
	}
	p->rawp->raw_addr = ntohl(((struct in_addr *) p->rawp->raw_hostent->h_addr)->s_addr);
	/* Set up the server sockets */
	status = xdd_raw_setup_reader_socket(p);
	if (status == FALSE) {
		xdd_raw_err("could not init raw reader socket");
		return(FALSE);
	}
	/* clear out the csd table */
	for (p->rawp->raw_current_csd = 0; p->rawp->raw_current_csd < FD_SETSIZE; p->rawp->raw_current_csd++)
		p->rawp->raw_csd[p->rawp->raw_current_csd] = 0;
	p->rawp->raw_current_csd = p->rawp->raw_next_csd = 0;
	/* Initialize the socket sets for select() */
    FD_ZERO(&p->rawp->raw_readset);
    FD_SET(p->rawp->raw_sd, &p->rawp->raw_readset);
	p->rawp->raw_active = p->rawp->raw_readset;
	p->rawp->raw_current_csd = p->rawp->raw_next_csd = 0;
	/* Find out how many sockets are in each set (berkely only) */
#if (IRIX || WIN32 )
    p->rawp->raw_nd = getdtablehi();
#endif
#if (LINUX || OSX)
    p->rawp->raw_nd = getdtablesize();
#endif
#if (AIX)
	p->rawp->raw_nd = FD_SETSIZE;
#endif
#if (SOLARIS )
	p->rawp->raw_nd = FD_SETSIZE;
#endif
	p->rawp->raw_msg_recv = 0;
	p->rawp->raw_msg_last_sequence = 0;
	return(TRUE);
} /* end of xdd_raw_reader_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_read_wait() - wait for a message from the writer to do something
 *
 */
int32_t
xdd_raw_read_wait(ptds_t *p) 
{
	int status; /* status of send/recv function calls */
	int  bytes_received;
	int  bytes_remaining;
#if (IRIX || WIN32 )
	p->rawp->raw_nd = getdtablehi();
#endif
	status = select(p->rawp->raw_nd, &p->rawp->raw_readset, NULL, NULL, NULL);
	/* Handle all the descriptors that are ready */
	/* There are two type of sockets: the one sd socket and multiple 
		* client sockets. We first check to see if the sd is in the readset.
		* If so, this means that a client is trying to make a new connection
		* in which case we need to issue an accept to establish the connection
		* and obtain a new Client Socket Descriptor (csd).
		*/
	if (FD_ISSET(p->rawp->raw_sd, &p->rawp->raw_readset)) { /* Process an incoming connection */
		p->rawp->raw_current_csd = p->rawp->raw_next_csd;
		p->rawp->raw_csd[p->rawp->raw_current_csd] = accept(p->rawp->raw_sd, NULL, NULL);
		FD_SET(p->rawp->raw_csd[p->rawp->raw_current_csd], &p->rawp->raw_active); /* Mark this fd as active */
		FD_SET(p->rawp->raw_csd[p->rawp->raw_current_csd], &p->rawp->raw_readset); /* Put in readset so that it gets processed */
		/* Find the next available csd close to the beginning of the CSD array */
		p->rawp->raw_next_csd = 0;
		while (p->rawp->raw_csd[p->rawp->raw_next_csd] != 0) {
			p->rawp->raw_next_csd++;
			if (p->rawp->raw_next_csd == FD_SETSIZE) {
					p->rawp->raw_next_csd = 0;
					fprintf(xgp->errout,"%s: xdd_raw_read_wait: no csd entries left\n",xgp->progname);
					break;
			}
		} /* end of WHILE loop that finds the next csd entry */
	} /* End of processing an incoming connection */
	/* This section will check to see which of the Client Socket Descriptors
		* are in the readset. For those csd's that are ready, a recv is issued to 
		* receive the incoming data. The clock is then read from pclk() and the
		* new clock value is sent back to the client.
		*/
	for (p->rawp->raw_current_csd = 0; p->rawp->raw_current_csd < FD_SETSIZE; p->rawp->raw_current_csd++) {
		if (FD_ISSET(p->rawp->raw_csd[p->rawp->raw_current_csd], &p->rawp->raw_readset)) { /* Process this csd */
			/* Receive the writer's current location and length.
				*
				* When the writer closes the socket we get a read
				* indication.  Treat any short send or receive as
				* a failed connection and silently clean up.
				*/
			bytes_received = 0;
			while (bytes_received < sizeof(p->rawp->raw_msg)) {
				bytes_remaining = sizeof(p->rawp->raw_msg) - bytes_received;
				status = recv(p->rawp->raw_csd[p->rawp->raw_current_csd], (char *) &p->rawp->raw_msg+bytes_received, bytes_remaining, 0);
				if (status < 0) break;
				bytes_received += status;
			}
			p->rawp->raw_msg_recv++;
			pclk_now(&p->rawp->raw_msg.recvtime);
			p->rawp->raw_msg.recvtime += xgp->gts_delta;
			if (status > 0) status = bytes_received;
			if (status == sizeof(p->rawp->raw_msg)) { /* Successful receive */
				if (p->rawp->raw_msg.magic != PTDS_RAW_MAGIC) {
					fprintf(stderr,"xdd_raw_read_wait: Bad magic number %08x on recv %d\n",p->rawp->raw_msg.magic, p->rawp->raw_msg_recv);
				}
				if (p->rawp->raw_msg.recvtime < p->rawp->raw_msg.sendtime) {
					fprintf(stderr,"xdd_raw_read_wait: msg %d recv time before send time by %llu picoseconds\n",p->rawp->raw_msg.sequence,p->rawp->raw_msg.sendtime-p->rawp->raw_msg.recvtime);
				}
				return(TRUE);
			} /* end of successful recv processing */
			else { /* error on the read operation */
				/* At this point, a recv returned an error in which case the connection
					* was most likely closed and we need to clear out this csd */
				/*"Deactivate" the socket. */
				FD_CLR(p->rawp->raw_csd[p->rawp->raw_current_csd], &p->rawp->raw_active);
				(void) closesocket(p->rawp->raw_csd[p->rawp->raw_current_csd]);
				p->rawp->raw_csd[p->rawp->raw_current_csd] = 0;
				return(FALSE);
				/* indicate that the writer is done and the reader should finish too */
			}
		} /* End of IF stmnt that processes a CSD */
	} /* End of FOR loop that processes all CSDs that were ready */
	p->rawp->raw_readset = p->rawp->raw_active;  /* Prepare for the next select */
    return(TRUE);
} /* end of xdd_raw_read_wait() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_setup_writer_socket()
 *
 */
int32_t
xdd_raw_setup_writer_socket(ptds_t *p)
{
	int  status; /* status of send/recv function calls */
	char  optionvalue; /* used to set the socket option */
    /* Create the socket and set some options */    
	p->rawp->raw_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p->rawp->raw_sd < 0) {
		xdd_raw_err("error opening socket for RAW writer");
		return(FALSE);
	}
    /* Set the TCP_NODELAY option so that packets get sent instantly */
	optionvalue = 1;
	status = setsockopt(p->rawp->raw_sd, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
	if (status != 0) {
		xdd_raw_err("error setting TCP_NODELAY for RAW writer");
	}
    /* Now build the "name" of the server and connect to it. */
    (void) memset(&p->rawp->raw_sname, 0, sizeof(p->rawp->raw_sname));
    p->rawp->raw_sname.sin_family = AF_INET;
    p->rawp->raw_sname.sin_addr.s_addr = htonl(p->rawp->raw_addr);
    p->rawp->raw_sname.sin_port = htons(p->rawp->raw_port);
	status = connect(p->rawp->raw_sd, (struct sockaddr *) &p->rawp->raw_sname, sizeof(p->rawp->raw_sname));
    if (status) {
		xdd_raw_err("error connecting to socket for RAW writer");
		return(FALSE);
	}
	return(TRUE);
} /* end of xdd_raw_setup_writer_socket() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_writer_init() - init the writer side of a read-after-write
 *
 */
int32_t
xdd_raw_writer_init(ptds_t *p) {
	int  status; /* status of various function calls */
	/* Set the target_options flag that identifies this target as a writer */
	p->target_options |= TO_RAW_WRITER;
	/* init the sockets */
	status = xdd_raw_sockets_init();
	if (status == FALSE) {
		xdd_raw_err("couldn't initialize sockets for RAW writer");
		return(FALSE);
	}
	/* Get the IP address of the reader host */
	p->rawp->raw_hostent = gethostbyname(p->rawp->raw_hostname);
	if (! p->rawp->raw_hostent) {
		xdd_raw_err("unable to identify the reader host");
		return(FALSE);
	}
	p->rawp->raw_addr = ntohl(((struct in_addr *) p->rawp->raw_hostent->h_addr)->s_addr);
	/* Set up the server sockets */
	status = xdd_raw_setup_writer_socket(p);
	if (status == FALSE){
		xdd_raw_err("couldn't setup sockets for RAW writer");
		return(FALSE);
	}
	p->rawp->raw_msg_sent = 0;
	p->rawp->raw_msg_last_sequence = 0;
	return(TRUE);
} /* end of xdd_raw_writer_init() */
/*----------------------------------------------------------------------*/
/*
 * xdd_raw_writer_send_msg() - send a message from fromt he writer to
 *      the reader for a read-after-write to be performed.
 */
int32_t
xdd_raw_writer_send_msg(ptds_t *p) {
	int  status; /* status of various function calls */
	pclk_now(&p->rawp->raw_msg.sendtime);
	p->rawp->raw_msg.sendtime += xgp->gts_delta;
	status = send(p->rawp->raw_sd, (char *) &p->rawp->raw_msg, sizeof(p->rawp->raw_msg),0);
	if (status != sizeof(p->rawp->raw_msg)) {
		xdd_raw_err("could not send message from RAW writer");
		return(FALSE);
	}
	p->rawp->raw_msg_sent++;
	p->rawp->raw_msg.sequence++;
	return(TRUE);
} /* end of xdd_raw_writer_send_msg() */
 
 
 
 
 
