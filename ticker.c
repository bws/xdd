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
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#if ( IRIX )
#include <fcntl.h> /* open(), O_RDONLY */
#include <errno.h> /* ENODEV */
#include <sys/mman.h> /* mmap(), MAP_SHARED, etc. */
#include <sys/syssgi.h> /* syssgi() */
/* Name of file through which we mmapped the ticker */
#define TICKER_FILE "/dev/mmem"
private unsigned int volatile     *ticker32 = NULL;
private unsigned long long volatile *ticker64 = NULL;
/*
 * These are used in setting up direct mmap'd access to the ticker.
 *
 * We map the memory that contains the clock register by opening a
 * file; we need to keep that file open, and ticker_fd is the file
 * descriptor for that open file.  We have to map memory in system
 * page size increments; pgsize records the system page size.
 */
private int ticker_fd = -1;     /* Opened TICKER_FILE descriptor */
private int pgsize = 0;     /* System page size */
#endif
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
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
#elif (LINUX || SOLARIS || AIX || OSX || FREEBSD )
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
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif ( IRIX )
void
ticker_open(tick_t *tickp) {
    ptrdiff_t   pgoffset;
    ptrdiff_t   pgaddr;
    ptrdiff_t   pgmask;
    uint32_t picos;
    if (ticker_period) {
        *tickp = ticker_period;
        return;
    }
    /* Look up our system page size. */
    pgsize = getpagesize();
    pgmask = (ptrdiff_t) (pgsize - 1);
    /*
     * Get physical address of the system's free running cycle counter.
     * We get the clock period (in picoseconds per tick) in the process.
     */
    if ((pgaddr = syssgi(SGI_QUERY_CYCLECNTR, &picos)) == ENODEV)
		return;    /* Hardware cycle counter not supported */
    /* Separate clock address into a base page address and offset */
    pgoffset = pgaddr & pgmask;
    pgaddr &= ~pgmask;

    /* Map the page containing the counter into memory (anywhere). */
    if ((ticker_fd = open(TICKER_FILE, O_RDONLY)) < 0) {
        *tickp = 0;
        return;    /* Couldn't open mappable kernel memory device */
    }

    pgaddr = (ptrdiff_t) mmap(NULL, (size_t) pgsize, PROT_READ,
                MAP_SHARED, ticker_fd, (off_t) pgaddr);

    if (pgaddr == (ptrdiff_t) MAP_FAILED) {
        (void) close(ticker_fd);
        *tickp = 0;
        return;    /* Failed to map in needed kernel memory */
    }

    /*
     * Find out out big our clock value is (32 or 64 bits), and
     * make the appropriate pointer refer to the clock's location
     * in our address space.
     */
    if (syssgi(SGI_CYCLECNTR_SIZE) == 64)
		ticker64 = (unsigned long long volatile *) (pgaddr + pgoffset);
    else
		ticker32 = (unsigned int volatile *) (pgaddr + pgoffset);
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
#elif (LINUX || SOLARIS || AIX || OSX || FREEBSD )
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
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif ( IRIX )
void
ticker_read(tick_t *tickp) {
    if (ticker_period) {
        if(ticker64) 
            *tickp = *ticker64;
        else *tickp = *ticker32;
        return;
    }
    *tickp = TICKER_BAD;
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
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#elif ( IRIX )
void
ticker_close(void) {
    ptrdiff_t const pgmask = (ptrdiff_t) (pgsize - 1);
    if (ticker_period) {
	if (ticker32) {
	    (void) munmap((void *) ((ptrdiff_t) ticker32 & ~pgmask), pgsize);
	    ticker32 = NULL;
	} else if (ticker64) {
	    (void) munmap((void *) ((ptrdiff_t) ticker64 & ~pgmask), pgsize);
	    ticker64 = NULL;
	}
	pgsize = 0;
	if (ticker_fd >= 0) {
	    (void) close(ticker_fd);
	    ticker_fd = -1;
	}
	ticker_period = 0;
    }
}
#endif
 
 
 
 
 
