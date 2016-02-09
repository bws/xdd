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

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

/* POSIX headers */
#include <sys/types.h>
#include <libgen.h>
#include <unistd.h> 
#include <ctype.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <sys/prctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/resource.h> /* needed for multiple processes */
#include <pthread.h>
#include <sched.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <string.h>
#include <syscall.h>
/* for the global clock stuff */
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#if (SNFS)
#include <client/cvdrfile.h>
#endif

/* Platform headers */
#if HAVE_ENABLE_XFS && HAVE_XFS_XFS_H
#include <xfs/xfs.h>
#endif
#if HAVE_ENABLE_XFS && HAVE_XFS_LIBXFS_H
#include <xfs/libxfs.h>
#endif
#if XFS_ENABLED && !HAVE_ENABLE_XFS
#error "ERROR: XFS Support is enabled, but the header support is not valid."
#endif
#if HAVE_LINUX_MAGIC_H && HAVE_DECL_XFS_SUPER_MAGIC
#include <linux/magic.h>
#else
#define XFS_SUPER_MAGIC 0x58465342
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif



/* XDD internal compatibility headers */
#include "xint_nclk.h" /* nclk_t, prototype compatibility */
#include "xint_misc.h"
#include "xint_restart.h"

#define MP_MUSTRUN 1 /* Assign this thread to a specific processor */
#define MP_NPROCS 2 /* return the number of processors on the system */
typedef int  sd_t;  /* A socket descriptor */
#define CLK_TCK sysconf(_SC_CLK_TCK)
#define DFL_FL_ADDR INADDR_ANY /* Any address */  /* server only */
#define closesocket(sd) close(sd)
extern int h_errno; // For socket calls

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
