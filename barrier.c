/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *      Tom Ruwart (tmruwart@ioperformance.com)
 *      I/O Perofrmance, Inc.
 */
/*
 * This file contains the subroutines that provide the barrier functions required
 * by xdd to run.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
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
		fprintf(xgp->errout,"%s: Error on semaphore %s, status is %d, errno is %d, threadsm1=%d\n", xgp->progname, bp->name, status, errno, threadsm1);
		fflush(xgp->errout);
		perror("Error on semop");
		return(-1);
	}
	return(0);
} /* end of xdd_barrier() */
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
	int32_t status; /* status of various system calls */
	char errmsg[512];
	uint16_t zeros[32] = {0};


	bp->initialized = FALSE;
	if (bp->counter == -1) /* Barrier is already destroyed */
		return;
	status = pthread_mutex_destroy(&bp->mutex);
	if (status) {
		sprintf(errmsg,"Error destroying %s barrier mutex",bp->name);
		perror(errmsg);
	}
	status = semctl( bp->semid, MAXSEM, IPC_RMID, zeros);
	if (status == -1) {
			sprintf(errmsg,"Error Destroying %s barrier semaphore: semctl",bp->name);
			perror(errmsg);
	}
	bp->counter = -1;
	bp->semid = 0;
	bp->name[0]= 0;
} /* end of xdd_destroy_barrier() */
/*----------------------------------------------------------------------------*/
/* xdd_destroy_all_barriers() - Destroy all the thread barriers
 */
void
xdd_destroy_all_barriers(void) {
	int32_t status;
	int32_t i;
	char errmsg[512];
	xdd_barrier_t *bp;


	i = 0;
	while (xgp->barrier_chain) {
		i++;
		/* Get the xdd init barrier mutex so that we can take this barrier off the barrier chain */
		pthread_mutex_lock(&xgp->xdd_init_barrier_mutex);
		bp = xgp->barrier_chain;
		bp->prev = (NULL);
		xgp->barrier_chain = bp->next;
		bp->next = (NULL);
		xgp->barrier_count--;
		pthread_mutex_unlock(&xgp->xdd_init_barrier_mutex);
		xdd_destroy_barrier(bp);
	}
	status = pthread_mutex_destroy(&xgp->xdd_init_barrier_mutex);
	if (status) {
		sprintf(errmsg,"%s: xdd_destroy_all_barriers: Error destroying xgp->xdd_init_barrier_mutex, status=%d", xgp->progname, status);
		perror(errmsg);
	}
} /* end of xdd_destroy_all_barriers() */
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
	strcpy(bp->name, barrier_name);
	status = pthread_mutex_init(&bp->mutex, 0);
	if (status) {
		sprintf(errmsg,"Error initializing %s barrier mutex",barrier_name);
		perror(errmsg);
		bp->initialized = FALSE;
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
		sprintf(errmsg,"Error getting %s barrier semaphore: semget",barrier_name);
		perror(errmsg);
		bp->initialized = FALSE;
		return(-1);
	}
	status = semctl( bp->semid, MAXSEM, SETALL, zeros);
	bp->initialized = TRUE;

	/* Get the xdd init barrier mutex so that we can put this barrier on the barrier chain */
	pthread_mutex_lock(&xgp->xdd_init_barrier_mutex);
	if (xgp->barrier_chain)
		xgp->barrier_chain->prev = bp;
	bp->next = xgp->barrier_chain;
	bp->prev = (NULL);
	xgp->barrier_chain = bp;
	xgp->barrier_count++;
	pthread_mutex_unlock(&xgp->xdd_init_barrier_mutex);
	return(0);
} /* end of xdd_init_barrier() */
/*----------------------------------------------------------------------------*/
/* xdd_init_all_barriers() - Initialize all the thread barriers
 */
int32_t
xdd_init_all_barriers(void) {
	int32_t status;
	char errmsg[512];
	int i,j;
	
	status = pthread_mutex_init(&xgp->xdd_init_barrier_mutex, 0);
	if (status) {
		sprintf(errmsg,"%s: xdd_init_all_barriers: Error initializing xgp->xdd_init_barrier_mutex, status=%d", xgp->progname, status);
		perror(errmsg);
		return(-1);
	}
	xgp->barrier_chain = (NULL);
	xgp->barrier_count = 0;
	status =   xdd_init_barrier(&xgp->thread_barrier[0], xgp->number_of_iothreads, "Thread0");
	status +=  xdd_init_barrier(&xgp->thread_barrier[1], xgp->number_of_iothreads, "Thread1");
	status += xdd_init_barrier(&xgp->cleanup_barrier, xgp->number_of_iothreads, "CleanUp");
	status += xdd_init_barrier(&xgp->final_barrier, xgp->number_of_iothreads+1, "Final");
	status +=  xdd_init_barrier(&xgp->syncio_barrier[0], xgp->number_of_iothreads, "Syncio0");
	status += xdd_init_barrier(&xgp->syncio_barrier[1], xgp->number_of_iothreads, "Syncio1");
	status +=  xdd_init_barrier(&xgp->results_pass_barrier[0],    xgp->number_of_iothreads+1, "ResultsPass0");
	status += xdd_init_barrier(&xgp->results_pass_barrier[1],    xgp->number_of_iothreads+1, "ResultsPass1");
	status +=  xdd_init_barrier(&xgp->results_display_barrier[0], xgp->number_of_iothreads+1, "ResultsDisplay0");
	status += xdd_init_barrier(&xgp->results_display_barrier[1], xgp->number_of_iothreads+1, "ResultsDisplay1");
	status +=  xdd_init_barrier(&xgp->results_run_barrier,    xgp->number_of_iothreads+1, "ResultsRun");
	status += xdd_init_barrier(&xgp->results_display_final_barrier, xgp->number_of_iothreads+1, "ResultsDisplayFinal");
	if (status < 0)  {
		fprintf(stderr,"%s: xdd_init_all_barriers: ERROR: Cannot init barrier for the results manager.\n",xgp->progname);
		return(status);
	}
	status = xdd_init_barrier(&xgp->serializer_barrier[0], 2, "Serializer0");
	status += xdd_init_barrier(&xgp->serializer_barrier[1], 2, "Serializer1");
	xgp->abort_io = 0;
	return(0);
} /* end of xdd_init_all_barriers() */

 
 
 
 
