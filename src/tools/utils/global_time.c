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
/*----------------------------------------------------------------------------*/
/* This file contains all the routines necessary to contact the global
 * clock time server and get a sense of the global time.
 */
/*----------------------------------------------------------------------------*/
/*
 * Returns the difference (in nanoseconds) between global and local
 * senses of picsecond time.
 */
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <sys/timeb.h>
#include <memory.h>
#include <string.h>
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include "mmsystem.h"
#else /* GENERIC_UNIX */
#include <netdb.h>
#include <sys/socket.h>
#if (FREEBSD)
#include <sys/types.h>
#include <sys/unistd.h>
#endif /* FREEBSD */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#if (AIX || FREEBSD || SOLARIS)
#include <stdarg.h>
#endif
#include "xdd.h"
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
globtim_err(char const *fmt, ...) {
#ifdef WIN32
	LPVOID lpMsgBuf;
#endif
	fprintf(stderr, "%s\n", fmt);
	perror("Reason");
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
} /* end of globtim_err() */
/*----------------------------------------------------------------------------*/
/* clk_initialize() - This routine calls clk_delta() to get the time difference
 * between the local clock and the master clock. This delta value is returned
 * to the calling routine.
 */
void
clk_initialize(in_addr_t addr, in_port_t port, int32_t bounce, nclk_t *nclkp) {
    nclk_t tt;

    nclk_initialize(&tt);
    if (tt == NCLK_BAD)
        globtim_err("\nglobal_time: Could not initialize nanosecond clock"); 

    clk_delta(addr, port, bounce, nclkp);
    return;

} /* end of clk_initialize() */
/*----------------------------------------------------------------------------*/
/* clk_delta() - This routine will contact the master time server (identified
 * by the address and port number passed in as arguments to this routine) and
 * request the clock value from the master time server. This clock value is 
 * is received "bounce" number of times and the clock value received from the
 * shortest round trip time is used as the value to calculate the difference
 * between the local clock and the value of the master time server minus one
 * half the shortest round trip time.
 * 
 */
void
clk_delta(in_addr_t addr, in_port_t port, int32_t bounce, nclk_t *nclkp) {
    sd_t  sd;
    struct sockaddr_in sname;
    int32_t  i;
    int32_t  status;
    nclk_t  delta = 0;
    nclk_t  min = NCLK_MAX;
    nclk_t  max = 0;
    nclk_t  now = 0;
    nclk_t  currentclock;
    nclk_t  roundtriptime;
    nclk_t  remoteclock;
    nclk_time_t in;
    nclk_time_t out;
#if WIN32
    char optionvalue;
#else
    int32_t  optionvalue;
#endif


    /* Create the socket and set some options */
    if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        globtim_err("\nglobal_time: Error opening socket");
    /* Set Socket Options. */
    optionvalue = 1;
    status = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &optionvalue, sizeof(optionvalue));
    if (status != 0) {
        globtim_err("\nglobal_time: Error setting TCP_NODELAY socket option\n");
    }
    /* Now build the "name" of the server and connect to it. */
    (void) memset(&sname, 0, sizeof sname);
    sname.sin_family = AF_INET;
    sname.sin_addr.s_addr = htonl(addr);
    sname.sin_port = htons(port);
    if (connect(sd, (struct sockaddr *) &sname, sizeof sname)) {
        globtim_err("\nglobal_time: Error connecting to socket");
        *nclkp = -1;
        return;
    }
    /* Bounce times back and forth a bunch of times, ignoring errors. */
    for (i = 0; i < bounce; i++) {
        nclk_now(&currentclock);
        if (currentclock != NCLK_BAD) {
            out.client = (nclk_t) htonll(currentclock);
            out.delta  = (nclk_t) htonll(delta);
            /* send the current clock to the master time server */
            send(sd, (char *) &out, sizeof out, 0);
            /* get the clock value back from the master time server */
            recv(sd, (char *) &in, sizeof in, 0);
            nclk_now(&now);
            /* Find the quickest turnaround time and record that clock value. */
            remoteclock = (nclk_t) ntohll(in.server);
            roundtriptime = now - currentclock;
            if (roundtriptime < min) {    /* Record quickest turnaround */
                min = roundtriptime;
                delta = remoteclock - (now + roundtriptime/2);
            } else if (roundtriptime > max) {
                max = roundtriptime;
            }
        } else {
        fprintf(stderr,"bounce %d BAD nclk\n", i );
        }
    }
    /* refine the delta a bit */
    for (i = 0; i < bounce; i++) {
        nclk_now(&currentclock);
        if (currentclock != NCLK_BAD) {
            /* send the current clock to the master time server */
            send(sd, (char *) &out, sizeof out, 0);
            /* get the clock value back from the master time server */
            recv(sd, (char *) &in, sizeof in, 0);
            nclk_now(&now);
            /* Find the quickest turnaround time and record that clock value. */
            remoteclock = (nclk_t) ntohll(in.server);
            if (remoteclock > now+delta) {
//              fprintf(stderr,"bounce %d remote is ahead by %llu nano seconds\n", i, (remoteclock-(now+delta)));
                delta += (remoteclock-(now+delta));
            }
        } else fprintf(stderr,"bounce %d BAD nclk\n", i );
    }
    (void) closesocket(sd);
//fprintf(stderr,"now+delta difference from remote is %llu nano seconds\n", ((now+delta)-remoteclock));
//fprintf(stderr,"min roundtriptime=%lld, max roundtriptime=%lld, remoteclock=%llu, now=%llu, \ndelta=%llu\n",min,max,remoteclock,now,delta);
    *nclkp = delta;
    return;
} /* end of clk_delta() */
