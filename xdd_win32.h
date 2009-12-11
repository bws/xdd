/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *   I/O Performance, Inc. and
 *
 */
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
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
#include "pclk.h"
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
 
 
