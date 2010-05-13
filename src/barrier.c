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
//      - QThread
//      - Support Thread like Results Manager, Heartbeat, Restart, or Interactive
//      - XDD Main - the main parent thread
// Each barrier will anchor a doubly linked circular list of "occupant" structures
// that contain information about the thread waiting on that barrier.
// The "occupant" structure contains the occupant's type, a pointer to a 
// PTDS if the occupant is a Target Thread or QThread, and previous/next pointers
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
// have a stalled QThread that needs to be terminated.
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

#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier_chain() - Initialize the barrier chain
 */
int32_t
xdd_init_barrier_chain(void) {
	int32_t status;
	
	status = pthread_mutex_init(&xgp->barrier_chain_mutex, 0);
	if (status) {
		fprintf(stderr,"%s: xdd_init_barrier_chain: ERROR initializing xgp->xdd_barrier_chain_mutex, status=%d", xgp->progname, status);
		perror("Reason");
		return(-1);
	}
	xgp->barrier_chain_first = (NULL);
	xgp->barrier_chain_last = (NULL);
	xgp->barrier_count = 0;
	return(0);
} /* end of xdd_init_barrier_chain() */
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier_occupant() - Initialize the barrier occupant structure
 */
void
xdd_init_barrier_occupant(xdd_occupant_t *bop, char *name, uint32_t type, ptds_t *p) {
	bop->prev_occupant = NULL;
	bop->next_occupant = NULL;
	bop->occupant_name = name;
	bop->occupant_type = type;
	bop->occupant_ptds = p;
	return;
} /* end of xdd_init_barrier_occupant() */
/*----------------------------------------------------------------------------*/
/* xdd_destroy_all_barriers() - Destroy all the barriers that are on the
 * barrier chain.
 */
void
xdd_destroy_all_barriers(void) {
	int32_t status;
	int32_t i;
	xdd_barrier_t *bp;

	i = 0;
	while (xgp->barrier_chain_first) {
		i++;
		/* Get the xdd init barrier mutex so that we can take this barrier off the barrier chain */
		bp = xgp->barrier_chain_first;
		xdd_destroy_barrier(bp);
	}
	status = pthread_mutex_destroy(&xgp->xdd_init_barrier_mutex);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_destroy_all_barriers: ERROR destroying xgp->xdd_init_barrier_mutex, status=%d", xgp->progname, status);
		perror("Reason");
	}
} /* end of xdd_destroy_all_barriers() */
// This section uses SystemV named semaphores to implement barriers
#ifdef SYSV_SEMAPHORES
//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV//SYSTEMV
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier() - Will initialize all the barriers used to synchronize
 * the I/O threads with each other and with the parent thread.
 */
int32_t
xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name) {
	int32_t status; /* status of various system calls */
	char errmsg[512];
	uint16_t zeros[32] = {0};


	/* Init barrier mutex lock */
	bp->threads = threads;
	bp->counter = 0;
	strncpy(bp->name,barrier_name,XDD_BARRIER_NAME_LENGTH-1);
	bp->name[XDD_BARRIER_NAME_LENGTH-1] = '\0';
	status = pthread_mutex_init(&bp->mutex, 0);
	if (status) {
		sprintf(errmsg,"Error initializing %s barrier mutex",barrier_name);
		perror(errmsg);
		bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
		return(-1);
	}
	/* Init barrier semaphore
         * The semid base needs to be unique among all instances of xdd running on a given system.
	 * To make it unique system-wide it will be a 32-bit unsigned integer with the
	 * high-order 16 bits being the lower 16 bits of the process id (P) and the
	 * low order 16 bits being the low order 16 bits of the current barrier count (C)
 	 *    "PPPPPPPPPPPPPPPPCCCCCCCCCCCCCCCC" >> "bit31 .... bit0" of semid_base
	 */
	bp->semid_base = (int32_t)getpid() << 16; // Shift low order 16 bits of PID to high order 16 bits
	bp->semid_base |= (xgp->barrier_count & 0x0000ffff); // Lop off the high order 16 bits 

	bp->semid = semget(bp->semid_base, MAXSEM, IPC_CREAT | 0600);

	if ((int)bp->semid == -1) {
		sprintf(errmsg,"%s: xdd_init_barrier: ERROR: Cannot get semaphore for barrier '%s': semget",xgp->progname, barrier_name);
		perror(errmsg);
		bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
		return(-1);
	}
	status = semctl( bp->semid, MAXSEM, SETALL, zeros);
	bp->flags |= XDD_BARRIER_FLAG_INITIALIZED; // Initialized!!

	/* Get the xdd init barrier mutex so that we can put this barrier on the barrier chain */
	pthread_mutex_lock(&xgp->barrier_chain_mutex);
	if (xgp->barrier_count == 0) { // Add first one to the chain
		xgp->barrier_chain_first = bp;
		xgp->barrier_chain_last = bp;
		bp->prev_barrier = bp;
		bp->next_barrier = bp;
	} else { // Add this barrier to the end of the chain
		bp->next_barrier = xgp->barrier_chain_first; // The last one on the chain points back to the first barrier on the chain as its "next" 
		bp->prev_barrier = xgp->barrier_chain_last;
		xgp->barrier_chain_last->next_barrier = bp;
		xgp->barrier_chain_last = bp;
	} // Done adding this barrier to the chain
	xgp->barrier_count++;
	pthread_mutex_unlock(&xgp->barrier_chain_mutex);
	return(0);
} /* end of xdd_init_barrier() */
/*----------------------------------------------------------------------------*/
/* xdd_destroy_barrier() - Will destroy all the barriers and semphores.
 * The barrier counter variable (bp->counter) is normally set to zero or some
 * positive value once they have been initialized. Only barriers that have been 
 * initialized can be destroyed. If the counter variable is -1 then it has not
 * been initialized or has already been destroyed and should not be destroyed
 * again. Upcon destroying this barrier, this routine will set bp->counter to -1.
 */
void
xdd_destroy_barrier(struct xdd_barrier *bp) {
	int32_t		status; 			// Status of various system calls
	uint16_t 	zeros[32] = {0}; 	// Required bu semctl


	if ((bp->flags &= XDD_BARRIER_FLAG_INITIALIZED) == 0) 
		return;

	bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
	if (bp->counter == -1) // Barrier is already destroyed?
		return;
	status = pthread_mutex_destroy(&bp->mutex);
	if (status) {
		fprintf(xgp->errout,"%s: ERROR: xdd_destroy_barrier<SysV>: Error destroying mutex for barrier '%s'",
			xgp->progname,bp->name);
		perror("Reason");
	}
	status = semctl( bp->sem, MAXSEM, IPC_RMID, zeros);
	if (status == -1) {
		fprintf(xgp->errout,"%s: ERROR: xdd_destroy_barrier<SysV>: semctl error destroying barrier '%s'",
			xgp->progname,bp->name);
		perror("Reason");
	}
	bp->counter = -1;
	bp->sem = 0;
	strcpy(bp->name,"DESTROYED");
	// Remove this barrier from the chain and relink the barrier before this one to the barrier after this one
	pthread_mutex_lock(&xgp->barrier_chain_mutex);
	if (xgp->barrier_count == 1) { // This is the last barrier on the chain
		xgp->barrier_chain_first = NULL;
		xgp->barrier_chain_last = NULL;
	} else { // Remove this barrier from the chain
		if (xgp->barrier_chain_first == bp) 
			xgp->barrier_chain_first = bp->next_barrier;
		if (xgp->barrier_chain_last == bp) 
			xgp->barrier_chain_last = bp->prev_barrier;
		if (bp->prev_barrier) 
			bp->prev_barrier->next_barrier = bp->next_barrier;
		if (bp->next_barrier) 
			bp->next_barrier->prev_barrier = bp->prev_barrier;
	}
	bp->prev_barrier = NULL;
	bp->next_barrier = NULL;
	xgp->barrier_count--;
	pthread_mutex_unlock(&xgp->barrier_chain_mutex);
} // End of xdd_destroy_barrier() 
/*----------------------------------------------------------------------------*/
/* xdd_barrier() - This is the actual barrier subroutine. 
 * The caller will block in this subroutine until all required threads enter
 * this subroutine <barrier> at which time they will all be released.
 * THIS SUBROUTINE IMPLEMENTS BARRIERS USING SYSTEMV SEMAPHORES
 */
int32_t
xdd_barrier(struct xdd_barrier *bp) {
	struct sembuf sb; /* Semaphore operation buffer */
	volatile int32_t local_counter = 0; /* copy of the barrier counter */
	int32_t threadsm1;  /* this is the number of threads minus 1 */
	int32_t status;  /* status of the semop system call */


	/* "threads" is the number of participating threads */
	threadsm1 = bp->threads - 1;
	if (threadsm1 == 0) return(0); /* If there is only one thread then why bother sleeping */
	/* increment the semaphore counter in this barrier */
	pthread_mutex_lock(&bp->mutex);
	local_counter = bp->counter;
	bp->counter++;
	/* If the local counter (before incrementing) is equal to the 
	 * number of threads minus 1 then all the threads have arrived
	 * at this barrier. Therefore, the sem_op can be set to the
	 * number of threads minus 1 and added to the semaphore counter
	 * where it will equal zero and all sleeping threads will be woken up.
	 * Otherwise, set the sem_op to -1 which will decrement the 
	 * semaphore counter one more time.
	 */
	if (local_counter == threadsm1) {
		bp->counter = 0;
		sb.sem_op = threadsm1;
	} else sb.sem_op = -1;
	pthread_mutex_unlock(&bp->mutex);
	/* set up the semaphore sturcture */
	sb.sem_flg = 0;
	sb.sem_num = 0;
	/* issue the semaphore operation */
	status = semop(bp->semid, &sb, 1);
	if (status) {
		if (errno == EIDRM) /* ignore the fact that it has been removed */
			return(0);
		fprintf(xgp->errout,"%s: xdd_barrier: ERROR: semop failed on semaphore '%s', status is %d, errno is %d, threadsm1=%d\n", 
			xgp->progname, 
			bp->name, 
			status, 
			errno, 
			threadsm1);
		fflush(xgp->errout);
		perror("Error on semop");
		return(-1);
	}
	return(0);
} /* end of xdd_barrier() */
// End of SYSTEMV Semaphore code

#else // Use POSIX pthread_barriers by default
////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section uses POSIX pthread_barriers to implement barriers
/*----------------------------------------------------------------------------*/
/* xdd_init_barrier() - Will initialize the specified pthread_barrier
 */
int32_t
xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name) {
	int32_t 		status; 			// status of various system calls 


	/* Init barrier mutex lock */
	bp->flags = 0;
	bp->threads = threads;
	bp->counter = 0;
	strncpy(bp->name,barrier_name,XDD_BARRIER_NAME_LENGTH-1);
	bp->name[XDD_BARRIER_NAME_LENGTH-1] = '\0';
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
	status = pthread_barrier_init(&bp->pbar, 0, threads);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_init_barrier<pthread_barriers>: ERROR in pthread_barrier_init for barrier '%s', status=%d",
			xgp->progname, barrier_name, status);
		perror("Reason");
		bp->flags &= ~XDD_BARRIER_FLAG_INITIALIZED; // NOT initialized!!!
		return(-1);
	}
	bp->flags |= XDD_BARRIER_FLAG_INITIALIZED; // Initialized!!!

	/* Get the xdd init barrier mutex so that we can put this barrier on the barrier chain */
	pthread_mutex_lock(&xgp->barrier_chain_mutex);
	if (xgp->barrier_count == 0) { // Add first one to the chain
		xgp->barrier_chain_first = bp;
		xgp->barrier_chain_last = bp;
		bp->prev_barrier = bp;
		bp->next_barrier = bp;
	} else { // Add this barrier to the end of the chain
		bp->next_barrier = xgp->barrier_chain_first; // The last one on the chain points back to the first barrier on the chain as its "next" 
		bp->prev_barrier = xgp->barrier_chain_last;
		xgp->barrier_chain_last->next_barrier = bp;
		xgp->barrier_chain_last = bp;
	} // Done adding this barrier to the chain
	xgp->barrier_count++;
	pthread_mutex_unlock(&xgp->barrier_chain_mutex);
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
xdd_destroy_barrier(struct xdd_barrier *bp) {
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
	status = pthread_barrier_destroy(&bp->pbar);
	if (status && !(xgp->global_options & GO_INTERACTIVE_EXIT)) { // If this is an exit requested by the interactive debugger then do not display error messages...
		fprintf(xgp->errout,"%s: xdd_destroy_barrier<pthread_barriers>: ERROR: pthread_barrier_destroy: errno %d destroying barrier '%s'\n",
			xgp->progname,status,bp->name);
		errno = status; // Set the errno
		perror("Reason");
	}
	bp->counter = -1;
	strcpy(bp->name,"DESTROYED");
	// Remove this barrier from the chain and relink the barrier before this one to the barrier after this one
	pthread_mutex_lock(&xgp->barrier_chain_mutex);
	if (xgp->barrier_count == 1) { // This is the last barrier on the chain
		xgp->barrier_chain_first = NULL;
		xgp->barrier_chain_last = NULL;
	} else { // Remove this barrier from the chain
		if (xgp->barrier_chain_first == bp) 
			xgp->barrier_chain_first = bp->next_barrier;
		if (xgp->barrier_chain_last == bp) 
			xgp->barrier_chain_last = bp->prev_barrier;
		if (bp->prev_barrier) 
			bp->prev_barrier->next_barrier = bp->next_barrier;
		if (bp->next_barrier) 
			bp->next_barrier->prev_barrier = bp->prev_barrier;
	}
	bp->prev_barrier = NULL;
	bp->next_barrier = NULL;
	xgp->barrier_count--;
	pthread_mutex_unlock(&xgp->barrier_chain_mutex);
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
 * If the barrier is a Target Thread or a QThread then the PTDS pointer is
 * valid and the "current_barrier" member of that PTDS is set to the barrier
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

	// Put this PTDS on the Barrier PTDS Chain so that we can track it later if we need to 
	/////// this is to keep track of which PTDSs are in a particular barrier at any given time...
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
	if (occupantp->occupant_type & (XDD_OCCUPANT_TYPE_TARGET | XDD_OCCUPANT_TYPE_QTHREAD)) {
		// Put the barrier pointer into this thread's PTDS->current_barrier
		occupantp->occupant_ptds->my_current_state |= CURRENT_STATE_BARRIER;
		occupantp->occupant_ptds->current_barrier = bp;
	}
	bp->counter++;
	pthread_mutex_unlock(&bp->mutex);

	// Now we wait here at this barrier until all the other threads arrive...
	pclk_now(&occupantp->entry_time);
	status = pthread_barrier_wait(&bp->pbar);
	pclk_now(&occupantp->exit_time);

	if ((status != 0) && (status != PTHREAD_BARRIER_SERIAL_THREAD)) {
		fprintf(xgp->errout,"%s: xdd_barrier<pthread_barriers>: ERROR: pthread_barrier_wait failed: Barrier %s, status is %d, errno is %d\n", 
			xgp->progname, bp->name, status, errno);
		perror("Reason");
		status = -1;
	}
	if (occupantp->occupant_type & (XDD_OCCUPANT_TYPE_TARGET | XDD_OCCUPANT_TYPE_QTHREAD)) {
		// Clear this thread's PTDS->current_barrier
		occupantp->occupant_ptds->current_barrier = NULL;
		occupantp->occupant_ptds->my_current_state &= ~CURRENT_STATE_BARRIER;
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
#endif
