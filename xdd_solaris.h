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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h> /* UNIX Only */
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/resource.h> /* needed for multiple processes */
#include <sys/utsname.h>
#include <sys/processor.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/unistd.h>
/* for the global clock stuff */
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#if (SNFS)
#include <client/cvdrfile.h>
#endif
#include "pclk.h" /* pclk_t, prototype compatibility */
#include "misc.h"

#define MP_MUSTRUN 1 /* ASsign this thread to a specific processor */
#define MP_NPROCS 2 /* return the number of processors on the system */
typedef int  sd_t;  /* A socket descriptor */
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif
#undef RAND_MAX
#define RAND_MAX ((1024*1024*1024*2)-1)

/* ------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------ */
#define DFL_FL_ADDR INADDR_ANY /* Any address */  /* server only */
#define closesocket(sd) close(sd)
static bool  sockets_init(void);
#include "ptds.h"
 
