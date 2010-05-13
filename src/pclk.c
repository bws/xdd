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
#include "pclk.h" /* pclk_t, prototype compatibility */
/* --------------- */
/* Private globals */
/* --------------- */
/* Nothing works until the pclk subsystem is initialized. */
private bool     pclk_initialized = false;
/*
 * The ticker simply counts ticks.  The value doesn't represent any
 * particular time; we use its values to track the time elapsed between
 * samples.  We translate a clock tick into some number of picoseconds
 * using this conversion variable.
 */
private pclk_t     pclk_tick_picos = 0;    /* # picoseconds per tick */
private pclk_t     pclk_ticker_max = 0;    /* Max acceptable tick value */
/* -------------------- */
/* Function definitions */
/* -------------------- */
/*----------------------------------------------------------------------------*/
/*
 * pclk_initialize()
 *
 * Initialize the clock used to record time stamps for timers.  Returns
 * the resolution of the clock (picoseconds per tick), or -1 on error.
 */
void
pclk_initialize(pclk_t *pclkp) {
    if (pclk_initialized) {
        *((pclk_t *)pclkp) =  (pclk_t) pclk_tick_picos;
        return;
    }
    ticker_open((tick_t *)&pclk_tick_picos);
    if (!pclk_tick_picos) {
        *pclkp = (pclk_t)PCLK_BAD;
        return;
    }
    /* Compute the maximum tick value that won't yield overflow on a pclk_t */
    pclk_ticker_max = PCLK_MAX;
    pclk_initialized = true;
    *pclkp =  (pclk_t) pclk_tick_picos;
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
    if (pclk_initialized) {
		ticker_close();
		pclk_tick_picos = 0;
		pclk_ticker_max = 0;
		pclk_initialized = false;
    }
}
/*----------------------------------------------------------------------------*/
/*
 * pclk_now()
 *
 * Return the current value of the ticker, in picoseconds.
 * If initialization hasn't been performed, or if the ticker
 * value has overflowed its useful range, then the return value is undefined
 */
void
pclk_now(pclk_t *pclkp) {
    pclk_t  value;

    ticker_read((tick_t *)&value);
    *pclkp = (pclk_t)(value * pclk_tick_picos);
    return;
}
 
 
 
 
 
