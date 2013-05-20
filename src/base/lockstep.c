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
// Assuming nothing bad happens whilst initializing things, this subroutine
// will return XDD_RC_GOOD.
//

int32_t
xdd_lockstep_init(ptds_t *p) {
	int32_t		status;			// Status of a function call
	lockstep_t	*lsp;	// Pointer to the Lock Step Struct
	char 		errmsg[256];


	lsp = p->lsp;
	// If there is no lockstep pointer then this target is not involved in a lockstep
	if (lsp == 0) 
		return(XDD_RC_GOOD); 

	// Check to see if this target has already been initialized
	if (lsp->ls_state & LS_STATE_INITIALIZED)
		return(XDD_RC_GOOD); 

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_init:p:%p:lsp:%p:ENTER my taget number is %d, the next target number is %d\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number,lsp->ls_next_ptdsp->my_target_number);
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
	// The "MASTER" is this target and the "SLAVE" is the next target
	sprintf(lsp->Lock_Step_Barrier.name,"LockStep_M%d_S%d",p->my_target_number,lsp->ls_next_ptdsp->my_target_number);
	xdd_init_barrier(p->my_planp, &lsp->Lock_Step_Barrier, 2, lsp->Lock_Step_Barrier.name);

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_init:p:%p:lsp:%p:EXIT \n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp);
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
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_before_pass:p:%p:lsp:%p:ENTER - ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_state);
	if (lsp) {
		if ((lsp->ls_state & LS_STATE_I_AM_THE_FIRST) == 0) {
			lsp->ls_state |= LS_STATE_WAIT;
		} else {
			lsp->ls_state &= ~LS_STATE_WAIT;
		}
		lsp->ls_ops_scheduled= 0;
		lsp->ls_ops_completed= 0;
		lsp->ls_bytes_scheduled= 0;
		lsp->ls_bytes_completed= 0;
		lsp->ls_state &= ~LS_STATE_PASS_COMPLETE;
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
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_pass:p:%p:::ENTER \n",(long long int)pclk_now()-xgp->debug_base_time,p);
	if (p->lsp == 0) 
		return(XDD_RC_GOOD);

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_pass:p:%p:::EXIT \n",(long long int)pclk_now()-xgp->debug_base_time,p);

	return(XDD_RC_GOOD);
	
} /* End of xdd_lockstep_after_pass() */

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_op(ptds_t *p) 
 */
int32_t
xdd_lockstep_before_io_op(ptds_t *p) {
	lockstep_t *lsp;			// Pointer to the lock step struct


	lsp = p->lsp;
	if (lsp == 0)
		return(XDD_RC_GOOD);

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_before_io_op:p:%p:lsp:%p:ENTER - ls_state=0x%x, ops_scheduled=%lld, bytes_scheduled=%lld, interval_value=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_state,(long long int)lsp->ls_ops_scheduled,(long long int)lsp->ls_bytes_scheduled, (long long int)lsp->ls_interval_value);

	pthread_mutex_lock(&lsp->ls_mutex);
	if (lsp->ls_ops_scheduled >= lsp->ls_interval_value) {
		lsp->ls_state |= LS_STATE_WAIT;
	} else {
		lsp->ls_ops_scheduled++;
	}
	pthread_mutex_unlock(&lsp->ls_mutex);

	if (lsp->ls_state & LS_STATE_WAIT) {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_before_io_op:p:%p:lsp:%p:ENTERING_LS_BARRIER - Waiting for something to do... ls_state=0x%x, ops_scheduled=%lld, bytes_scheduled=%lld, interval_value=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_state,(long long int)lsp->ls_ops_scheduled,(long long int)lsp->ls_bytes_scheduled, (long long int)lsp->ls_interval_value);
		xdd_barrier(&lsp->Lock_Step_Barrier,&p->occupant,0);
		lsp->ls_state &= ~LS_STATE_WAIT;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_before_io_op:p:%p:lsp:%p:RETURNED_FROM_LS_BARRIER - I have something to do, ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_state);
	} else {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_before_io_op:p:%p:lsp:%p:DONT NEED TO WAIT for something to do... ls_state=0x%x, ops_scheduled=%lld, bytes_scheduled=%lld, interval_value=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_state,(long long int)lsp->ls_ops_scheduled,(long long int)lsp->ls_bytes_scheduled, (long long int)lsp->ls_interval_value);
	}
	return(XDD_RC_GOOD);

} // End of xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_io_op(ptds_t *qp) 
 * This subroutine runs in the context of a QThread (not a Target Thread)
 *
 */
int32_t
xdd_lockstep_after_io_op(ptds_t *qp) {
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t *next_lsp;			// Pointer to the lock step struct
	int32_t		status; 		// Returned from the master/slave berfore iop routines
	ptds_t		*p;				// Pointer to the PTDS of the TARGET
	ptds_t		*next_ptdsp;				// Pointer to the PTDS of the next slave TARGET


	p = qp->target_ptds;
	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
	//
	if (p->lsp == 0)
		return(XDD_RC_GOOD);

	lsp = p->lsp;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:ENTER\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp);
	next_ptdsp = lsp->ls_next_ptdsp;
	if (next_ptdsp) 
		next_lsp = next_ptdsp->lsp;
	else next_lsp = NULL;

	pthread_mutex_lock(&lsp->ls_mutex);
	lsp->ls_state &= ~LS_STATE_RELEASE_TARGET;

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:CALLING UPDATE_TRIGGERS - next_ptdsp=%p, next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,next_ptdsp,next_lsp);
	// Check to see if we have reached a point where we need to release the SLAVE
	xdd_lockstep_update_triggers(qp, lsp);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:CALLING CHECK_TRIGGERS - next_ptdsp=%p, next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,next_ptdsp,next_lsp);
	status = xdd_lockstep_check_triggers(qp, lsp);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:REUTNRED FROM CHECK_TRIGGERS - status=%d from check_triggers, bytes_completed=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,status,(long long int)p->bytes_completed);
	
	if (status == TRUE) {
		lsp->ls_state |= LS_STATE_WAIT; // This tells the target thread to WAIT in lockstep_before_io_op()
		// Check to see if we have transferred all the bytes for this pass.
		// If so, then this was the last I/O op for this pass and this pass is complete
		if (p->bytes_completed >= p->target_bytes_to_xfer_per_pass) {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:TARGET %d END OF PASS - lsp->ls_state=0x%x \n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, lsp->ls_state);
			lsp->ls_state |= LS_STATE_PASS_COMPLETE;
		}

		next_lsp = xdd_lockstep_get_next_target_to_release(p, lsp);
		if (next_lsp != lsp) {
			lsp->ls_state |= LS_STATE_RELEASE_TARGET;
		} else { // The "next_lsp" is actually this same target 
			if (p->queue_depth > 1) 
				lsp->ls_state |= LS_STATE_RELEASE_TARGET;
			else lsp->ls_state &= ~LS_STATE_WAIT; // This tells the target thread to NOT to WAIT in lockstep_before_io_op()
		}
	}

	pthread_mutex_unlock(&lsp->ls_mutex);

	if (lsp->ls_state & LS_STATE_RELEASE_TARGET) {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:TARGET %d is RELEASING NEXT TARGET %d - next_lsp=%p, lsp->ls_state=0x%x, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, lsp->ls_next_ptdsp->my_target_number,next_lsp,lsp->ls_state, next_lsp->ls_state);

		// Enter the next target's barrier which will release it
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:TARGET %d ENTERING BARRIER TO WAKE UP NEXT TARGET %d - next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, lsp->ls_next_ptdsp->my_target_number,next_lsp);
		xdd_barrier(&next_lsp->Lock_Step_Barrier,&p->occupant,0);
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:TARGET %d RETURNED FROM BARRIER AFTER WAKING UP NEXT TARGET %d - next_lsp=%p\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, lsp->ls_next_ptdsp->my_target_number,next_lsp);
	}

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_after_io_op:p:%p:lsp:%p:EXIT - next_lsp=%p, lsp->ls_state=0x%x, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,next_lsp,lsp->ls_state, next_lsp->ls_state);
	return(XDD_RC_GOOD);

} // End of xdd_lockstep_after_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_get_next_target_to_release()
//
lockstep_t * 
xdd_lockstep_get_next_target_to_release(ptds_t *p, lockstep_t *lsp) {
	lockstep_t	*next_lsp;


	next_lsp = lsp->ls_next_ptdsp->lsp;
	while (next_lsp->ls_state & LS_STATE_PASS_COMPLETE) {
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_get_next_target_to_release:p:%p:lsp:%p:TARGET %d NEXT_LSP IS AT END_OF_PASS - next_lsp=%p, lsp->ls_state=0x%x, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, next_lsp,lsp->ls_state, next_lsp->ls_state);
		next_lsp = next_lsp->ls_next_ptdsp->lsp;
		if (next_lsp == lsp)
			break;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_get_next_target_to_release:p:%p:lsp:%p:TARGET %d WHAT IS NEXT NEXT LSP - next_lsp=%p, lsp->ls_state=0x%x, next_lsp->ls_state=0x%x\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,p->my_target_number, next_lsp,lsp->ls_state, next_lsp->ls_state);
		// At this point next_lsp points to:
		//    The next target that needs to be released
		//    or
		//    The lsp of this target (no targets get released)
	}
	return(next_lsp);
} // End of xdd_lockstep_get_next_target_to_release()


/*----------------------------------------------------------------------------*/
// xdd_lockstep_update_triggers()
//  It is assumed that the lock for the lockstep data structure pointed to
//  by lsp is held by the caller.
//
void 
xdd_lockstep_update_triggers(ptds_t *qp, lockstep_t *lsp) {
	ptds_t		*p;				// Pointer to the PTDS of the TARGET


	p = qp->target_ptds;

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_update_triggers:p:%p:lsp:%p:ENTER - ls_interval_type=0x%x, ls_interval_value=%lld \n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_interval_type,(long long int)lsp->ls_interval_value);
	/* Check to see if it is time to ping the slave to do something */
	if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
		// If we are past the start time then signal the SLAVE to start.
			lsp->ls_task_counter++;
	}
	if (lsp->ls_interval_type & LS_INTERVAL_OP) {
		// If we are past the specified operation, then signal the SLAVE to start.
		lsp->ls_ops_completed++;
		lsp->ls_ops_scheduled--;
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_update_triggers:p:%p:lsp:%p:INTERVAL_OP - ls_interval_value=%lld, ls_ops_completed=%lld, p->tgtstp->my_current_op_number=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,(long long int)lsp->ls_interval_value, (long long int)lsp->ls_ops_completed, (long long int)p->tgtstp->my_current_op_number);
	}
	if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		// If we have completed percentage of operations then signal the SLAVE to start.
		lsp->ls_task_counter++;
	}
	if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		// If we have completed transferring the specified number of bytes, then signal the SLAVE to start.
		lsp->ls_bytes_completed += qp->tgtstp->my_current_io_status;
		lsp->ls_bytes_scheduled -= qp->tgtstp->my_current_io_status;
	}
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_update_triggers:p:%p:lsp:%p:returning \n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp);
	return;

} // End of xdd_lockstep_update_triggers() 
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

if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_check_triggers:p:%p:lsp:%p:ENTER - ls_interval_type=0x%x, ls_interval_value=%lld \n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,lsp->ls_interval_type,(long long int)lsp->ls_interval_value);
	status = FALSE;
	/* Check to see if it is time to ping the slave to do something */
	if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
		// If we are past the start time then signal the SLAVE to start.
		nclk_now(&time_now);
		if (time_now > lsp->ls_interval_value) {
			status=TRUE;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_OP) {
		// If we are past the specified operation, then signal the SLAVE to start.
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_check_triggers:p:%p:lsp:%p:INTERVAL_OP - ls_interval_value=%lld, ls_ops_completed=%lld, p->tgtstp->my_current_op_number=%lld\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,(long long int)lsp->ls_interval_value, (long long int)lsp->ls_ops_completed, (long long int)p->tgtstp->my_current_op_number);
		if (lsp->ls_ops_completed >= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_ops_completed= 0;
		} 
	}
	if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		// If we have completed percentage of operations then signal the SLAVE to start.
		if (p->tgtstp->my_current_op_number >= lsp->ls_task_counter) {
			status=TRUE;
			lsp->ls_task_counter = 0;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		// If we have completed transferring the specified number of bytes, then signal the SLAVE to start.
		if (lsp->ls_bytes_completed>= lsp->ls_interval_value) {
			status=TRUE;
			lsp->ls_bytes_completed= 0;
		}
	}
if (xgp->global_options & GO_DEBUG) fprintf(stderr,"%lld:lockstep_check_triggers:p:%p:lsp:%p:returning status %d\n",(long long int)pclk_now()-xgp->debug_base_time,p,lsp,status);
	return(status);

} // End of xdd_lockstep_check_triggers() 
