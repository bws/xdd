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
/*
 * This file contains a variety of utility functions used by any of the other 
 * functions within xdd.
 */
#include "xint.h"
#include <stdlib.h>

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
    
#ifdef HAVE_INITSTATE
    if (xgp->random_initialized == 0) {
	initstate(xgp->random_init_seed, xgp->random_init_state, 256);
	xgp->random_initialized = 1;
    }
#endif

#ifdef HAVE_RANDOM
    return random();
#elif HAVE_RAND
    return rand();
#endif
} /* end of xdd_random_int() */
/*----------------------------------------------------------------------------*/
/* xdd_random_float() - returns a random floating point number in double.
 */
double xdd_random_float(void) {
    double rm;
    double recip;
    double rval;
    
#ifdef HAVE_INITSTATE
    if (xgp->random_initialized == 0) {
	initstate(xgp->random_init_seed, xgp->random_init_state, 256);
	xgp->random_initialized = 1;
    }
#endif

#ifdef HAVE_RANDOM
    rm = RAND_MAX;
    recip = 1.0 / rm;
    rval = random();
    rval = recip * rval;
    if (rval > 1.0)
	rval = 1.0/rval;
#elif HAVE_RAND
    rval = ((double)(1.0 / RAND_MAX) * rand());
#endif
    return(rval);
} /* end of xdd_random_float() */
 
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
