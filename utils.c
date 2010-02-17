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
char *
xdd_getnexttoken(char *tp) {
	char *cp;
	cp = tp;
	while ((*cp != TAB) && (*cp != SPACE)) cp++;
	while ((*cp == TAB) || (*cp == SPACE)) cp++;
	return(cp);
} /* end of xdd_getnexttoken() */
/*----------------------------------------------------------------------------*/
/* xdd_random_int() - returns a random integer
 */
int
xdd_random_int(void) {
#ifdef  LINUX


	if (xgp->random_initialized == 0) {
		initstate(72058, xgp->random_init_state, 256);
		xgp->random_initialized = 1;
	}
#endif
#ifdef WIN32
	return(rand());
#else
	return(random());
#endif
} /* end of xdd_random_int() */
/*----------------------------------------------------------------------------*/
/* xdd_random_float() - returns a random floating point number in double.
 */
double
xdd_random_float(void) {
	double rm;
	double recip;
	double rval;
#ifdef WIN32
	return((double)(1.0 / RAND_MAX) * rand());
#elif LINUX
	rm = RAND_MAX;
#else
	rm = 2^31-1;
#endif
	recip = 1.0 / rm;
	rval = random();
	rval = recip * rval;
	if (rval > 1.0)
		rval = 1.0/rval;
	return(rval);
} /* end of xdd_random_float() */
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
