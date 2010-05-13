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
 * This file contains the subroutines necessary to access the actual
 * high-resolution clock for this particular OS.
 */
#include "xdd.h"
private tick_t ticker_period = 0; /* Picoseconds per tick */
/*----------------------------------------------------------------------*/
/*
 * ticker_open()
 *
 * Open the ticker; this is required before any attempt to read it.
 *
 * Returns the period of the ticker (in picoseconds per tick), or
 * 0 on error.
 */
#ifdef WIN32
void
ticker_open(tick_t *tickp) {
    __int64   ticks_per_sec;
    __int64   llticks;
    if (ticker_period) {
        *tickp = ticker_period;
        return;
    }
    if (! QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_sec))
		return; /* Performance clock not supported */
    if ((llticks = (ticks_per_sec)) < 1LL) {
    	*tickp = 0; /* Bad ticks per second value */
    	return; 
    }
    ticker_period = (tick_t) (TRILLION / llticks);
    *tickp = ticker_period;
    return;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif (LINUX)
void
ticker_open(tick_t *tickp) {
    uint32_t		picos;	// Resolution of the clock in picoseconds
	int				status;	// Status of the clock_getres() call
	struct timespec	res;	// The resolution of the real-time clock


	// If the "ticker_period" is non-zero then it has already been initialized
    if (ticker_period) {
        *tickp = ticker_period;
        return;
    }

	status = clock_getres(CLOCK_REALTIME, &res);
	if (status) {
		fprintf(stderr,"xdd: ticker_open: ERROR: Bad status getting clock resolution: %d.\n",status);
		errno = status;
		perror("Reason");
		return;
	}
    picos = TRILLION / (res.tv_nsec * BILLION); // This is the number of picoseconds per clock tick
    ticker_period = picos;
    *tickp = ticker_period;
    return;
}
#elif (SOLARIS || AIX || OSX || FREEBSD )
void
ticker_open(tick_t *tickp) {
    uint32_t picos;
    if (ticker_period) {
        *tickp = ticker_period;
        return;
    }
    picos = 1000000; /* using a microsecond clock */
    ticker_period = picos;
    *tickp = ticker_period;
    return;
}
#endif
/*----------------------------------------------------------------------*/
/*
 * ticker_read()
 *
 * Read the ticker and return its current value, or TICKER_BAD if
 * an error occurs.
 */
#ifdef WIN32
void
ticker_read(tick_t *tickp) {
    __int64  tick;
    if (ticker_period && QueryPerformanceCounter((LARGE_INTEGER *)&tick)) {
        *tickp = tick;
        return;
    }
    return; /* Ticker not open, or error reading ticker */
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif (LINUX)
void
ticker_read(tick_t *tickp) {
    struct timespec current_time;
	int		status;

    if (ticker_period) {
        status = clock_gettime(CLOCK_REALTIME, &current_time);
		if (status) {
			fprintf(stderr,"xdd: ticker_read: ERROR: Bad status reading high-resolution clock: %d.\n", status);
			errno = status;
			perror("Reason");
			return;
		}
        *tickp = (tick_t)((((long long int)current_time.tv_sec) * BILLION) + ((long long int)current_time.tv_nsec));
    }
    return;
}
#elif (SOLARIS || AIX || OSX || FREEBSD )
void
ticker_read(tick_t *tickp) {
    struct timeval current_time;
    struct timezone tz;
    tick_t i;

    if (ticker_period) {
        gettimeofday(&current_time, &tz);
        i = current_time.tv_sec;
        i *= 1000000;
        i += current_time.tv_usec;
        *tickp = i;
        return;
    }
    return;
}
#endif
/*----------------------------------------------------------------------*/
/*
 * ticker_close()
 *
 * Close the ticker.
 */
#if (WIN32 || LINUX || SOLARIS || AIX || OSX || FREEBSD )
void
ticker_close(void) {
    ticker_period = 0;
}
#endif
