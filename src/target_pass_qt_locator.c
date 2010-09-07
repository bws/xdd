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
 * This file contains the subroutine that locates an available QThread for
 * a specific target.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_get_next_available_qthread() - This subroutine will scan the list of
 * QThreads and return one of two values:
 *      (1) A pointer to a QThread that is available to be assigned a task
 *      (2) A NULL pointer if this is the Destination Side of an E2E operation
 *          -and- ALL the QThreads have received their EOF packet indicated
 *			by the "pass_complete" variable being set to 1.
 * Under most conditions this subroutine will block until a QThread becomes 
 * available except for the E2E Destination Side case mentioned above. 
 * The input parameter is a pointer to the Target Thread PTDS that owns 
 * the chain of QThreads to be scanned.
 * This subroutine is called by target_pass_loop() or target_pass_e2e_loop().
 */
ptds_t *
xdd_get_next_available_qthread(ptds_t *p) {
	ptds_t		*qp;					// Pointer to a QThread PTDS
	int			status;					// Status of the sem_wait system calls
	int			false_scans;			// Number of times we looked at the QP Chain expecting to find an available QThread but did not
	int			completed_qthreads;		// Number of QThreads that have completed their passes


	if ((p->target_options & TO_STRICT_ORDERING) || 
		(p->target_options & TO_LOOSE_ORDERING)) { // Strict or Loose Ordering requires us to wait for a "specific" QThread to become available
		qp = p->next_qthread_to_use;
		if (qp->next_qp) // Is there another QThread on this chain after the current one?
			p->next_qthread_to_use = qp->next_qp; // point to the next QThread in the chain
		else p->next_qthread_to_use = p->next_qp; // else point to the first QThread in the chain
		// Wait for this specific QThread to become available
		p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		status = sem_wait(&qp->this_qthread_available);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_get_next_available_qthread: Target %d: WARNING: Bad status from sem_wait on this_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		// Reset this variable as well
		pthread_mutex_lock(&qp->this_qthread_is_available_mutex);
		qp->this_qthread_is_available = 0; // Mark this QThread Unavailable
		pthread_mutex_unlock(&qp->this_qthread_is_available_mutex);

	} else { // No Strict Ordering - just wait for any QThread to become available
		false_scans = 0;
		qp = 0;
		while (qp == 0) {
			p->my_current_state |= CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
			status = sem_wait(&p->any_qthread_available);
			if (status) {
				fprintf(xgp->errout,"%s: xdd_get_next_available_qthread: Target %d: WARNING: Bad status from sem_post on any_qthread_available semaphore: status=%d, errno=%d\n",
					xgp->progname,
					p->my_target_number,
					status,
					errno);
			}
			p->my_current_state &= ~CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
			// At this point we know that at least ONE QThread has indicated that it is available. 
			// The following WHILE loop looks for that "available" QThread and takes the first one it finds
			// that is available and does NOT have its "pass_complete" variable set.
			// Any QThread that is "available" -and- has its "pass_complete" variable set to 1 is not 
			// able to be used for a task. Such a QThread is considered to have "completed" and will be 
			// counted as a "completed_qthread". When the number of "completed_qthreads" is equal to the
			// total number of QThreads for this Target Thread, then a NULL QThread pointer is returned
			// to the caller indicating that all QThreads have completed this pass. 

			// Get the first QThread pointer from this Target
			qp = p->next_qp;
			completed_qthreads = 0;
			while (qp) {
				pthread_mutex_lock(&qp->this_qthread_is_available_mutex);
				// If this QThread is available *and* pass_complete has NOT been set then we can assign this QThread
				if ((qp->this_qthread_is_available) && (qp->pass_complete == 0))  { 
					qp->this_qthread_is_available = 0; // Mark this QThread Unavailable
					pthread_mutex_unlock(&qp->this_qthread_is_available_mutex);
					break; // qp now points to the QThread to use
				}

				if ((qp->this_qthread_is_available) && (qp->pass_complete == 1))  // This QThread has completed its pass
					completed_qthreads++;

				// Release the "this_qthread_is_available" lock 
				pthread_mutex_unlock(&qp->this_qthread_is_available_mutex);
	
				// Get the next QThread Pointer 
				qp = qp->next_qp;
			} // End of WHILE (qp) that scans the QThread Chain looking for the next available QThread

			if (qp) 
				return(qp);

			if ((qp == 0)  && (completed_qthreads == p->queue_depth)) { // When there are no available QThreads because they have all completed their passes then return NULL
				return(0);
			} 
		} // END of WHILE (qp == 0)
	} // End of QThread locator 

	// At this point:
	//    The variable "qp" points to a valid QThread  
	//    That QThread is not doing anything 
	//    The "this_qthread_is_available_mutex" is unlocked
	//    The "this_qthread_is_available" variable is 0 for the QThread being returned


	return(qp);
} // End of xdd_get_next_available_qthread()

