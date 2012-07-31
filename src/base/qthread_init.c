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
 *       Brad Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
#include "xdd.h"
#include <sched.h>
/*----------------------------------------------------------------------------*/
/* xdd_qthread_init() - Initialization routine for a QThread
 * This routine is passed a pointer to the PTDS for this QThread.
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_qthread_init(ptds_t *qp) {
    int32_t  	status;
    ptds_t		*p;			// Pointer to this qthread's target PTDS
    char		tmpname[XDD_BARRIER_NAME_LENGTH];	// Used to create unique names for the barriers

// Print the cpuset
int i;
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
printf("Thread %d running on", pthread_self());
for (i = 0; i< 48; i++)
  if (CPU_ISSET(i, &cpuset))
    printf(" %d", i);
printf("\n");

    // Get the target Thread PTDS address as well
    p = qp->target_ptds;

#if (AIX)
	qp->my_thread_id = thread_self();
#elif (LINUX)
	qp->my_thread_id = syscall(SYS_gettid);
#else
	qp->my_thread_id = qp->my_pid;
#endif

	// The "my_current_state_mutex" is used by the QThreads when checking or updating the state info
	status = pthread_mutex_init(&qp->my_current_state_mutex, 0);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Cannot init my_current_state_mutex \n",
			xgp->progname, 
			qp->my_target_number,
			qp->my_qthread_number);
		fflush(xgp->errout);
		return(-1);
	}
	// The "mutex_this_qthread_is_available" is used by the QThreads and the get_next_available_qthread() subroutines
	status = pthread_mutex_init(&qp->qthread_target_sync_mutex, 0);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Cannot init qthread_target_sync_mutex\n",
			xgp->progname, 
			qp->my_target_number,
			qp->my_qthread_number);
		fflush(xgp->errout);
		return(-1);
	}
	qp->qthread_target_sync = 0;

#if (LINUX || AIX)
        // Copy the file descriptor from the target thread (requires pread/pwrite support)
        status = xdd_target_shallow_open(qp);
#else
	// Open the target device/file
	status = xdd_target_open(qp);
#endif
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Failed to open Target named '%s'\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname);
		return(-1);
	}
        
	// Get a RW buffer
	qp->rwbuf = xdd_init_io_buffers(qp);
	if (qp->rwbuf == 0) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Failed to allocate I/O buffer.\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number);
		return(-1);
	}

	// Set proper data pattern in RW buffer
	xdd_datapattern_buffer_init(qp);

	// Init the QThread-TargetPass WAIT Barrier for this QThread
	sprintf(tmpname,"T%04d:Q%04d>qthread_targetpass_wait_barrier",qp->my_target_number,qp->my_qthread_number);
	status = xdd_init_barrier(&qp->qthread_targetpass_wait_for_task_barrier, 2, tmpname);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Cannot initialize QThread TargetPass WAIT barrier.\n",
			xgp->progname, 
			qp->my_target_number,
			qp->my_qthread_number);
		fflush(xgp->errout);
		return(-1);
	}

	// Initialize the qthread ordering semaphores - these are "non-shared" semaphores 

	// Init the semaphore used by targetpass and qthread 
	//status = sem_init(&qp->this_qthread_is_available_sem, 0, 0);
	status = pthread_cond_init(&qp->this_qthread_is_available_condition, 0);
	//if (status) {
	//	fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: ERROR: Cannot initialize this_qthread_is_available semaphore.\n",
	//		xgp->progname, 
	//		qp->my_target_number,
	//		qp->my_qthread_number);
	//	fflush(xgp->errout);
	//	return(-1);
	//}

	// Indicate to the Target Thread that this QThread is available
	pthread_mutex_lock(&p->any_qthread_available_mutex);
	p->any_qthread_available++;
	status = pthread_cond_broadcast(&p->any_qthread_available_condition);
	pthread_mutex_unlock(&p->any_qthread_available_mutex);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: WARNING: Bad status from sem_post on any_qthread_available semaphore: status=%d, errno=%d\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			status,
			errno);
	}

	// Set up for an End-to-End operation (if requested)
	if (qp->target_options & TO_ENDTOEND) {
		if (qp->target_options & (TO_E2E_DESTINATION|TO_E2E_SOURCE)) {
			status = xdd_e2e_qthread_init(qp);
		} else { // Not sure which side of the E2E this target is supposed to be....
			fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: Cannot determine which side of the E2E operation this target is supposed to be.\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number);
			fprintf(xgp->errout,"%s: xdd_qthread_init: Check to be sure that the '-e2e issource' or '-e2e isdestination' was specified for this target.\n",
				xgp->progname);
				fflush(xgp->errout);
			return(-1);
		}

		if (status == -1) {
			fprintf(xgp->errout,"%s: xdd_qthread_init: Target %d QThread %d: E2E %s initialization failed.\n",
				xgp->progname,
				qp->my_target_number,
				qp->my_qthread_number,
				(qp->target_options & TO_E2E_DESTINATION) ? "DESTINATION":"SOURCE");
		return(-1);
		}
	} // End of end-to-end setup

	// All went well...
	return(0);

} // End of xdd_qthread_init()
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
