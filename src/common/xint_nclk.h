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
#ifndef XINT_NCLK_H
#define XINT_NCLK_H
/*
 * nclk.h
 */
/* Routines is used in accessing a system clock register. */
/* -------- */
/* Includes */
/* -------- */
#include <limits.h>
#include "xint_misc.h" /* bool, LONGLONG_MAX, LONGLONG_MIN */
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
 
 
 
 
 
