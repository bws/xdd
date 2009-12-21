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
// I/O Loop
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_io_loop() - The actual "inner" loop that does all the I/O operations 
 */
int32_t
xdd_io_loop(ptds_t *p) {
	int32_t  status;		// Status of the last function call

if (xgp->global_options & GO_DEBUG_INIT) fprintf(stderr,"io_loop: enter, p=0x%x\n",p);

	// Set up for I/O loop
	status = xdd_io_loop_before_loop(p);

	/* This is the main loop for a single pass */
	for (p->my_current_op = 0; p->my_current_op < p->target_ops; p->my_current_op++) {

		// Set up for I/O operation
		status = xdd_io_loop_before_io_operation(p);
		if (status == FAILED) break;

		// Issue the I/O operation
		status = xdd_io_loop_perform_io_operation(p);

		// Check I/O operation completion
		status = xdd_io_loop_after_io_operation(p);
		if (status == 1) break;

	}

	// Perform Completion functions for this pass
	status = xdd_io_loop_after_loop(p);

if (xgp->global_options & GO_DEBUG_INIT) fprintf(stderr,"io_loop: exit, p=0x%x\n",p);
	// Exit this routine
	return(status);
} // End of xdd_io_loop()

 
