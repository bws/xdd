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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <libgen.h>
#include <ulimit.h>
#include <unistd.h> /* UNIX Only */
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/resource.h> /* needed for multiple processes */
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <string.h>
/* for the global clock stuff */
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#if (SNFS)
#include <client/cvdrfile.h>
#endif
#include "nclk.h" /* nclk_t, prototype compatibility */
#include "misc.h"

#define MP_MUSTRUN 1 /* Assign this thread to a specific processor */
#define MP_NPROCS 2 /* return the number of processors on the system */
typedef int  sd_t;  /* A socket descriptor */

#ifndef CLK_TCK
#define CLK_TCK sysconf(_SC_CLK_TCK)
#endif

#define DFL_FL_ADDR INADDR_ANY /* Any address */  /* server only */
#define closesocket(sd) close(sd)

#include "restart.h"

#include "ptds.h"
 
int32_t xdd_target_open_for_aix(ptds_t *p);
