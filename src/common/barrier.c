/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines that provide the barrier functions required
 * by xdd to run.
 */
// Barrier Naming Convention: 
//    (1) The first part of the name is the barrier "owner" or thread that initializes the barrier
//    (2) The second part of the name is the other [dependant] thread that will use this barrier
//    (3) The third part of the name is a descriptive word about the function of this barrier
//    (4) The fourth part of the name is the word "barrier" to indicate what it is
// It is important to use the xdd_init_barrier() subroutine to initialize all barriers within XDD
// because it keeps track of all initialized barriers so that they can be properly shut down
// upon any exit from XDD. This prevents an unwanted accumulation of zombie semaphores lying about.
//
// About the Barrier Chain
// The barrier chain is used to keep track of all barriers that are created so that we can easily scan
// through the list during termination to make sure all of them have been removed properly.
// Barriers are added to the chain when they are initialized in xdd_barrier_init(). 
// Barriers are removed from the chain when they are destrroyed in xdd_barrier_destroy().
// The xdd_global_data structure contains a pointer to the first barrier on the chain, the last barrier
// on the chain, and the number of barriers on the chain at any given time.
// Each barrier structure contains a pointer to the barrier before it (the *prev* member) and a pointer
// to the barrier after it (the *next* member) in the chain. 
// At any given time the chain is a circular doubly-linked list that looks like so:
//                           (the last barrier)
//                            ^
//xgp->barrier_chain_first>+--|--------------+
//                         | prev            |
//                         | Barrier A       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier B       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier C       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier D       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//xgp->barrier_chain_last->+--|--------------+
//                         | prev            |
//                         | Barrier E       |
//                         |         next    |
//                         +-----------|-----+
//                                     V
//                                (the first barrier)
// 
// Barriers are always added to the end of the chain.
// Barriers can be removed from the chain in any order.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// During normal execution a barrier will have some number of "occupants" or 
// threads that are waiting at a particular barrier for other threads to arrive.
// The occupants waiting in a barrier are of four different types:
//      - Target Thread
//      - Worker Thread
//      - Support Thread like Results Manager, Heartbeat, Restart, or Interactive
//      - XDD Main - the main parent thread
// Each barrier will anchor a doubly linked circular list of "occupant" structures
// that contain information about the thread waiting on that barrier.
// The "occupant" structure contains the occupant's type, a pointer to a 
// Target Data Struct if the occupant is a Target Thread or Worker Thread, and previous/next pointers
// to the occupants in front or behind any given occupant structure. If the
// occupant is the only one on the occupant chain then it simply points to itself.
// Each time a thread enters a barrier, the pointer to the occupant structure passed 
// in as an argument to xdd_barrier() is added to the end of the occupant chain.
// Similarly, when the barrier is released, the occupant chain is cleared by the
// thread that caused the barrier to open. 
//
// The reason for the "occupant" chain is to be able to figure out at any
// given time if there are threads of any sort *stuck* in a barrier so that
// something might be done about releasing them without necessarily terminating
// the XDD run. This is useful for copy (aka end-to-end) operations that might
// have a stalled Worker Thread that needs to be terminated.
//
// The "occupant" chain is very similar to the barrier chain in that it is
// a circular doubly-linked list of "occupant" structures. Each barrier
// contains an "anchor" for its occupant chain that consists of a pair of
// pointers to the first and last occupant structure on the chain. New occupant
// structures are always added to the end of the chain. Occupant structures
// can be removed from anywhere in the chain but under normal operating conditions
// when the final occupant enters the barrier, the owner of the barrier will 
// simply remove the first/last pointers to the chain effectively taking all the
// occupants off the chain at once (rather than removing them individually).
// The "counter" member of the barrier structure indicates the number of occupants
// in the barrier at any given time. 
// The "threads" member of the barrier structure indicates the number of occupants 
// that must enter the barrier before all the occupants are released.
//

#include "xint.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier_chain() - Initialize the barrier chain
 */
int32_t
xdd_init_barrier_chain(xdd_plan_t* planp) {
	int32_t status;
	
	status = pthread_mutex_init(&planp->barrier_chain_mutex, 0);
	if (status) {
		fprintf(stderr,"%s: xdd_init_barrier_chain: ERROR initializing xgp->xdd_barrier_chain_mutex, status=%d", xgp->progname, status);
		perror("Reason");
		return(-1);
	}
	planp->barrier_chain_first = (NULL);
	planp->barrier_chain_last = (NULL);
	planp->barrier_count = 0;
	return(0);
} /* end of xdd_init_barrier_chain() */
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier_occupant() - Initialize the barrier occupant structure
 */
void
xdd_init_barrier_occupant(xdd_occupant_t *bop, char *name, uint32_t type, void *datap) {
	bop->prev_occupant = NULL;
	bop->next_occupant = NULL;
	bop->occupant_name = name;
	bop->occupant_type = type;
	bop->occupant_data = datap;
	return;
} /* end of xdd_init_barrier_occupant() */
/*----------------------------------------------------------------------------*/
/* xdd_destroy_all_barriers() - Destroy all the barriers that are on the
 * barrier chain.
 */
void
xdd_destroy_all_barriers(xdd_plan_t* planp) {
	int32_t i;
	xdd_barrier_t *bp;

	i = 0;
	while (planp->barrier_chain_first) {
		i++;
		/* Get the xdd init barrier mutex so that we can take this barrier off the barrier chain */
		bp = planp->barrier_chain_first;
		xdd_destroy_barrier(planp, bp);
	}
} /* end of xdd_destroy_all_barriers() */

////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section uses POSIX pthread_barriers to implement barriers
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier() - Will initialize the specified pthread_barrier
 */
int32_t
xdd_init_barrier(xdd_plan_t* planp, struct xdd_barrier *bp, int32_t threads, char *barrier_name) {
	int32_t 		status; 			// status of various system calls 


	/* Init barrier mutex lock */
	bp->flags = 0;
	bp->threads = threads;
	bp->counter = 0;
	strncpy(bp->name,barrier_name,XDD_BARRIER_MAX_NAME_LENGTH-1);
	bp->name[XDD_BARRIER_MAX_NAME_LENGTH-1] = '\0';
	bp->first_occupant = NULL;
	bp->last_occupant = NULL;

	status = pthread_mutex_init(&bp->mutex, 0);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_init_barrier<pthread_barriers>: ERROR initializing mutex for barrier '%s', status=%d",
			xgp->progname, barrier_name, status);
		perror("Reason");
		bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
		return(-1);
	}
	// Init barrier
#ifdef HAVE_PTHREAD_BARRIER_T
	status = pthread_barrier_init(&bp->pbar, 0, threads);
#else
	status = xint_barrier_init(&bp->pbar, threads);
#endif
	if (status) {
		fprintf(xgp->errout,"%s: xdd_init_barrier<pthread_barriers>: ERROR in pthread_barrier_init for barrier '%s', status=%d",
			xgp->progname, barrier_name, status);
		perror("Reason");
		bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
		return(-1);
	}
	bp->flags |= XDD_BARRIER_FLAG_INITIALIZED; // Initialized!!!

	/* Get the xdd init barrier mutex so that we can put this barrier on the barrier chain */
	pthread_mutex_lock(&planp->barrier_chain_mutex);
	if (planp->barrier_count == 0) { // Add first one to the chain
		planp->barrier_chain_first = bp;
		planp->barrier_chain_last = bp;
		bp->prev_barrier = bp;
		bp->next_barrier = bp;
	} else { // Add this barrier to the end of the chain
		bp->next_barrier = planp->barrier_chain_first; // The last one on the chain points back to the first barrier on the chain as its "next" 
		bp->prev_barrier = planp->barrier_chain_last;
		planp->barrier_chain_last->next_barrier = bp;
		planp->barrier_chain_last = bp;
	} // Done adding this barrier to the chain
	planp->barrier_count++;
	pthread_mutex_unlock(&planp->barrier_chain_mutex);
	return(0);
} // End of xdd_init_barrier() POSIX

/*----------------------------------------------------------------------------*/
/* xdd_destroy_barrier() - Will destroy all the barriers and mutex locks.
 * The barrier counter variable (bp->counter) is normally set to zero or some
 * positive value once they have been initialized. Only barriers that have been 
 * initialized can be destroyed. If the counter variable is -1 then it has not
 * been initialized or has already been destroyed and should not be destroyed
 * again. Upcon destroying this barrier, this routine will set bp->counter to -1.
 */
void
xdd_destroy_barrier(xdd_plan_t* planp, struct xdd_barrier *bp) {
	int32_t		status; 			// Status of various system calls

	if ((bp->flags &= XDD_BARRIER_FLAG_INITIALIZED) == 0) 
		return;

	bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
	if (bp->counter == -1) // Barrier is already destroyed?
		return;
	status = pthread_mutex_destroy(&bp->mutex);
	if (status && !(xgp->global_options & GO_INTERACTIVE_EXIT)) { // If this is an exit requested by the interactive debugger then do not display error messages...
		fprintf(xgp->errout,"%s: xdd_destroy_barrier<pthread_barriers>: ERROR: pthread_mutex_destroy: errno %d destroying mutex for barrier '%s'\n",
			xgp->progname, status, bp->name);
		errno = status; // Set the errno
		perror("Reason");
	}
#ifdef HAVE_PTHREAD_BARRIER_T
	status = pthread_barrier_destroy(&bp->pbar);
#else
	status = xint_barrier_destroy(&bp->pbar);
#endif
	if (status && !(xgp->global_options & GO_INTERACTIVE_EXIT)) { // If this is an exit requested by the interactive debugger then do not display error messages...
		fprintf(xgp->errout,"%s: xdd_destroy_barrier<pthread_barriers>: ERROR: pthread_barrier_destroy: errno %d destroying barrier '%s'\n",
			xgp->progname,status,bp->name);
		errno = status; // Set the errno
		perror("Reason");
	}
	bp->counter = -1;
	strcpy(bp->name,"DESTROYED");
	// Remove this barrier from the chain and relink the barrier before this one to the barrier after this one
	pthread_mutex_lock(&planp->barrier_chain_mutex);
	if (planp->barrier_count == 1) { // This is the last barrier on the chain
		planp->barrier_chain_first = NULL;
		planp->barrier_chain_last = NULL;
	} else { // Remove this barrier from the chain
		if (planp->barrier_chain_first == bp) 
			planp->barrier_chain_first = bp->next_barrier;
		if (planp->barrier_chain_last == bp) 
			planp->barrier_chain_last = bp->prev_barrier;
		if (bp->prev_barrier) 
			bp->prev_barrier->next_barrier = bp->next_barrier;
		if (bp->next_barrier) 
			bp->next_barrier->prev_barrier = bp->prev_barrier;
	}
	bp->prev_barrier = NULL;
	bp->next_barrier = NULL;
	planp->barrier_count--;
	pthread_mutex_unlock(&planp->barrier_chain_mutex);
	// There, I think we're done...
} // End of xdd_destroy_barrier() POSIX
// 							PTHREAD BARRIERS 
/*----------------------------------------------------------------------------*/
/* xdd_barrier() - This is the actual barrier subroutine. 
 * The caller will block in this subroutine until all required threads enter
 * this subroutine <barrier> at which time they will all be released.
 * 
 * The "owner" parameter indicates whether or not the calling thread is the
 * owner of this barrier. 0==NOT owner, 1==owner. If this thread owns this
 * barrier then it will be responsible for removing the "occupant" chain 
 * upon being released from this barrier.
 *
 * Before entering the barrier, the occupant structure is added to the end
 * of the occupant chain. This allows the debug routine to see which threads
 * are in a barrier at any given time as well as when they entered the barrier.
 * 
 * If the barrier is a Target Thread or a Worker Thread then the Target_Data pointer is
 * valid and the "current_barrier" member of that Target_Data is set to the barrier
 * pointer of the barrier that this thread is about to enter. Upon leaving the
 * barrier, this pointer is cleared.
 *
 * THIS SUBROUTINE IMPLEMENTS BARRIERS USING PTHREAD_BARRIERS 
 */
int32_t
xdd_barrier(struct xdd_barrier *bp, xdd_occupant_t *occupantp, char owner) {
	int32_t 	status;  			// Status of the pthread_barrier call 


	/* "threads" is the number of participating threads */
	if (bp->threads == 1) return(0); /* If there is only one thread then why bother sleeping */

	// Put this Target_Data on the Barrier Target_Data Chain so that we can track it later if we need to 
	/////// this is to keep track of which Target_Data are in a particular barrier at any given time...
	pthread_mutex_lock(&bp->mutex);
	// Add occupant structure here
	if (bp->counter == 0) { // Add first occupant to the chain
		bp->first_occupant = occupantp;
		bp->last_occupant = occupantp;
		occupantp->prev_occupant = occupantp;
		occupantp->next_occupant = occupantp;
	} else { // Add this barrier to the end of the chain
		occupantp->next_occupant = bp->first_occupant; // The last one on the chain points back to the first barrier on the chain as its "next" 
		occupantp->prev_occupant = bp->last_occupant;
		bp->last_occupant->next_occupant = occupantp;
		bp->last_occupant = occupantp;
	} // Done adding this barrier to the chain
	if (occupantp->occupant_type & XDD_OCCUPANT_TYPE_TARGET ) {
		// Put the barrier pointer into this thread's Target_Data->current_barrier
		((target_data_t *)(occupantp->occupant_data))->td_current_state |= TARGET_CURRENT_STATE_BARRIER;
		((target_data_t *)(occupantp->occupant_data))->td_current_barrier = bp;
	} else if (occupantp->occupant_type & XDD_OCCUPANT_TYPE_WORKER_THREAD) {
		// Put the barrier pointer into this thread's Worker_Data->current_barrier
		((worker_data_t *)(occupantp->occupant_data))->wd_current_state |= WORKER_CURRENT_STATE_BARRIER;
		((worker_data_t *)(occupantp->occupant_data))->wd_current_barrier = bp;
	}
	
	bp->counter++;
	pthread_mutex_unlock(&bp->mutex);

	// Now we wait here at this barrier until all the other threads arrive...
	nclk_now(&occupantp->entry_time);

#ifdef HAVE_PTHREAD_BARRIER_T
	status = pthread_barrier_wait(&bp->pbar);
	nclk_now(&occupantp->exit_time);
	if ((status != 0) && (status != PTHREAD_BARRIER_SERIAL_THREAD)) {
		fprintf(xgp->errout,"%s: xdd_barrier<pthread_barriers>: ERROR: pthread_barrier_wait failed: Barrier %s, status is %d, errno is %d\n", 
			xgp->progname, bp->name, status, errno);
		perror("Reason");
		status = -1;
	}
#else
	status = xint_barrier_wait(&bp->pbar);
	nclk_now(&occupantp->exit_time);
	if (status != 0) {
		fprintf(xgp->errout,"%s: xdd_barrier<pthread_barriers>: ERROR: xint_barrier_wait failed: Barrier %s, status is %d, errno is %d\n", 
			xgp->progname, bp->name, status, errno);
		perror("Reason");
		status = -1;
	}
#endif

	if (occupantp->occupant_type & XDD_OCCUPANT_TYPE_TARGET ) {
		// Clear this thread's Target_Data->current_barrier
		((target_data_t *)(occupantp->occupant_data))->td_current_barrier = NULL;
		((target_data_t *)(occupantp->occupant_data))->td_current_state &= ~TARGET_CURRENT_STATE_BARRIER;
	} else if (occupantp->occupant_type & XDD_OCCUPANT_TYPE_WORKER_THREAD) {
		// Put the barrier pointer into this thread's Worker_Data->current_barrier
		((worker_data_t *)(occupantp->occupant_data))->wd_current_barrier = NULL;
		((worker_data_t *)(occupantp->occupant_data))->wd_current_state &= ~WORKER_CURRENT_STATE_BARRIER;
	}
	// Clear this occupant chain if we are the owner of this barrier
	if (owner) {
		pthread_mutex_lock(&bp->mutex);
		// Clear the first/last chain pointers
		bp->first_occupant = NULL;
		bp->last_occupant = NULL;
		bp->counter = 0;
		pthread_mutex_unlock(&bp->mutex);
	}
	return(status);
} // End of xdd_barrier() POSIX
// End of POSIX pthread_barrier code

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
