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
 * This file contains the subroutine that locates an available Worker Thread for
 * a specific target.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_get_specific_worker_thread() - This subroutine will locate the specified
 * Worker Thread and wait for it to become available then return its pointer.
 * This subroutine is called by xdd_target_pass()
 */
worker_data_t *
xdd_get_specific_worker_thread(target_data_t *tdp, int32_t q) {
	worker_data_t *wdp;					// Pointer to a Worker Thread Data Struct
	int i;
	nclk_t checktime;

	nclk_now(&checktime);

	// Sanity Check
	if (q >= tdp->td_queue_depth) { // This should *NEVER* happen - famous last words...
		fprintf(xgp->errout,"%s: xdd_get_specified_worker_thread: Target %d: INTERNAL ERROR: Specified Worker Thread is out of range: q=%d\n",
			xgp->progname,
			tdp->td_target_number,
			q);
		exit(1);
	}
	
	// Locate the pointer to the requested Worker Thread Data Struct
	// The Worker Threads are on an ordered linked list anchored on the Target Thread Data Struct 
	wdp = tdp->td_next_wdp;
	for (i=0; i<q; i++) 
		wdp = wdp->wd_next_wdp;
	// wdp should now point to the desired Worker Thread

	// Wait for this specific Worker Thread to become available
	pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
	if (wdp->wd_worker_thread_target_sync & WTSYNC_BUSY) { 
            // Set the "target waiting" bit, unlock the mutex,
            // and lets wait for this Worker Thread to become available - i.e. not busy
            tdp->td_current_state |= CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE;
            wdp->wd_worker_thread_target_sync |= WTSYNC_TARGET_WAITING;
            nclk_now(&checktime);

            while (wdp->wd_worker_thread_target_sync & WTSYNC_BUSY) {
                pthread_cond_wait(&wdp->wd_this_worker_thread_is_available_condition,
                                  &wdp->wd_worker_thread_target_sync_mutex);            
            }
            tdp->td_current_state &= ~CURRENT_STATE_WAITING_THIS_WORKER_THREAD_AVAILABLE;
        }
        
        // Indicate that this Worker Thread is now busy, and unlock
        wdp->wd_worker_thread_target_sync |= WTSYNC_BUSY; 
        pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);

	// At this point we have a pointer to the specified Worker Thread
	return(wdp);

} // End of  xdd_get_specific_worker_thread()

/*----------------------------------------------------------------------------*/
/* xdd_get_any_available_worker_thread() - This subroutine will scan the list of
 * Worker Threads and return a pointer to a Worker Thread that is available to be assigned a task.
 * This subroutine is called by xdd_target_pass()
 */
worker_data_t *
xdd_get_any_available_worker_thread(target_data_t *tdp) {
    worker_data_t		*wdp; // Pointer to a Worker Thread Data Struct
    int eof;	// Number of Worker Threads that have reached End-of-File on the destination side of an E2E operation        

    // Use a polling strategy to find available worker_threads -- this might be a good
    // candidate for asynchronous messages in the future    
    wdp = 0;
    eof = 0;
    while (0 == wdp && eof != tdp->td_queue_depth) {
		pthread_mutex_lock(&tdp->td_any_worker_thread_available_mutex);
		while (tdp->td_any_worker_thread_available <= 0) {
	    	tdp->td_current_state |= CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE;
	    	pthread_cond_wait(&tdp->td_any_worker_thread_available_condition, &tdp->td_any_worker_thread_available_mutex);
	    	tdp->td_current_state &= ~CURRENT_STATE_WAITING_ANY_WORKER_THREAD_AVAILABLE;
		}
		tdp->td_any_worker_thread_available--;
        
		// Locate an idle worker_thread
		wdp = tdp->td_next_wdp;
    	eof = 0;
		while (wdp) {
	    	pthread_mutex_lock(&wdp->wd_worker_thread_target_sync_mutex);
	    	// Ignore busy worker threads
	    	if (wdp->wd_worker_thread_target_sync & WTSYNC_BUSY) {
				pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
				wdp = wdp->wd_next_wdp;
				continue;
	    	}

	    	// Ignore e2e threads that have received their eof
	    	if ((tdp->td_target_options & TO_E2E_DESTINATION) && 
				(wdp->wd_worker_thread_target_sync & WTSYNC_EOF_RECEIVED)) {
				eof++;
				// Multi-level short circuit hack for when all threads
				// have recieved an EOF
				pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
				if (eof == tdp->td_queue_depth) {
		    		wdp = 0;
		    		break;
				}
				wdp = wdp->wd_next_wdp;            
	    	} else {
				// Got a Worker Thread - mark it BUSY
				// Indicate that this Worker Thread is now busy
				wdp->wd_worker_thread_target_sync |= WTSYNC_BUSY; 
				pthread_mutex_unlock(&wdp->wd_worker_thread_target_sync_mutex);
				break;
	    	}
		}
	//unlock_and_return:	    
	pthread_mutex_unlock(&tdp->td_any_worker_thread_available_mutex);
    }


    return wdp;        
} // End of xdd_get_any_available_worker_thread()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
