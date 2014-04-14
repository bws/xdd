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
   gettime - get the global time from the global time server.
*/
#include "xdd.h"
/* global variables */
/* Input parameters passed into the program */
static char  *gettime_progname; /* Program name from argv[0] */
/* information needed to access the Global Time Server */
static in_addr_t gts_addr; /* Global Time Server IP address */
static in_port_t gts_port; /* Global Time Server Port number */
static char  *gts_hostname; /* name of the Global time server */
static char  verbose; /* verbose */
static int32_t  add;  /* Amount to add to the clock */
static int32_t  bounce;  /* number of times to hit the Global Time server */
static nclk_t  wait_for_time;  /* number of times to hit the Global Time server */
static nclk_t  global_time;  /* The actual global time */ 
static int32_t  sleep_time_dw;  /* number of milli seconds to wait  */
/*----------------------------------------------------------------------------*/
/* gts_init_global_clock_network() - Initialize the network so that we can
 *    talk to the global time timeserver.
 * This routine is given a hostname in the form of either an 
 * ascii name that can be found in the /etc/hosts file or equivalent or
 * it is given a fully qualified ip address like 192.168.13.6. 
 *
 * This routine will return the associated 32-bit address.
 *    
 */
in_addr_t
gts_init_global_clock_network(char *hostname) {
	struct hostent *hostent; /* used to init the time server info */
	in_addr_t addr;  /* Address of hostname from hostent */
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
		fprintf(stderr,"%s: Error initializing network connection\nReason: %s\n",
			gettime_progname, reason);
		WSACleanup();
		return(-1);
	} 
	/* Check the version number */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Couldn't find WinSock DLL version 2.2 or better */
		fprintf(stderr,"%s: Error initializing network connection\nReason: Could not find version 2.2\n",
			gettime_progname);
		WSACleanup();
		return(-1);
	}
#endif
	/* Network is initialized and running */
	hostent = gethostbyname(hostname);
	if (!hostent) {
		fprintf(stderr,"%s: Error: Unable to identify host %s\n",gettime_progname,gts_hostname);
#ifdef WIN32
		WSACleanup();
#endif
		return(-1);
	}
	/* Got it - unscramble the address bytes and return to caller */
	addr = ntohl(((struct in_addr *)hostent->h_addr)->s_addr);
	return(addr);
} /* end of gts_init_global_clock_network() */
/*----------------------------------------------------------------------------*/
/* gts_init_global_clock() - Initialize the global clock if requested
 */
nclk_t
gts_init_global_clock(void) {
	nclk_t  delta;  /* time returned by clk_initialize() */
	nclk_t  now;  /* the current time returned by nclk() */
	/* Global clock stuff here */
	if (gts_hostname) {
		gts_addr = gts_init_global_clock_network(gts_hostname);
		if (gts_addr == -1) { /* Problem with the network */
			fprintf(stderr,"%s: Error initializing global clock - network malfunction\n",gettime_progname);
			return(0);
		}
		clk_initialize(gts_addr, gts_port, bounce, &delta);
		nclk_now(&now);
		global_time = now+delta;
		if (verbose) 
			fprintf(stdout,"Global Time now is ");
		fprintf(stdout,"%lld", ((global_time/BILLION)+add));
	}
	if (verbose) {
		fprintf(stdout,"\ndelta is %lld ticks or %lld seconds\n", delta, delta/BILLION);
	}

	if (wait_for_time != 0) { // Wait for this particular time and then exit silently
		wait_for_time *= BILLION;
		if (wait_for_time < global_time ) {
			fprintf(stderr, "gettime: ***NOTICE*** The requested wait_for_time has already passed!\n");
			return(1);
		}
		// Ok, let's wait for this time to occur
		sleep_time_dw = (int32_t)((wait_for_time - global_time)/BILLION);
		if (verbose) {
			fprintf(stderr,"Waiting for %d milliseconds to elapse before continuing...\n",sleep_time_dw);
		}
#ifdef WIN32
		Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || DARWIN) /* Add OS Support to this line for usleep() */
		if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX )
			sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || DARWIN) /* Add OS Support to this line for usleep() as well*/
			usleep(sleep_time_dw*1000);
#endif
#endif
		nclk_now(&now);
		global_time = now+delta;
		if (verbose) 
			fprintf(stdout,"Done waiting... global Time now is ");
		fprintf(stdout,"%lld", ((global_time/BILLION)+add));
	}

	return(0);
} /* end of gts_init_global_clock() */
/*----------------------------------------------------------------------------*/
/* gts_ts_overhead() - determine time stamp overhead
 */
void
gts_ts_overhead(struct tthdr *ttp) { 
	int32_t  i;
	nclk_t  tv[101];
	ttp->timer_oh = 0;
	for (i = 0; i < 101; i++) {
		nclk_now(&tv[i]);
	}
	for (i = 0; i < 100; i++) 
		ttp->timer_oh += (tv[i+1]-tv[i]);
	ttp->timer_oh /= 100;
	fprintf(stderr,"Timer overhead is %lld nanoseconds\n", ttp->timer_oh/1000);
} /* end of gts_ts_overhead() */
/*----------------------------------------------------------------------------*/
/* gts_parse_args() - Parameter parsing 
 */
void
gts_parse_args(int32_t argc, char *argv[]) {
  	int32_t  i;

  	for (i = 1; i < argc; i++) {
    	if (strcmp(argv[i], "-timeserver") == 0) { /* name of the time server */
      		gts_hostname = argv[i+1];
      		i++;
      		continue;
    	}
    	else if (strcmp(argv[i], "-add") == 0) { /* amount to add to the result */
      		add = atoi(argv[i+1]);
      		i++;
			continue;
		}
	    else if (strcmp(argv[i], "-bounce") == 0) { /* number of times to hit the global time server */
      		bounce = atoi(argv[i+1]);
			i++;
			continue;
		}
    	else if (strcmp(argv[i], "-port") == 0) { /* port number of the time server */
      		gts_port = (in_port_t)atol(argv[i+1]);
      		i++;
			continue;
		}
    	else if (strcmp(argv[i], "-waitfortime") == 0) { /* wait for this time then exit */
      		wait_for_time = (nclk_t)atoll(argv[i+1]);
      		i++;
			continue;
		}
    	else if (strcmp(argv[i], "-verbose") == 0) { /*  */
      		verbose = 1;
      		continue;
    	} else {
      		fprintf(stderr, "%s: Invalid option: %s (%d)\n",
	      		gettime_progname, argv[i],i);
     			exit(1);
    	}
	} /* end of FOR statement that parses command line */
} /* end of gts_parse_args() */
/*----------------------------------------------------------------------------*/
/* gts_parse() - Command line parser.
 */
void
gts_parse(int32_t argc, char *argv[]) {
  	gettime_progname = argv[0];
	gts_hostname = 0;
	gts_addr = 0;
	gts_port = DEFAULT_PORT;
	wait_for_time = 0;
	add = 0;
	bounce = 100;
  	/* parse the command line arguments */
  	gts_parse_args(argc,argv);
} /* end of gts_parse() */
/*----------------------------------------------------------------------------*/
/* gts_usage() - Display usage information 
 */
void
gts_usage(int32_t fullhelp) {
	fprintf(stderr,"\t-timeserver hostname\n");
	fprintf(stderr,"\t-port port#\n");
	fprintf(stderr,"\t-verbose\n");
	fprintf(stderr,"\t-add #seconds\n");
	fprintf(stderr,"\t-bounce #times\n");;
	fprintf(stderr,"\t-waitfortime time_value\n");
} /* end of gts_usage() */
/*----------------------------------------------------------------------------*/
int32_t
main(int32_t argc,char *argv[]) {
	struct tthdr ttp;
	/* Parese the input arguments */
	gts_parse(argc,argv);
	/* Init the Global Clock */
	gts_init_global_clock();
	if (verbose) {
		gts_ts_overhead(&ttp);
		fprintf(stderr,"Bounce count is %d\n",bounce);
	}
	/* Time to leave... sigh */
	return(0);
} /* end of main() */
 
 
 
 
 
