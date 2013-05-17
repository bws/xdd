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
#include "xint.h"

//******************************************************************************
// Contains subroutines that implement the lockstep option
//******************************************************************************
/*----------------------------------------------------------------------------*/
// xdd_lockstep_init(ptds_t *p) - Initialize mutex and barriers for a lockstep
//    operation.
// This subroutine is called by target_init().
// The target being initialized can be either a MASTER or a SLAVE or both.
// If it is a MASTER then the 'master lockstep' structure is initialized.
// If it is a SLAVE then the 'slave lockstep' structure is initialized.
// Assuming nothing bad happens whilst initializing things, this subroutine
// will return XDD_RC_GOOD.
//

int32_t
xdd_lockstep_init(ptds_t *p) {
	int32_t		status;			// Status of a function call
	lockstep_t	*lsp;	// Pointer to the MASTER's Lock Step Struct
	char 		errmsg[256];


if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_init: ENTER - p=%p\n",p);
	if (p->lsp == 0) 
		return(XDD_RC_GOOD); // No Lockstep Operation Requested

	lsp = p->lsp;
	if (lsp) {
		// Init the task-counter mutex and the lockstep barrier 
		status = pthread_mutex_init(&lsp->ls_mutex, 0);
		if (status) {
			sprintf(errmsg,"%s: io_thread_init:Error initializing lock step target %d mutex",
				xgp->progname,p->my_target_number);
			perror(errmsg);
			fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
				xgp->progname,p->my_target_number);
			fflush(xgp->errout);
			xgp->abort = 1;
			return(XDD_RC_UGLY);
		}
		// Initialize barrier in this lockstep structure
		sprintf(lsp->Lock_Step_Barrier.name,"LockStep_M%d_S%d",p->my_target_number,lsp->ls_ms_target);
		xdd_init_barrier(p->my_planp, &lsp->Lock_Step_Barrier, 2, lsp->Lock_Step_Barrier.name);
	}
	return(XDD_RC_GOOD);
} // End of xdd_lockstep_init()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_pass() - This subroutine initializes the variables that
 * are used by the lockstep option.
 */
void
xdd_lockstep_before_pass(ptds_t *p) {
	lockstep_t *lsp;			// Pointer to the lock step struct


	if (p->lsp == 0) 
		return;

	lsp = p->lsp;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_before_pass: ENTER - p=%p, ls_ms_state=0x%x\n",p,lsp->ls_ms_state);
	if (lsp) {
		if (lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			lsp->ls_ms_state |= LS_STARTUP_WAIT;
		} else {
			lsp->ls_ms_state &= ~LS_STARTUP_WAIT;
		}
		lsp->ls_op_counter = 0;
		lsp->ls_byte_counter = 0;
		lsp->ls_ms_state &= ~LS_PASS_COMPLETE;
	}

} // End of xdd_lockstep_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_pass() - This subroutine will do all the stuff needed to 
 * be done for lockstep operations after an entire pass is complete.
 */
int32_t
xdd_lockstep_after_pass(ptds_t *p) {
	//lockstep_t *lsp;			// Pointer to the lock step struct

	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_pass: ENTER - p=%p\n",p);
	if (p->lsp == 0) 
		return(XDD_RC_GOOD);

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_pass: EXIT - p=%p\n",p);

	return(XDD_RC_GOOD);
	
} /* End of xdd_lockstep_after_pass() */

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_op(ptds_t *p) 
 */
int32_t
xdd_lockstep_before_io_op(ptds_t *p) {
	lockstep_t *lsp;			// Pointer to the lock step struct
	char	startup_wait;


	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
	//
	if (p->lsp == 0)
		return(XDD_RC_GOOD);

	// At this point we know that this target is part of a lockstep operation.
	// Therefore this process is either a MASTER or a SLAVE target or both.
	// If we are a SLAVE, then "p->lsp" should be a valid pointer to 
	// our lockstep structure. 
	//
	// In general, the MASTER does not have to do anything before an I/O op.
	//

	// The "lsp" variable always points to our lockstep structure.
	lsp = p->lsp;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_before_io_op: ENTER - p=%p, lsp=%p, ls_ms_state=0x%x\n",p,lsp,lsp->ls_ms_state);

	// As a slave, we need to check to see if we should stop running at this point. 
	// If not, keep on truckin'.
	// Look at the type of task that we need to perform and the quantity. 
	// If we have exhausted the quantity of operations for this interval then 
	// enter the lockstep barrier.
	startup_wait = FALSE;
	pthread_mutex_lock(&lsp->ls_mutex);
	if (lsp->ls_ms_state & LS_STARTUP_WAIT) {
		lsp->ls_ms_state &= ~LS_STARTUP_WAIT;
		startup_wait = TRUE;
	}
	pthread_mutex_unlock(&lsp->ls_mutex);

	if (startup_wait == TRUE) {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_before_io_op: ENTERING LS BARRIER - Waiting for something to do... p=%p, lsp=%p, ls_ms_state=0x%x\n",p,lsp,lsp->ls_ms_state);
		xdd_barrier(&lsp->Lock_Step_Barrier,&p->occupant,0);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_before_io_op: RETURNED FROM LS BARRIER - I have something to do, p=%p, lsp=%p, ls_ms_state=0x%x\n",p,lsp,lsp->ls_ms_state);
	}
	return(XDD_RC_GOOD);

} // End of xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_io_op(ptds_t *p) 
 *  It is important to note that this subroutine could process a SLAVE and
 *  a MASTER in the same call because a target can be a slave to one master
 *  *and* a master to a different slave. 
 *
 */
int32_t
xdd_lockstep_after_io_op(ptds_t *qp) {
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t *next_lsp;			// Pointer to the lock step struct
	int32_t		status; 		// Returned from the master/slave berfore iop routines
	ptds_t		*p;				// Pointer to the PTDS of the TARGET
	ptds_t		*next_ptdsp;				// Pointer to the PTDS of the next slave TARGET
	char		release_next_target;	// Indicates that we need to release the next target.


	p = qp->target_ptds;
	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op: ENTER - p=%p\n",p);
	//
	if (p->lsp == 0)
		return(XDD_RC_GOOD);

	lsp = p->lsp;
	next_ptdsp = lsp->ls_next_ptdsp;
	if (next_ptdsp) 
		next_lsp = next_ptdsp->lsp;
	else next_lsp = NULL;

	pthread_mutex_lock(&lsp->ls_mutex);

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op: ENTER - p=%p, lsp=%p, next_ptdsp=%p, next_lsp=%p\n",p,lsp,next_ptdsp,next_lsp);
	// Check to see if we have reached a point where we need to release the SLAVE
	status = xdd_lockstep_check_triggers(qp, lsp);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op: status=%d from check_triggers, bytes_remaining=%lld\n",status,(long long int)p->bytes_remaining);
	
	if (status == TRUE) {
		release_next_target = TRUE;
		lsp->ls_ms_state |= LS_STARTUP_WAIT;
	}

	if (p->bytes_remaining <= 0) {
		lsp->ls_ms_state |= LS_PASS_COMPLETE;
		release_next_target = TRUE;
		lsp->ls_ms_state |= LS_STARTUP_WAIT;
	}

	if (lsp->ls_ms_state & LS_PASS_COMPLETE) {
		if (next_lsp->ls_ms_state & LS_PASS_COMPLETE) {
			release_next_target = FALSE;
		} else {
			release_next_target = TRUE;
			next_lsp->ls_ms_state |= LS_PASS_COMPLETE;
			lsp->ls_ms_state |= LS_STARTUP_WAIT;
		}
	}

	pthread_mutex_unlock(&lsp->ls_mutex);

	if (release_next_target) { 
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op_master: RELEASING NEXT TARGET - p=%p, lsp=%p, next_lsp=%p, lsp->ls_ms_state=0x%x, next_lsp->ls_ms_state=0x%x\n",p,lsp,next_lsp,lsp->ls_ms_state, next_lsp->ls_ms_state);

		// Enter the next target's barrier which will release it
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op_master: ENTERING BARRIER TO WAKE UP NEXT TARGET - p=%p, lsp=%p, next_lsp=%p\n",p,lsp,next_lsp);
		xdd_barrier(&next_lsp->Lock_Step_Barrier,&p->occupant,0);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op_master: RETURNED FROM BARRIER AFTER WAKING UP NEXT TARGET - p=%p, lsp=%p, next_lsp=%p\n",p,lsp,next_lsp);
	}

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_after_io_op: EXIT - p=%p\n",p);
	return(XDD_RC_GOOD);

} // End of xdd_lockstep_after_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_check_triggers()
//  This subroutine is called by either the MASTER or SLAVE to check whether
//  or not it has reached a trigger point.
//  A return of FALSE means that no trigger has been met.
//  A return of TRUE means that one of the triggers has been reached.
//
//  It is assumed that the lock for the lockstep data structure pointed to
//  by lsp is held by the caller.
//
int32_t
xdd_lockstep_check_triggers(ptds_t *qp, lockstep_t *lsp) {
	int32_t		status;			// Status to return to caller
	nclk_t   	time_now;		// Used by the lock step functions 
	ptds_t		*p;				// Pointer to the PTDS of the TARGET


	p = qp->target_ptds;

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_check_triggers: ENTER - p=%p, lsp=%p ls_interval_type=0x%x, ls_interval_value=%lld \n",p,lsp,lsp->ls_interval_type,(long long int)lsp->ls_interval_value);
	status = FALSE;
	/* Check to see if it is time to ping the slave to do something */
	if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
		// If we are past the start time then signal the SLAVE to start.
		nclk_now(&time_now);
		if (time_now > lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_task_counter++;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_OP) {
		// If we are past the specified operation, then signal the SLAVE to start.
		lsp->ls_op_counter++;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_check_triggers: INTERVAL_OP - p=%p, ls_interval_value=%lld, ls_op_counter=%lld, p->tgtstp->my_current_op_number=%lld\n",p,(long long int)lsp->ls_interval_value, (long long int)lsp->ls_op_counter, (long long int)p->tgtstp->my_current_op_number);
		if (lsp->ls_op_counter >= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_op_counter = 0;
		} 
	}
	if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		// If we have completed percentage of operations then signal the SLAVE to start.
		lsp->ls_task_counter++;
		if (p->tgtstp->my_current_op_number >= lsp->ls_task_counter) {
			status=TRUE;
			lsp->ls_task_counter = 0;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		// If we have completed transferring the specified number of bytes, then signal the SLAVE to start.
		lsp->ls_byte_counter += qp->tgtstp->my_current_io_status;
		if (lsp->ls_byte_counter >= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_byte_counter = 0;
		}
	}
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"lockstep_check_triggers: returning status %d\n", status);
	return(status);

} // End of xdd_lockstep_check_triggers() 
