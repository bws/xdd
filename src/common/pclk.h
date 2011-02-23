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
#ifndef PCLK_H
#define PCLK_H
/*
 * pclk.h
 */
/* Routines is used in accessing a system clock register. */
/* -------- */
/* Includes */
/* -------- */
#include "misc.h" /* bool, LONGLONG_MAX, LONGLONG_MIN */
/* ----- */
/* Types  */
/* ----- */
typedef unsigned long long int pclk_t;  /* Number of picoseconds */
/* --------- */
/* Constants */
/* --------- */
#define PCLK_MAX LONGLONG_MAX
#define PCLK_BAD LONGLONG_MIN
/* --------------------- */
/* Structure declarations */
/* --------------------- */
struct pclk_time {
	pclk_t client;
	pclk_t server;
	pclk_t delta;
}; 
typedef struct pclk_time pclk_time_t;
/* --------------------- */
/* Function declarations */
/* --------------------- */
/*
 * pclk_initialize()
 *
 * Initialize the clock used to record time stamps for timers.  Returns
 * the resolution of the clock (picoseconds per tick), or -1 on error.
 */
extern void pclk_initialize(pclk_t *pclkp);
/*
 * pclk_shutdown()
 *
 * De-initialize the clock.
 */
extern void pclk_shutdown(void);
/*
 * pclk_now()
 *
 * Return the current value of the clock, in picoseconds.
 */
extern void pclk_now(pclk_t *pclkp);
/* #define PCLK_TEST */
#ifdef PCLK_TEST
/*
 * pclk_test()
 *
 * Test the picosecond clock functionality.
 */
extern void pclk_test(void);
#endif /* PCLK_TEST */
#endif /* ! PCLK_H */
 
 
 
 
 
