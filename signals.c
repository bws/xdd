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
 * functions when xdd is started.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_sigint() - Routine that gets called when an Interrupt occurs. This will
 * call the appropriate routines to remove all the barriers and semaphores so
 * that we shut down gracefully.
 */
void
xdd_sigint(int n) {
	fprintf(xgp->errout,"Program canceled - destroying all barriers...");
	fflush(xgp->errout);
	xgp->canceled = 1;
	xdd_destroy_all_barriers();
	fprintf(xgp->errout,"done. Exiting\n");
	fflush(xgp->errout);
} /* end of xdd_sigint() */

/*----------------------------------------------------------------------------*/
/* xdd_init_signals() - Initialize all the signal handlers
 */
void
xdd_init_signals(void) {
	signal(SIGINT, xdd_sigint);
} /* end of xdd_init_signals() */

