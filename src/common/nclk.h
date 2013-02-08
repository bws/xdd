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
#ifndef NCLK_H
#define NCLK_H
/*
 * nclk.h
 */
/* Routines is used in accessing a system clock register. */
/* -------- */
/* Includes */
/* -------- */
#include <limits.h>
#include "misc.h" /* bool, LONGLONG_MAX, LONGLONG_MIN */
/* ----- */
/* Types  */
/* ----- */
typedef unsigned long long int nclk_t;  /* Number of nanoseconds */
/* --------- */
/* Constants */
/* --------- */
#define NCLK_MAX ULONGLONG_MAX
#define NCLK_BAD ULONGLONG_MIN
/* --------------------- */
/* Structure declarations */
/* --------------------- */
struct nclk_time {
	nclk_t client;
	nclk_t server;
	nclk_t delta;
}; 
typedef struct nclk_time nclk_time_t;
/* --------------------- */
/* Function declarations */
/* --------------------- */
/*
 * nclk_initialize()
 *
 * Initialize the clock used to record time stamps for timers.  Returns
 * the resolution of the clock (nanoseconds per tick), or -1 on error.
 */
extern void nclk_initialize(nclk_t *nclkp);
/*
 * nclk_shutdown()
 *
 * De-initialize the clock.
 */
extern void nclk_shutdown(void);
/*
 * nclk_now()
 *
 * Return the current value of the clock, in nanoseconds.
 */
extern void nclk_now(nclk_t *nclkp);
/*
 * nclk_now()
 *
 * Return the current value of the clock, in nanoseconds.
 */
extern void nclk_now(nclk_t *nclkp);
/* #define NCLK_TEST */
#ifdef NCLK_TEST
/*
 * nclk_test()
 *
 * Test the nanosecond clock functionality.
 */
extern void nclk_test(void);
#endif /* NCLK_TEST */
#endif /* ! NCLK_H */
 
 
 
 
 
