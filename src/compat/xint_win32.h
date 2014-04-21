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
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <memory.h>
#include <string.h>
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include "mmsystem.h"
#include <Winsock.h>
#include <sys/stat.h>

// XDD include files
#include "nclk.h"
#include "misc.h"
// For Windows, ptds needs to be included here. 
#include "ptds.h"

#define DFL_FL_ADDR 0x0a000001 /*  10.0.0.1 */


// The remainder is OS-specific
/* ------------------------------------------------------------------
 * Macros
 * ------------------------------------------------------------------ */

/*
 * We use this for the first argument to select() to avoid having it
 * try to process all possible socket descriptors. Windows ignores
 * the first argument to select(), so we could use anything.  But we
 * use the value elsewhere so we give it its maximal value.
 */
#define getdtablehi() FD_SETSIZE  /* Number of sockets in a fd_set */

/* ------------------------------------------------------------------
 * Function Prototype Definition specific to Windows
 * ------------------------------------------------------------------ */
int32_t  ts_serializer_init(HANDLE *mp, char *mutex_name);
 
 
