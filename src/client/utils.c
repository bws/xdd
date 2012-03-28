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
 * This file contains a variety of utility functions used by any of the other 
 * functions within xdd.
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
// Given a pointer to a string, this routine will place a NULL character at the
// end of each non-white-space group of characters (aka tokens) and return
// the total number of tokens found. All other white space is left alone
int
xdd_tokenize(char *cp) {
	int		tokens;		// Current number of tokens
	int		len;		// Length of the line 
	int		tokenlen;	// length of a token
	char	*tp;		// Pointer to the start of a token

	len = strlen(cp);
	if (len < 1) 
		return(0);

	tokens = 0;
	while (len > 0) {
		tokenlen = 0;
		// Skip past any leading white space 
		while ((*cp == TAB) || (*cp == SPACE)) {
			len--;
			cp++;
		}
		// Now that we are at the start of the token, skip over the token
		tp = cp;
		while ((*cp != TAB) && (*cp != SPACE) && (*cp != '\0') && (*cp != '\n')) {
			len--;
			cp++;
			tokenlen++;
		}
		// Now we are at the end of the token so put a null character there and continue
		*cp = '\0';
		tokens++;
		len--;
		cp++;
	}

	return(tokens);
} /* end of xdd_tokenize() */

/*----------------------------------------------------------------------------*/
/* xdd_random_int() - returns a random integer
 */
int
xdd_random_int(void) {
#ifdef  LINUX


	if (xgp->random_initialized == 0) {
		initstate(xgp->random_init_seed, xgp->random_init_state, 256);
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
	rm = (2^31)-1;
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
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
