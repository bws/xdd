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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

/* ------- */
/* Headers */
/* ------- */
#include "misc.h" /* Ensure we have LONGLONG_MIN */
/* ----- */
/* Types */
/* ----- */
/* A ticker value */
#if (LINUX || AIX || SOLARIS || OSX || FREEBSD )
typedef unsigned long long tick_t;
#else
typedef unsigned __int64  tick_t;
#endif
/* --------- */
/* Constants */
/* --------- */
#define TICKER_BAD LONGLONG_MIN /* A bad ticker value */
/* --------------------- */
/* Function declarations */
/* --------------------- */
/**
 * ticker_open()
 *
 * Open the ticker; this is required before any attempt to read it.
 *
 * Returns the period of the ticker (in picoseconds per tick), or
 * 0 on error.
 */
extern void ticker_open(tick_t *tickp);
/**
 * ticker_read()
 *
 * Read the ticker and return its current value, or TICKER_BAD if
 * an error occurs.
 */
extern void ticker_read(tick_t *tickp);
/**
 * ticker_close()
 *
 * Close the previously-opened ticker.
 */
extern void ticker_close(void);

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
