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
#include "xdd.h"

//******************************************************************************
// After I/O Loop
//******************************************************************************

/*----------------------------------------------------------------------------*/
/* xdd_io_loop_after_loop() - This subroutine will do all the stuff needed to 
 * be done after the inner-most loop has done does all the I/O operations
 * that constitute a "pass" or some portion of a pass if it terminated early.
 */
int32_t
xdd_io_loop_after_loop(ptds_t *p) {
	int32_t  status;


	/* Get the ending time stamp */
	pclk_now(&p->my_pass_end_time);
	p->my_elapsed_pass_time = p->my_pass_end_time - p->my_pass_start_time;

	/* Get the current CPU user and system times and the effective current wall clock time using pclk_now() */
	times(&p->my_current_cpu_times);

	status = xdd_lockstep_after_io_loop(p);

	if (status) return(1);

	return(0);
} // End of xdd_io_loop_after_loop()

 
