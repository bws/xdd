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
/* xdd_get_specific_qthread() - This subroutine will locate the specified
 * QThread and wait for it to become available then return its pointer.
 * This subroutine is called by xdd_target_pass()
 */
ptds_t *
xdd_get_specific_qthread(ptds_t *p, int32_t q) {
	ptds_t		*qp;					// Pointer to a QThread PTDS
	int			status;					// Status of the sem_wait system calls
	int			i;
	pclk_t		checktime;

	nclk_now(&checktime);

	// Sanity Check
	if (q >= p->queue_depth) { // This should *NEVER* happen - famous last words...
		fprintf(xgp->errout,"%s: xdd_get_specified_qthread: Target %d: INTERNAL ERROR: Specified QThread is out of range: q=%d\n",
			xgp->progname,
			p->my_target_number,
			q);
		exit(1);
	}
	
	// Locate the pointer to the requested QThread PTDS
	// The QThreads are on an ordered linked list anchored on the Target Thread PTDS with QThread being the first one on the list
	qp = p->next_qp;
	for (i=0; i<q; i++) 
		qp = qp->next_qp;
	// qp should now point to the desired QThread

	// Wait for this specific QThread to become available
	pthread_mutex_lock(&qp->qthread_target_sync_mutex);
	if (qp->qthread_target_sync & QTSYNC_BUSY) { 
		// Set the "target waiting" bit, unlock the mutex, and lets wait for this QThread to become available - i.e. not busy
		qp->qthread_target_sync |= QTSYNC_TARGET_WAITING;
	nclk_now(&checktime);
		pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
		p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		status = sem_wait(&qp->this_qthread_is_available_sem);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_get_specified_qthread: Target %d: WARNING: Bad status from sem_wait on this_qthread_is_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
		pthread_mutex_lock(&qp->qthread_target_sync_mutex);
		qp->qthread_target_sync |= QTSYNC_BUSY; // Indicate that this QThread is now busy
		pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
		// Upon waking up from this sem_wait(), this QThread should now be available
	} else {
		qp->qthread_target_sync |= QTSYNC_BUSY; // Indicate that this QThread is now busy
		pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
	}

	// At this point we have a pointer to the specified QThread
	return(qp);

} // End of  xdd_get_specific_qthread()

/*----------------------------------------------------------------------------*/
/* xdd_get_any_available_qthread() - This subroutine will scan the list of
 * QThreads and return a pointer to a QThread that is available to be assigned a task.
 * This subroutine is called by xdd_target_pass()
 */
ptds_t *
xdd_get_any_available_qthread(ptds_t *p) {
	ptds_t		*qp;					// Pointer to a QThread PTDS
	int			status;					// Status of the sem_wait system calls
	int			eof;					// Number of QThreads that have reached End-of-File on the destination side of an E2E operation

	// Just wait for any QThread to become available
	qp = 0;
	while (qp == 0) {
		p->my_current_state |= CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
		status = sem_wait(&p->any_qthread_available_sem);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_get_any_available_qthread: Target %d: WARNING: Bad status from sem_post on any_qthread_available semaphore: status=%d, errno=%d\n",
				xgp->progname,
				p->my_target_number,
				status,
				errno);
		}
		p->my_current_state &= ~CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
		// At this point we know that at least ONE QThread has indicated that it is not busy. 
		// The following WHILE loop looks for the QThread that is not busy,
		// marks it "BUSY", and returns that QThread pointer (qp).

		// Get the first QThread pointer from this Target
		qp = p->next_qp;
		eof = 0;
		while (qp) {
			pthread_mutex_lock(&qp->qthread_target_sync_mutex);
			if (qp->qthread_target_sync & QTSYNC_BUSY)  { 
				pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
				qp = qp->next_qp;
				continue;
			}
			// If this is the Destination Side of an E2E then check to see if this target has received its EOF
			if ((qp->target_options & TO_E2E_DESTINATION) && 
				(qp->qthread_target_sync & QTSYNC_EOF_RECEIVED)) {
				eof++;
				if (eof == p->queue_depth) { // This means that all QThreads have received the EOF packets - return 0
					pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
					return(0);
				}
				// This particular QThread has received its EOF so it cannot be used at the moment
				pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
				qp = qp->next_qp;
				continue;
			}

			// Got a QThread - mark it BUSY
			qp->qthread_target_sync |= QTSYNC_BUSY; // Indicate that this QThread is now busy
			pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
			break; // qp now points to the QThread to use
		} // End of WHILE (qp) that scans the QThread Chain looking for the QThread that said it was Available

		// At this point:
		//    The variable "qp" is either 0 or it points to a valid QThread  
		//    If "qp" is non-zero then we will break out of this WHILE loop and return it to the caller
		//    If "qp" is zero it could be due to one of the following:
		//    	(1) All the QThreads are currently busy which is odd because at least one of them set the "any_qthread_available" semaphore...
		//    	(2) All QThreads are either BUSY or have received their EOF packet - this is normal - keep waiting for all the others to receive their EOF
		if ((qp == 0) & !(p->target_options & TO_E2E_DESTINATION)) { // When there are no available QThreads - This should *never* happen - famous last words!
			fprintf(xgp->errout,"%s: xdd_get_any_available_qthread: Target %d: INTERNAL ERROR: Looking for *any qthread available* but did not find one... - going to go wait for another one...\n",
				xgp->progname,
				p->my_target_number);
		}
	} // END of WHILE that looks for *any qthread available*
	return(qp);
} // End of xdd_get_any_available_qthread()

