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
 * This set of routines is used in accessing a system clock.
 */
/* -------- */
/* Includes */
/* -------- */
#include <stdio.h>
#if (LINUX)
#include <time.h> /* pclk_t, prototype compatibility */
#endif
#include "pclk.h" /* pclk_t, prototype compatibility */
/* --------------- */
/* Private globals */
/* --------------- */
/* Nothing works until the pclk subsystem is initialized. */
private bool     pclk_initialized = false;
/*----------------------------------------------------------------------------*/
/* pclk_initialize()
 *
 * Initialize the clock used to record time stamps for timers.  Returns
 * the resolution of the clock (picoseconds per tick)
 * In some operating systems there is an actual "initialization" routine that
 * needs to be called but most of those have gone away so the initialization 
 * function is simply returning the resolution of the clock to the caller. 
 *
 */
void
pclk_initialize(pclk_t *pclkp) {

#if (LINUX)
	// Since we use the "nanosecond" clocks in Linux, 
	// the number of picoseconds per nanosecond "tick" is 1000
    *pclkp =  THOUSAND;
#elif (SOLARIS || AIX || OSX || FREEBSD )
	// Since we use the "gettimeofday" clocks in this OS, 
	// the number of picoseconds per microsecond "tick" is 1,000,000
    *pclkp =  MILLION;
#endif
    pclk_initialized = true;
    return;
}
/*----------------------------------------------------------------------------*/
/*
 * pclk_shutdown()
 *
 * De-initialize the clock.
 */
void
pclk_shutdown(void) {
    if (pclk_initialized) 
		pclk_initialized = false;
}
/*----------------------------------------------------------------------------*/
/*
 * pclk_now()
 *
 * Return the current value of the high resolution clock, in picoseconds.
 * If initialization hasn't been performed then the return value is 0.
 */
#ifdef WIN32
void
pclk_now(pclk_t *pclkp) {

    if(pclk_initialized) 
		QueryPerformanceCounter((LARGE_INTEGER *)pclkp);
	else *pclkp = 0;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif (LINUX)
void
pclk_now(pclk_t *pclkp) {
    struct timespec current_time;

    if(pclk_initialized) {
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		*pclkp =  (pclk_t)(((long long int)current_time.tv_sec * TRILLION) +
						   ((long long int)current_time.tv_nsec * THOUSAND));
    } else *pclkp = 0;
    return;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif (SOLARIS || AIX || OSX || FREEBSD )
void
pclk_now(pclk_t *pclkp) {
    struct timeval current_time;
    struct timezone tz;
    pclk_t i;

    if(pclk_initialized) {
        gettimeofday(&current_time, &tz);
		*pclkp =  (pclk_t)(((long long int)current_time.tv_sec * TRILLION) +
						   ((long long int)current_time.tv_usec * MILLION));
    } else *pclkp = 0;
    return;
}
#endif
