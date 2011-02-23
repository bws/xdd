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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_target_thread_cleanup() - Perform termination processing of this 
 * this Target thread.
 * Return Values: 0 is good, -1 indicates an error but what are ya gonna do?
 * 
 */
void
xdd_target_thread_cleanup(ptds_t *p) {
	ptds_t	*qp;		// Pointer to a QThread PTDS
	
	qp = p->next_qp;
	while (qp) {
		qp->task_request = TASK_REQ_STOP;
		p->occupant.occupant_type |= XDD_OCCUPANT_TYPE_CLEANUP;
		// Release this QThread
		xdd_barrier(&qp->qthread_targetpass_wait_for_task_barrier,&p->occupant,0);

		// get the next PTDS in this chain
		qp = qp->next_qp;
	}

} // End of xdd_target_thread_cleanup()

