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
	ptds_t *qp;					// Pointer to a QThread PTDS
	int i;
	nclk_t checktime;

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
            // Set the "target waiting" bit, unlock the mutex,
            // and lets wait for this QThread to become available - i.e. not busy
            p->my_current_state |= CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
            qp->qthread_target_sync |= QTSYNC_TARGET_WAITING;
            nclk_now(&checktime);

            while (qp->qthread_target_sync & QTSYNC_BUSY) {
                pthread_cond_wait(&qp->this_qthread_is_available_condition,
                                  &qp->qthread_target_sync_mutex);            
            }
            p->my_current_state &= ~CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE;
        }
        
        // Indicate that this QThread is now busy, and unlock
        qp->qthread_target_sync |= QTSYNC_BUSY; 
        pthread_mutex_unlock(&qp->qthread_target_sync_mutex);

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
    ptds_t		*qp; // Pointer to a QThread PTDS
    int			eof;	// Number of QThreads that have reached End-of-File on the destination side of an E2E operation        

    // Use a polling strategy to find available qthreads -- this might be a good
    // candidate for asynchronous messages in the future
    pthread_mutex_lock(&p->any_qthread_available_mutex);
    while (p->any_qthread_available <= 0) {
        p->my_current_state |= CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
        pthread_cond_wait(&p->any_qthread_available_condition, &p->any_qthread_available_mutex);
        p->my_current_state &= ~CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE;
    }
    p->any_qthread_available--;
        
    // Locate an idle qthread
    qp = p->next_qp;
    while (qp) {
        pthread_mutex_lock(&qp->qthread_target_sync_mutex);
        // Ignore busy qthreads
        if (qp->qthread_target_sync & QTSYNC_BUSY) {
            pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
            qp = qp->next_qp;
            continue;
        }

        // Ignore e2e threads that have received their eof
        if ((qp->target_options & TO_E2E_DESTINATION) && 
            (qp->qthread_target_sync & QTSYNC_EOF_RECEIVED)) {
            eof++;
            pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
            qp = qp->next_qp;
            
            // Multi-level short circuit hack for when all threads
            // have recieved an EOF
            if (eof == p->queue_depth) {
                qp = 0;
                goto unlock_and_return;
            }
        }
        else {
            // Got a QThread - mark it BUSY
            // Indicate that this QThread is now busy
            qp->qthread_target_sync |= QTSYNC_BUSY; 
            pthread_mutex_unlock(&qp->qthread_target_sync_mutex);
            break;
        }
    }

  unlock_and_return:
    pthread_mutex_unlock(&p->any_qthread_available_mutex);
    return qp;        
} // End of xdd_get_any_available_qthread()

