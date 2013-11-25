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
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_target_thread_cleanup() - Perform termination processing of this 
 * this Target thread.
 * Return Values: 0 is good, -1 indicates an error but what are ya gonna do?
 * 
 */
void
xdd_target_thread_cleanup(target_data_t *tdp) {
	worker_data_t	*wdp;		// Pointer to a Worker Thread Data Struct
	
	wdp = tdp->td_next_wdp;
	while (wdp) {
		wdp->wd_task.task_request = TASK_REQ_STOP;
		tdp->td_occupant.occupant_type |= XDD_OCCUPANT_TYPE_CLEANUP;
		// Release this Worker Thread
		xdd_barrier(&wdp->wd_thread_targetpass_wait_for_task_barrier,&tdp->td_occupant,0);

		// get the next Worker in this chain
		wdp = wdp->wd_next_wdp;
	}

        /* On e2e XNI, part of cleanup includes closing the source side */
        if ((TO_ENDTOEND & tdp->td_target_options) &&
            (PLAN_ENABLE_XNI & tdp->td_planp->plan_options)) {
            xni_close_connection(&tdp->td_e2ep->xni_td_conn);
        }

} // End of xdd_target_thread_cleanup()

