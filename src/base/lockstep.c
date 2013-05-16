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
	lockstep_t	*master_lsp;	// Pointer to the MASTER's Lock Step Struct
	lockstep_t 	*slave_lsp;		// Pointer to the SLAVE's Lock Step Struct
	char 		errmsg[256];


fprintf(stderr,"lockstep_init: ENTER - p=%p\n",p);
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return(XDD_RC_GOOD); // No Lockstep Operation Requested

	master_lsp = p->master_lsp;
	if (master_lsp) {
		if (master_lsp->ls_ms_state & LS_I_AM_A_MASTER) { 
			// Init the task-counter mutex and the lockstep barrier 
			status = pthread_mutex_init(&master_lsp->ls_mutex, 0);
			if (status) {
				sprintf(errmsg,"%s: io_thread_init:Error initializing lock step slave target %d task counter mutex",
					xgp->progname,p->my_target_number);
				perror(errmsg);
				fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
					xgp->progname,p->my_target_number);
				fflush(xgp->errout);
				xgp->abort = 1;
				return(XDD_RC_UGLY);
			}
			// Initialize barrier in this lockstep structure
			sprintf(master_lsp->Lock_Step_Barrier.name,"LockStep_MASTER_M%d_S%d",p->my_target_number,master_lsp->ls_ms_target);
			xdd_init_barrier(p->my_planp, &master_lsp->Lock_Step_Barrier, 2, master_lsp->Lock_Step_Barrier.name);
		} 
	}
	slave_lsp = p->slave_lsp;
	if (slave_lsp) {
		if (slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			// Init the task-counter mutex and the lockstep barrier 
			status = pthread_mutex_init(&slave_lsp->ls_mutex, 0);
			if (status) {
				sprintf(errmsg,"%s: io_thread_init:Error initializing lock step slave target %d task counter mutex",
					xgp->progname,p->my_target_number);
				perror(errmsg);
				fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
					xgp->progname,p->my_target_number);
				fflush(xgp->errout);
				xgp->abort = 1;
				return(XDD_RC_UGLY);
			}
			// Initialize barrier in this lockstep structure
			sprintf(slave_lsp->Lock_Step_Barrier.name,"LockStep_SLAVE_S%d_M%d",p->my_target_number,slave_lsp->ls_ms_target);
			xdd_init_barrier(p->my_planp, &slave_lsp->Lock_Step_Barrier, 2, slave_lsp->Lock_Step_Barrier.name);
		} 
	}
	return(XDD_RC_GOOD);
} // End of xdd_lockstep_init()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_pass() - This subroutine initializes the variables that
 * are used by the lockstep option.
 */
void
xdd_lockstep_before_pass(ptds_t *p) {
	lockstep_t *master_lsp;			// Pointer to the lock step struct
	lockstep_t *slave_lsp;			// Pointer to the lock step struct


fprintf(stderr,"lockstep_before_pass: ENTER - p=%p\n",p);
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return;

	master_lsp = p->master_lsp;
	if (master_lsp) {
		if (master_lsp->ls_ms_state & LS_I_AM_A_MASTER) {
			master_lsp->ls_interval_base_value = 0;
			if (master_lsp->ls_interval_type & LS_INTERVAL_TIME) {
				master_lsp->ls_interval_base_value = p->tgtstp->my_pass_start_time;
			}
			if (master_lsp->ls_interval_type & LS_INTERVAL_OP) {
				master_lsp->ls_interval_base_value = 0;
			}
			if (master_lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
				master_lsp->ls_interval_base_value = 1; 
			}
			if (master_lsp->ls_interval_type & LS_INTERVAL_BYTES) {
				master_lsp->ls_interval_base_value = 0;
			}
		} 
	}
	slave_lsp = p->slave_lsp;
	if (slave_lsp) {
		if (slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			slave_lsp->ls_task_base_value = 0;
			slave_lsp->ls_ms_state |= LS_SLAVE_STARTUP_WAIT;
			if (slave_lsp->ls_task_type & LS_TASK_TIME) {
				slave_lsp->ls_task_base_value = p->tgtstp->my_pass_start_time;
			}
			if (slave_lsp->ls_task_type & LS_TASK_OP) {
				slave_lsp->ls_task_base_value = 0;
			}
			if (slave_lsp->ls_task_type & LS_TASK_PERCENT) {
				slave_lsp->ls_task_base_value = 1; 
			}
			if (slave_lsp->ls_task_type & LS_TASK_BYTES) {
				slave_lsp->ls_task_base_value = 0;
			}
		}
	}

} // xdd_lockstep_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_pass() - This subroutine will do all the stuff needed to 
 * be done for lockstep operations after an entire pass is complete.
 */
int32_t
xdd_lockstep_after_pass(ptds_t *p) {
	lockstep_t *master_lsp;			// Pointer to the lock step struct
	lockstep_t *slave_lsp;			// Pointer to the lock step struct

	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
fprintf(stderr,"lockstep_after_pass: ENTER - p=%p\n",p);
	if ((p->master_lsp == 0) && (p->slave_lsp == 0)) 
		return(XDD_RC_GOOD);

	slave_lsp = p->slave_lsp;
	if (slave_lsp) {
		if (slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 	* master does not inadvertently wait for the slave to complete.
		 	*/
			pthread_mutex_lock(&slave_lsp->ls_mutex);
			slave_lsp->ls_ms_state |= LS_SLAVE_STARTUP_WAIT;
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
			slave_lsp->ls_ms_state |= LS_SLAVE_FINISHED;
			slave_lsp->ls_ms_state &= ~LS_MASTER_WAITING;
			slave_lsp->ls_ms_state &= ~LS_MASTER_PASS_COMPLETED;
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
		}
	}
	master_lsp = p->master_lsp;
	if (master_lsp) {
	}

	return(XDD_RC_GOOD);
	
} /* end of xdd_lockstep_after_pass() */

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_op(ptds_t *p) 
 */
int32_t
xdd_lockstep_before_io_op(ptds_t *p) {
	int32_t		status; // Returned from the master/slave berfore iop routines


	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
	//
fprintf(stderr,"lockstep_before_io_op: ENTER - p=%p\n",p);
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return(XDD_RC_GOOD);

	status = XDD_RC_GOOD; // Default status
	// At this point we know that this target is part of a lockstep operation.
	// Therefore this process is either a MASTER or a SLAVE target or both.
	// If we are a SLAVE, then "p->slave_lsp" should be a valid pointer to 
	// our lockstep structure. 
	//
	// In general, the MASTER does not have to do anything before an I/O op.
	//

	if (p->slave_lsp) {
		if (p->slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE)
			status = xdd_lockstep_before_io_op_slave(p);
	}

	return(status);

} // xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_before_io_op_slave(ptds_t *p)
//  This subroutine is called by xdd_lockstep_before_io_op() and is responsible
//  for processing the SLAVE side of a lockstep operation. 
//
// 
int32_t
xdd_lockstep_before_io_op_slave(ptds_t *p) {
	lockstep_t 	*slave_lsp;			// Pointer to the SLAVE's lock step struct


fprintf(stderr,"lockstep_before_io_op_slave: ENTER - p=%p\n",p);
	// The "slave_lsp" variable always points to our lockstep structure.
	slave_lsp = p->slave_lsp;

	// As a slave, we need to check to see if we should stop running at this point. 
	// If not, keep on truckin'.
	// Look at the type of task that we need to perform and the quantity. 
	// If we have exhausted the quantity of operations for this interval then 
	// enter the lockstep barrier.
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	if (slave_lsp->ls_ms_state & LS_SLAVE_STARTUP_WAIT) {
		slave_lsp->ls_ms_state &= ~LS_SLAVE_STARTUP_WAIT;
	}
	pthread_mutex_unlock(&slave_lsp->ls_mutex);

	xdd_barrier(&slave_lsp->Lock_Step_Barrier,&p->occupant,0);
	return(XDD_RC_GOOD);

} // xdd_lockstep_before_io_op_slave()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_io_op(ptds_t *p) 
 *  It is important to note that this subroutine could process a SLAVE and
 *  a MASTER in the same call because a target can be a slave to one master
 *  *and* a master to a different slave. 
 */
int32_t
xdd_lockstep_after_io_op(ptds_t *p) {
	int32_t		status; 		// Returned from the master/slave berfore iop routines


	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
fprintf(stderr,"lockstep_after_io_op: ENTER - p=%p\n",p);
	//
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return(XDD_RC_GOOD);

	//////// MASTER Processing /////////
	status = XDD_RC_GOOD;
	if (p->master_lsp) {
		if (p->master_lsp->ls_ms_state & LS_I_AM_A_MASTER) {
			status = xdd_lockstep_after_io_op_master(p);
		}
	}
	//////// SLAVE Processing /////////
	if (p->slave_lsp) {
		if (p->slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			status = xdd_lockstep_after_io_op_slave(p);
		} 
	}

	return(status);
} // xdd_lockstep_after_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_after_io_op_slave(ptds_t *p)
int32_t
xdd_lockstep_after_io_op_slave(ptds_t *p) {
	lockstep_t 	*master_lsp;		// Pointer to the MASTER's lock step struct
	lockstep_t 	*slave_lsp;			// Pointer to the SLAVE's lock step struct
	char		wakeup_master;


	slave_lsp = p->slave_lsp;
	// My lockstep structure has a pointer to my MASTER's lockstep structure.
	master_lsp = slave_lsp->ls_master_ptdsp->master_lsp;
fprintf(stderr,"lockstep_after_io_op_slave: ENTER - p=%p, slave_lsp=%p, master_lsp=%p\n",p,slave_lsp,master_lsp);
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	// Check to see if we have reached a point where we need to release the SLAVE
	wakeup_master = xdd_lockstep_check_triggers(p, slave_lsp);
	if (slave_lsp->ls_ms_state & LS_MASTER_WAITING) {
		wakeup_master = TRUE;
	} 

	pthread_mutex_unlock(&slave_lsp->ls_mutex);

	if (wakeup_master == TRUE) { // Enter the MASTER's barrier to release it
		xdd_barrier(&master_lsp->Lock_Step_Barrier,&p->occupant,0);
	}
	return(XDD_RC_GOOD);
} // xdd_lockstep_after_io_op_slave()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_after_io_op_master(ptds_t *p)
//
int32_t
xdd_lockstep_after_io_op_master(ptds_t *p) {
	lockstep_t 	*slave_lsp;		// Pointer to the SLAVE's lock step struct
	lockstep_t 	*master_lsp;	// Pointer to the MASTER's lock step struct
	char		release_slave;	// Return status from check_triggers()
	char		master_pass_complete;	// Return status from check_triggers()


	master_lsp = p->master_lsp;
	slave_lsp = master_lsp->ls_slave_ptdsp->slave_lsp;

fprintf(stderr,"lockstep_after_io_op_master: ENTER - p=%p, slave_lsp=%p, master_lsp=%p\n",p,slave_lsp,master_lsp);
	// Check to see if we have reached a point where we need to release the SLAVE
	release_slave = xdd_lockstep_check_triggers(p, master_lsp);
	
	// Check to see if this is was the last I/O operation for the MASTER. 
	// If so, then a flag will get set in the SLAVE's lockstep structure so that
	// it knows that the MASTER has finished this pass.
	if (p->bytes_remaining <= 0) {
		master_pass_complete = TRUE;
		release_slave = TRUE;
	}

	// Note that the SLAVE owns the mutex and the counter used to tell it to do something 
	if (release_slave) { // Looks like it is time to release the SLAVE to do something 
		// Check to see if we (the MASTER) is done with a pass
		pthread_mutex_lock(&slave_lsp->ls_mutex);
		if (master_pass_complete == TRUE) 
			slave_lsp->ls_ms_state |= LS_MASTER_PASS_COMPLETED;
		else slave_lsp->ls_ms_state |= LS_MASTER_WAITING;
		pthread_mutex_unlock(&slave_lsp->ls_mutex);

		// Enter the SLAVE's barrier which will release the SLAVE
		xdd_barrier(&slave_lsp->Lock_Step_Barrier,&p->occupant,0);
		// At this point the slave outght to be running. 
		// Now that the SLAVE is running, we (the MASTER) may need to wait for the SLAVE to finish
		// If the MASTER has not yet finished its pass, then enter the MASTER's barrier 
		// and wait for the SLAVE to release us
		if ((slave_lsp->ls_ms_state |= LS_MASTER_WAITING))
			xdd_barrier(&master_lsp->Lock_Step_Barrier,&p->occupant,0);
	} // Done releasing the slave 
	return(XDD_RC_GOOD);

} // xdd_lockstep_after_io_op_master()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_check_triggers()
//  This subroutine is called by either the MASTER or SLAVE to check whether
//  or not it has reached a trigger point.
//  A return of FALSE means that no trigger has been met.
//  A return of TRUE means that one of the triggers has been reached.
//
int32_t
xdd_lockstep_check_triggers(ptds_t *p, lockstep_t *lsp) {
	int32_t		status;			// Status to return to caller
	nclk_t   	time_now;		// Used by the lock step functions 


fprintf(stderr,"lockstep_check_triggers: ENTER - p=%p, lsp=%p ls_interval_type=0x%x, ls_interval_value=%lld, ls_interval_base_value=%lld\n",p,lsp,lsp->ls_interval_type,(long long int)lsp->ls_interval_value, (long long int)lsp->ls_interval_base_value);
	status = FALSE;
	/* Check to see if it is time to ping the slave to do something */
	if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
		// If we are past the start time then signal the SLAVE to start.
		nclk_now(&time_now);
		if (time_now > (lsp->ls_interval_value + lsp->ls_interval_base_value)) {
			status=TRUE;
			lsp->ls_interval_base_value = time_now; /* reset the new base time */
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_OP) {
		// If we are past the specified operation, then signal the SLAVE to start.
		if (p->tgtstp->my_current_op_number >= (lsp->ls_interval_value + lsp->ls_interval_base_value)) {
			status=TRUE;
			lsp->ls_interval_base_value = p->tgtstp->my_current_op_number;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		// If we have completed percentage of operations then signal the SLAVE to start.
		if (p->tgtstp->my_current_op_number >= ((lsp->ls_interval_value*lsp->ls_interval_base_value) * p->qthread_ops)) {
			status=TRUE;
			lsp->ls_interval_base_value++;
		}
	}
	if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		// If we have completed transferring the specified number of bytes, then signal the SLAVE to start.
		if (p->tgtstp->my_current_bytes_xfered >= (lsp->ls_interval_value + (int64_t)(lsp->ls_interval_base_value))) {
			status=TRUE;
			lsp->ls_interval_base_value = p->tgtstp->my_current_bytes_xfered;
		}
	}
fprintf(stderr,"lockstep_check_triggers: returning status %d\n", status);
	return(status);

} // xdd_lockstep_check_triggers() 
