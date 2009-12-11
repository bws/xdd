/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *      Alex Elder
 *      Tom Ruwart (tmruwart@ioperformance.com)
 *      I/O Perofrmance, Inc.
 */
/*
 * This set of routines is used in accessing a system clock.
 */
/* -------- */
/* Includes */
/* -------- */
#include <stdio.h>
#include "pclk.h" /* pclk_t, prototype compatibility */
#include "ticker.h" /* Machine dependent ticker routines */
#include "misc.h" /* bool, Assert() */
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
pclk_initialize(pclk_t *pclkp)
{
//fprintf(stderr,"pclk_init: size of pclk_t is %d bytes size of PCLK_BAD is %d \n",sizeof(pclk_t), sizeof(PCLK_BAD));
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
pclk_shutdown(void)
{
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
 * value has overflowed its useful range, returns PCLK_BAD.
 */
void
pclk_now(pclk_t *pclkp)
{
    pclk_t  value;

    if (! pclk_initialized) {
        *pclkp = (pclk_t) PCLK_BAD;
        return; /* Nothing works until initialized */
    }

    ticker_read((tick_t *)&value);
    if (value == (pclk_t) PCLK_BAD) {
        *pclkp = (pclk_t) PCLK_BAD;
        return; /* Something is wrong */
    }

    /* We could be more robust, but for now, we'll just err on overflow */
    if (value > pclk_ticker_max) {
        *pclkp = (pclk_t) PCLK_BAD;
        return; /* Overflow */
    }
    *pclkp = (pclk_t)(value * pclk_tick_picos);
    return;
}
 
 
 
 
 
