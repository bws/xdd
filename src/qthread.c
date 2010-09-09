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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_qthread() - This is the QThread routine that actually performs I/O
 * on behalf of a Target thread.
 * This thread, of which there can be  many, is started by the xdd_target_thread_init()
 * routine. 
 * The QThread will initialize itself and make itself Available
 * and wait for something to do.
 */
void *
xdd_qthread(void *pin) {
	int32_t  	status;
	ptds_t		*qp;	// Pointer to this QThread's PTDS
	ptds_t		*p;		// Pointer to this QThread's Target PTDS


	qp = (ptds_t *)pin; 
	p = qp->target_ptds;	// This is the pointer to this QThread's Target PTDS

	status = xdd_qthread_init(qp);
	if (status != 0) {
		fprintf(xgp->errout,"%s: xdd_qthread: Aborting target due to previous initialization failure for Target number %d name '%s' QThread %d.\n",
			xgp->progname,
			qp->my_target_number,
			qp->target_full_pathname,
			qp->my_qthread_number);
		fflush(xgp->errout);
		xgp->abort = 1; // This will prevent all other threads from being created...
	}

	// Enter the QThread_Init barrier so that the next QThread can start 
	xdd_barrier(&p->target_qthread_init_barrier,&qp->occupant,0);

	if ( xgp->abort == 1) // Something went wrong during thread initialization so let's just leave
		return(0);

	// If this is a dry run then just exit at this point
	if (xgp->global_options & GO_DRYRUN)
		return(0);

	//////////////////////////////////// Begin the WHILE loop waiting for tasks to perform /////////////////////////////////
	// The subroutine that is called for any particular task will set the "xgp->canceled" flag to
	// indicate that there was a condition that warrants canceling the entire run
	while (1) {
		// Enter the QThread_TargetPass_Wait barrier until we are assigned something to do byte targetpass()
		xdd_barrier(&qp->qthread_targetpass_wait_barrier,&qp->occupant,1);

		// Look at Task request 
		switch (qp->task_request) {
			case TASK_REQ_IO:
				// Perform the requested I/O operation
				xdd_qthread_io(qp);
//TMR fprintf(stderr,"qthread: target %d qthread %d, finished io %lld\n",qp->my_target_number, qp->my_qthread_number,(long long int)qp->target_op_number);
				break;
			case TASK_REQ_REOPEN:
				// Reopen the target as requested
				xdd_target_reopen(qp);
				break;
			case TASK_REQ_STOP:
				// This indicates that we should clean up and exit this subroutine
				xdd_qthread_cleanup(qp);
				return(0);
			case TASK_REQ_EOF:
				// Upon completion of xdd_e2e_eof_source_side(), pass_complete will be set for this QThread
				xdd_e2e_eof_source_side(qp);
				qp->pass_complete = 1;
				break;
			case TASK_REQ_END_OF_PASS:
				// Used by targetpass_loop() to make sure all QThreads have finished and enter the targetpass_qthread_pass_complete barrier
				qp->pass_complete = 1;
//TMR fprintf(stderr,"qthread: qthread %d, task req end of pass\n", qp->my_qthread_number);
				break;
			default:
				// Technically, we should never see this....
				fprintf(xgp->errout,"%s: xdd_qthread: WARNING: Target number %d name '%s' QThread %d - unknown work request: 0x%x.\n",
					xgp->progname,
					qp->my_target_number,
					qp->target_full_pathname,
					qp->my_qthread_number,
					qp->task_request);
				break;
		} // End of SWITCH stmnt that determines the TASK

//TMR if (qp->pass_complete) fprintf(stderr,"qthread: qthread %d, locking this_thread_is_available\n",qp->my_qthread_number);
		pthread_mutex_lock(&qp->mutex_this_qthread_is_available);
//TMR if (qp->pass_complete) fprintf(stderr,"qthread: qthread %d, this_thread_is_available locked\n",qp->my_qthread_number);
		// Mark this QThread Available
		qp->this_qthread_is_available = 1;
		if ((p->target_options & TO_STRICT_ORDERING) || 
			(p->target_options & TO_LOOSE_ORDERING)) { // Release the QThread Locator which might be waiting if we are using Strict or Loose Ordering 
			status = sem_post(&qp->sem_this_qthread_is_available);
			if (status) {
				fprintf(xgp->errout,"%s: xdd_qthread: Target %d QThread %d: WARNING: Bad status from sem_post on this_qthread_is_available semaphore: status=%d, errno=%d\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					status,
					errno);
			}
		} else { // No ordering
			// Release the QThread Locator which might be waiting for any avaiable QThread
			status = sem_post(&p->sem_any_qthread_available);
			if (status) {
				fprintf(xgp->errout,"%s: xdd_qthread: Target %d QThread %d: WARNING: Bad status from sem_post on sem_any_qthread_available semaphore: status=%d, errno=%d\n",
					xgp->progname,
					qp->my_target_number,
					qp->my_qthread_number,
					status,
					errno);
			}
		}
//TMR if (qp->pass_complete) fprintf(stderr,"qthread: qthread %d, unlocking this_thread_is_available\n",qp->my_qthread_number);
		pthread_mutex_unlock(&qp->mutex_this_qthread_is_available);

		// For an E2E operation pass_complete is set by xdd_qthread_io() on the destination side after reading an EOF packet
		// or on the source side pass_complete will be set after returning from xdd_e2e_eof_source_side().      
		if (qp->pass_complete) { // If this QThread is done for this pass then enter the targetpass_qthread_passcomplete barrier
//TMR fprintf(stderr,"qthread: target %d <%p> qthread %d <%p>, entering pass complete barrier\n",qp->my_target_number, p, qp->my_qthread_number,qp);
			xdd_barrier(&p->targetpass_qthread_passcomplete_barrier,&qp->occupant,0);
//TMR fprintf(stderr,"qthread: target %d qthread %d, left pass complete barrier\n",qp->my_target_number, qp->my_qthread_number);
		}

	} // end of WHILE loop 

} /* end of xdd_qthread() */
