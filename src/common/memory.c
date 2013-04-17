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
 * This file contains the subroutines that perform various initialization 
 * functions with respect to memory such as locking and unlocking pages.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
void
xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not lock memory 
	if (xgp->global_options & GO_NOMEMLOCK)
		return;

#if (AIX)
	int32_t liret;
	off_t newlim;
	/* Get the current memory stack limit - required before locking the the memory */
	liret = -1;
	newlim = -1;
	liret = ulimit(GET_STACKLIM,0);
	if (liret == -1) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not get memory stack limit\n",
				getpid(),xgp->progname);
		perror("Reason");
	}
	/* Get 8 pages more than the stack limit */
	newlim = liret - (PAGESIZE*8);
	return;
#else
#if  (LINUX || SOLARIS || DARWIN || AIX || FREEBSD)
	if (getuid() != 0) {
		fprintf(xgp->errout,"(PID %d) %s: You must run as superuser to lock memory for %s\n",
			getpid(),xgp->progname, sp);
		return;
	}
#endif
	status = mlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not lock %d bytes of memory for %s\n",
				getpid(),xgp->progname,bsize,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_lock_memory() */
/*----------------------------------------------------------------------------*/
void
xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not unlock memory because it was never locked
	if (xgp->global_options & GO_NOMEMLOCK)
		return;
#if (AIX)
#ifdef notdef
	status = plock(UNLOCK);
	if (status) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not unlock memory for %s\n",
				getpid(),xgp->progname, sp);
		perror("Reason");
	}
#endif
	return;
#else
#if (IRIX || SOLARIS || LINUX || DARWIN || FREEBSD)
	if (getuid() != 0) {
		return;
	}
#endif
	status = munlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not unlock memory for %s\n",
				getpid(),xgp->progname,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_unlock_memory() */
