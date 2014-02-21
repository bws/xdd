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
/* xdd Global Time Server */
/* ------- */
/* Headers */
/* ------- */
#define TIMESERVER_VERSION "XDD TimeServer Version 6.6.061409.0000"
#define FD_SETSIZE 32
#define HOSTNAMELENGTH 1024
#include <stdio.h> /* fprintf(), etc. */
#include <stdlib.h> /* strtoul() */
#include <stdarg.h> /* variable arguments stuff */
#include <string.h> /* strrchr(), strerror() */
#include <errno.h> /* errno stuff */
#if (IRIX || SOLARIS || AIX || LINUX || DARWIN || FREEBSD)
#include <unistd.h>
#if !(SOLARIS || AIX || LINUX || DARWIN || FREEBSD)
#include <bstring.h>
#endif
#include <limits.h> /* USHRT_MAX */
#include <netdb.h> /* gethostbyname() */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> /* inet_ntoa() */
#else /* ! IRIX */
#include <Winsock.h>
#endif /* IRIX */
#if (AIX)
#include <sys/select.h>
#endif
#include "nclk.h"
#include "misc.h" /* bool */
#include "net_utils.h"

/* ------- */
/* Renames */
/* ------- */
#define private static

/* ------ */
/* Macros used only by the time server */
/* ------ */


/* ----- */
/* Types */
/* ----- */
#if !(IRIX || SOLARIS || AIX || LINUX || DARWIN || FREEBSD )
/* SGI defines these in <netinet/in.h> */
typedef unsigned long in_addr_t; /* An IP number */
typedef unsigned short in_port_t; /* A port number */
typedef SOCKET  sd_t;  /* A socket descriptor */
#else /* sgi */
typedef int  sd_t;  /* A socket descriptor */
#endif /* IRIX */
/*
 * Note that IP addresses are stored in host byte order; they're
 * converted to network byte order prior to use for network routines.
 */
typedef struct flags_t {
    in_addr_t fl_addr;    /* IP address to use */
    in_port_t fl_port;    /* Port number to use */
} flags_t;
/* --------- */
/* Constants */
/* --------- */
#define BACKLOG SOMAXCONN   /* Pending request limit for listen() */
/* XXX *//* This needs to be converted to use a config file */
/* Default flag values */
#ifdef IRIX
#define DFL_FL_ADDR INADDR_ANY /* Any address */  /* server only */
#else /* ! IRIX */
#define DFL_FL_ADDR 0x0a000003 /* 10.0.0.3 */
#endif /* ! IRIX */
#define DFL_FL_PORT 2000  /* Port to use */
#define DFL_FL_TIME LONG_MAX  /* Zero means don't wait */
/* XXX *//* Windows might be limiting the range from 1024 to 5000 */
#define PORT_MAX USHRT_MAX /* Max value for a port number */
/* --------------- */
/* Private globals */
/* --------------- */
private char *progname; /* Global program name */
private flags_t flags = { /* Options specified on the command line */
    DFL_FL_ADDR,
    DFL_FL_PORT
};
private sd_t csd[FD_SETSIZE]; /* Client socket descriptors */
private char hostname[HOSTNAMELENGTH]; /* Host Name of this machine */
private fd_set  active; /* This set contains the sockets currently active */
private fd_set  readset;/* This set is passed to select() */
private struct sockaddr_in  sname; /* used by setup_server_socket */
private int  snamelen = sizeof(sname); /* used by setup_server_socket */
/* ------ */
/* Macros */
/* ------ */
#ifdef WIN32 /* Windows environment */
/*
 * We use this for the first argument to select() to avoid having it
 * try to process all possible socket descriptors.  Windows ignores
 * the first argument to select(), so we could use anything.  But we
 * use the value elsewhere so we give it its maximal value.
 */
#define getdtablehi() FD_SETSIZE /* Number of sockets in a fd_set */
#else /* UNIX */
/* Windows distinguishes between sockets and files; UNIX does not. */
#define closesocket(sd) close(sd)
#endif /* UNIX */
/* ---------- */
/* Prototypes */
/* ---------- */
private void     parseargs(int argc, char **argv);
private void     usage(char *fmt, ...);
private sd_t     setup_server_socket(void);
private bool     sockets_init(void);
/* -------------------- */
/* Function definitions */
/* -------------------- */
/*
 * err(fmt, ...)
 *
 * General-purpose error handling routine.  Prints the short message
 * provided to standard error, along with some boilerplate information
 * such as the program name and errno value.  Any remaining arguments
 * are used in printed message (the usage here takes the same form as
 * printf()).
 *
 * After the message is printed, execution is terminated with a nonzero
 * status.
 */
void
timeserver_err(char const *fmt, ...)
{
    va_list ap;
#ifdef WIN32
	LPVOID lpMsgBuf;
#endif
    va_start(ap, fmt);
    (void) vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, " (reason: %s)\n", strerror(errno));
#ifdef WIN32 /* Windows environment, actually */
    fprintf(stderr, "last socket error was %d\n", WSAGetLastError());
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
		fprintf(stderr,"Reason: %s",lpMsgBuf);
#endif /* WIN32 */
} /* end of timeserver_err() */
/*----------------------------------------------------------------------*/
/*
 * main()
 *
 */
int
main(int argc, char **argv) {
	sd_t    sd; 
	int     nd;  /* The number of fd sets in each readset */
	int  status; /* status of send/recv function calls */
	int  next_csd; /* index into the csd array of the next available csd */
	int  current_csd; /* The current csd being used */
	struct hostent *hostent;
	nclk_time_t mytime;
	nclk_time_t yourtime;
	nclk_t tt;


	fprintf(stderr,"%s\n",TIMESERVER_VERSION);
	/* init the sockets */
    if (! sockets_init())
		timeserver_err("couldn't initialize sockets");
	/* Parse the commandline arguments */
    parseargs(argc, argv);
	/* Init the nano-second clock */
    nclk_initialize(&tt);
    if (tt == NCLK_BAD)
		timeserver_err("couldn't initialize nanosecond clock");
	/* Get the name of the machine that we are running on */
	status = gethostname(hostname, HOSTNAMELENGTH);
	if (status != 0) {
		timeserver_err("could not get hostname");
		exit(1);
	}
	/* Get the IP address of this host */
	hostent = gethostbyname(hostname);
	if (! hostent) {
		usage("unable to identify host \"%s\"", *argv);
		exit(1);
	}
	flags.fl_addr = ntohl(((struct in_addr *) hostent->h_addr)->s_addr);
	/* Set up the server sockets */
	sd = setup_server_socket();
	/* clear out the csd table */
	for (current_csd = 0; current_csd < FD_SETSIZE; current_csd++)
		csd[current_csd] = 0;
	current_csd = next_csd = 0;
	/* Initialize the socket sets for select() */
    FD_ZERO(&readset);
    FD_SET(sd, &readset);
	active = readset;
	/* Find out how many sockets are in each set (berkely only) */
#if (IRIX || WIN32)
    nd = getdtablehi();
#endif
#if (LINUX || DARWIN)
    nd = getdtablesize();
#endif
#if (AIX)
    nd = sd + 1;
#endif
#if (SOLARIS || FREEBSD )
	nd = FD_SETSIZE;
#endif
    for (;;) {
nd = FD_SETSIZE;
		select(nd, &readset, NULL, NULL, NULL);
		/* Handle all the descriptors that are ready */
		/* There are two type of sockets: the one sd socket and multiple 
		 * client sockets. We first check to see if the sd is in the readset.
		 * If so, this means that a client is trying to make a new connection
		 * in which case we need to issue an accept to establish the connection
		 * and obtain a new Client Socket Descriptor (csd).
		 */
		if (FD_ISSET(sd, &readset)) { /* Process an incoming connection */
			current_csd = next_csd;
			csd[current_csd] = accept(sd, NULL, NULL);
			FD_SET(csd[current_csd], &active); /* Mark this fd as active */
			FD_SET(csd[current_csd], &readset); /* Put in readset so that it gets processed */
			/* Find the next available csd close to the beginning of the CSD array */
			next_csd = 0;
			while (csd[next_csd] != 0) {
				next_csd++;
				if (next_csd == FD_SETSIZE) {
						next_csd = 0;
						fprintf(stderr,"Timeserver: no csd entries left\n");
						break;
				}
			} /* end of WHILE loop that finds the next csd entry */
		} /* End of processing an incoming connection */
		/* This section will check to see which of the Client Socket Descriptors
		 * are in the readset. For those csd's that are ready, a recv is issued to 
		 * receive the incoming data. The clock is then read from nclk() and the
		 * new clock value is sent back to the client.
		 */
		for (current_csd = 0; current_csd < FD_SETSIZE; current_csd++) {
			if (FD_ISSET(csd[current_csd], &readset)) { /* Process this csd */
				/*
				 * Receive the client's clock value and bounce our
				 * own back.  We don't care what value the client
				 * sends (and it's discarded).
				 *
				 * When the client closes the socket we get a read
				 * indication.  Treat any short send or receive as
				 * a failed connection and silently clean up.
				 */
				status = recv(csd[current_csd], (char *) &yourtime, sizeof yourtime, 0);
				if (status == sizeof yourtime) { /* Successful receive */
					nclk_now(&mytime.server);
					if(mytime.server != NCLK_BAD) { /* Successful nclk read */
						mytime.server = (nclk_t) htonll(mytime.server);
						status = send(csd[current_csd], (char *) &mytime, sizeof mytime, 0);
						if (status == sizeof mytime) { /* Successful send */
							continue; /* All is good; go on. */
						} 
					} /* end of successful nclk read */
				} /* end of successful recv processing */
				else {
					/* At this point, a recv returned an error in which case the connection
					 * was most likely closed and we need to clear out this csd */
					/*"Deactivate" the socket. */
					FD_CLR(csd[current_csd], &active);
					(void) closesocket(csd[current_csd]);
					csd[current_csd] = 0; 
					/* We need to scan the csd array for the next empty csd entry */
				}
			} /* End of IF stmnt that processes a CSD */
		} /* End of FOR loop that processes all CSDs that were ready */
		readset = active; /* Prepare for the next select */
#if (IRIX || WIN32)
		nd = getdtablehi();
#endif
	} /* end of WHILE loop that processes all the sockets */
    exit(0);
} /* end of main() */
/*----------------------------------------------------------------------*/
/*
 * parseargs()
 *
 */
private void
parseargs(int argc, char **argv) {
	unsigned int port;
    argc = argc;
    progname = argv[0];
    while (*++argv)
	if (**argv == '-') /* Got an option */
	    switch (*++*argv) {
	    case 'p': { /* port number */
			port = strtoul(*++argv, NULL, 0);
			if (port > PORT_MAX)
				usage("port number %lu is out of range", port);
			flags.fl_port = (in_port_t) port;
			fprintf(stderr,"Port is set to %x\n",flags.fl_port);
			break;
			}
	    default:
			usage("unrecognized flag '%c'", **argv);
			break;
	    }
	else
	    usage("unrecognized option '%s'", *argv);
} /* end of parseargs() */
/*----------------------------------------------------------------------*/
/*
 * usage()
 *
 */
private void
usage(char *fmt, ...) {
    va_list args;
    fprintf(stderr, "\n%s: ", progname);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Usage: %s", progname);
    fprintf(stderr, " [-p port]");
    fprintf(stderr, "\n\n");
    fprintf(stderr, "\tDefault port value is %uh, max is %uh; 0 means any\n",
	DFL_FL_PORT, PORT_MAX);
    fprintf(stderr, "\tDefault global_time value is %d nanoseconds\n",
	(int)DFL_FL_TIME);
    fprintf(stderr, "\n\n");
    exit(1);
} /* end of usage() */
/*----------------------------------------------------------------------*/
/*
 * setup_server_socket()
 *
 */
private sd_t
setup_server_socket(void)
{
	sd_t  sd;
	int  status;
#if WIN32
	char  optionvalue;
#else
	int  optionvalue;
#endif

	/* Create the socket and set some options */
	if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		timeserver_err("error opening socket");
	optionvalue = 1;
	status = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
	if (status != 0) 
		timeserver_err("Error setting TCP_NODELAY socket option");
    /* Bind the name to the socket */
	(void) memset(&sname, 0, sizeof sname);
	sname.sin_family = AF_INET;
	sname.sin_addr.s_addr = htonl(flags.fl_addr);
	sname.sin_port = htons(flags.fl_port);
	if (bind(sd, (struct sockaddr *) &sname, snamelen))
		timeserver_err("error binding name to socket");
	/* Get and display the name (in case "any" was used) */
	if (getsockname(sd, (struct sockaddr *) &sname, (socklen_t *)&snamelen))
		timeserver_err("error getting socket name");
	flags.fl_addr = ntohl(sname.sin_addr.s_addr);
	flags.fl_port = ntohs(sname.sin_port);
	fprintf(stderr,"%s: Hostname is %s\n\tSocket address is 0x%x = %s\n\tPort %d <0x%x>\n", 
		progname,
		hostname,
		sname.sin_addr.s_addr, 
		inet_ntoa(sname.sin_addr),
		sname.sin_port,flags.fl_port);
	/* All set; prepare to accept connection requests */
	if (listen(sd, BACKLOG))
		timeserver_err("error starting listening on socket");
	return(sd);
} /* end of setup_server_socket() */
/*----------------------------------------------------------------------*/
/* sockets_init()
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
private bool
sockets_init(void)
{
#ifdef WIN32 /* Windows environment, actually */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)
		|| LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Couldn't find WinSock DLL version 2.2 or better */
		WSACleanup();
		return(false); 
	}
#endif /* WIN32 */
    return(true);
} /* end of sockets_init() */
 
 
