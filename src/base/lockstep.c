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
	int32_t		i;				// loop counter
	int32_t		status;			// Status of a function call
	lockstep_t	*master_lsp;	// Pointer to the MASTER's Lock Step Struct
	lockstep_t 	*slave_lsp;		// Pointer to the SLAVE's Lock Step Struct
	char 		errmsg[256];



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
			// Initialize only the SLAVE barriers in this lockstep structure
			for (i=0; i<=1; i++) {
				sprintf(master_lsp->Lock_Step_Barrier[i].name,"LockStep_MASTER_M%d_S%d",p->my_target_number,master_lsp->ls_ms_target);
				xdd_init_barrier(&master_lsp->Lock_Step_Barrier[i], 2, master_lsp->Lock_Step_Barrier[i].name);
			}
			master_lsp->Lock_Step_Barrier_Index = 0;
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
			// Initialize only the SLAVE barriers in this lockstep structure
			for (i=0; i<=1; i++) {
				sprintf(slave_lsp->Lock_Step_Barrier[i].name,"LockStep_SLAVE_M%d_S%d",p->my_target_number,slave_lsp->ls_ms_target);
				xdd_init_barrier(&slave_lsp->Lock_Step_Barrier[i], 2, slave_lsp->Lock_Step_Barrier[i].name);
			}
			slave_lsp->Lock_Step_Barrier_Index = 0;
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


	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return;

	master_lsp = p->master_lsp;
	if (master_lsp) {
		if (master_lsp->ls_ms_state & LS_I_AM_A_MASTER) {
			master_lsp->ls_slave_loop_counter = 0;
			master_lsp->ls_interval_base_value = 0;
			if (master_lsp->ls_interval_type & LS_INTERVAL_TIME) {
				master_lsp->ls_interval_base_value = p->my_pass_start_time;
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
			slave_lsp->ls_slave_loop_counter = 0;
			slave_lsp->ls_task_base_value = 0;
			slave_lsp->ls_ms_state |= LS_SLAVE_STARTUP_WAIT;
			if (slave_lsp->ls_task_type & LS_TASK_TIME) {
				slave_lsp->ls_task_base_value = p->my_pass_start_time;
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
	if ((p->master_lsp == 0) && (p->slave_lsp == 0)) 
		return(XDD_RC_GOOD);

	slave_lsp = p->slave_lsp;
	if (slave_lsp) {
		if (slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 	* master does not inadvertently wait for the slave to complete.
		 	*/
			pthread_mutex_lock(&slave_lsp->ls_mutex);
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
			slave_lsp->ls_ms_state |= LS_SLAVE_FINISHED;
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
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return(XDD_RC_GOOD);

	status = XDD_RC_GOOD; // Default status
	// At this point we know that this target is part of a lockstep operation.
	// Therefore this process is either a MASTER or a SLAVE target or both and
	// "lsp" points to our lockstep structure. 
	// If we are a MASTER, then "ls_slave" will be the target number of our SLAVE.
	// Likewise, if we are a SLAVE, then "ls_master" will be the target number 
	// of our MASTER. 
	// It is possible that we are a MASTER to one target an a SLAVE to another 
	// target. We cannot be a MASTER or SLAVE to ourself.
	//

	if (p->slave_lsp) {
		if (p->slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE)
			status = xdd_lockstep_slave_before_io_op(p);
	}

	return(status);

} // xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_slave_before_io_op(ptds_t *p)
//  This subroutine is called by xdd_lockstep_before_io_op() and is responsible
//  for processing the SLAVE side of a lockstep operation. 
//
// 
int32_t
xdd_lockstep_slave_before_io_op(ptds_t *p) {
	lockstep_t 	*master_lsp;		// Pointer to the MASTER's lock step struct
	lockstep_t 	*slave_lsp;			// Pointer to the SLAVE's lock step struct


	// The "slave_lsp" variable always points to our lockstep structure.
	slave_lsp = p->slave_lsp;
	// My lockstep structure has a pointer to my MASTER's lockstep structure.
	master_lsp = slave_lsp->ls_master_lsp;

	// As a slave, we need to check to see if we should stop running at this point. If not, keep on truckin'.
	// Look at the type of task that we need to perform and the quantity. If we have exhausted
	// the quantity of operations for this interval then enter the lockstep barrier.
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	if (slave_lsp->ls_ms_state & LS_SLAVE_STARTUP_WAIT) {
		slave_lsp->ls_ms_state &= ~LS_SLAVE_STARTUP_WAIT;
		// At this point it looks like the SLAVE needs to wait for the MASTER to tell us to do something 
		pthread_mutex_unlock(&slave_lsp->ls_mutex);
		xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Index],&p->occupant,0);
		// At this point the MASTER has released us from this barrier to either do some work or stop immediately.
		if ((slave_lsp->ls_ms_state & LS_MASTER_PASS_COMPLETED) && (slave_lsp->ls_ms_state & LS_SLAVE_COMPLETION_STOP)) {
			/* if we need to simply finish all this, just release the lock and move on */
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
			slave_lsp->ls_slave_loop_counter = 0;
		}
	pthread_mutex_unlock(&slave_lsp->ls_mutex);
	} 
	return(XDD_RC_GOOD);

} // xdd_lockstep_slave_before_io_op()

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
	//
	if ((p->master_lsp == 0) && (p->slave_lsp == 0))
		return(XDD_RC_GOOD);

	//////// MASTER Processing /////////
	status = XDD_RC_GOOD;
	if (p->master_lsp) {
		if (p->master_lsp->ls_ms_state & LS_I_AM_A_MASTER) {
			status = xdd_lockstep_master_after_io_op(p);
		}
	}
	//////// SLAVE Processing /////////
	if (p->slave_lsp) {
		if (p->slave_lsp->ls_ms_state & LS_I_AM_A_SLAVE) {
			status = xdd_lockstep_slave_after_io_op(p);
		} 
	}

	return(status);
} // xdd_lockstep_after_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_slave_after_io_op(ptds_t *p)
int32_t
xdd_lockstep_slave_after_io_op(ptds_t *p) {
	int			wakeup_master;
	nclk_t   	time_now;			// Used by the lock step functions 
	lockstep_t 	*master_lsp;		// Pointer to the MASTER's lock step struct
	lockstep_t 	*slave_lsp;			// Pointer to the SLAVE's lock step struct

	// Ahhhh... we must be doing something... 
	slave_lsp = p->slave_lsp;
	// My lockstep structure has a pointer to my MASTER's lockstep structure.
	master_lsp = slave_lsp->ls_master_lsp;
	wakeup_master = FALSE;
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	if (slave_lsp->ls_task_type & LS_TASK_TIME) {
		// If we are past the start time then signal the specified target to start.
		nclk_now(&time_now);
		if (time_now > (slave_lsp->ls_task_value + slave_lsp->ls_task_base_value)) {
			wakeup_master = TRUE;
			slave_lsp->ls_task_base_value = time_now;
			slave_lsp->ls_task_counter--;
		}
	}
	if (slave_lsp->ls_task_type & LS_TASK_OP) {
		// If we are past the specified operation, then indicate slave to wait.
		if (p->my_current_op_number >= (slave_lsp->ls_task_value + slave_lsp->ls_task_base_value)) {
			wakeup_master = TRUE;
			slave_lsp->ls_task_base_value = p->my_current_op_number;
			slave_lsp->ls_task_counter--;
		}
	}
	if (slave_lsp->ls_task_type & LS_TASK_PERCENT) {
		// If we have completed percentage of operations then indicate slave to wait.
		if (p->my_current_op_number >= ((slave_lsp->ls_task_value * slave_lsp->ls_task_base_value) * p->qthread_ops)) {
			wakeup_master = TRUE;
			slave_lsp->ls_task_base_value++;
			slave_lsp->ls_task_counter--;
		}
	}
	if (slave_lsp->ls_task_type & LS_TASK_BYTES) {
		// If we have completed transferring the specified number of bytes, then indicate slave to wait.
		if (p->my_current_bytes_xfered >= (slave_lsp->ls_task_value + (int64_t)(slave_lsp->ls_task_base_value))) {
			wakeup_master = TRUE;
			slave_lsp->ls_task_base_value = p->my_current_bytes_xfered;
			slave_lsp->ls_task_counter--;
		}
	}
	pthread_mutex_unlock(&slave_lsp->ls_mutex);
	if (wakeup_master == TRUE) { // Enter the MASTER's barrier to release it
		xdd_barrier(&master_lsp->Lock_Step_Barrier[master_lsp->Lock_Step_Barrier_Index],&p->occupant,0);
	}
	// At this point the MASTER has released us from this barrier to either do some work or stop immediately.
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	if (slave_lsp->ls_ms_state & LS_MASTER_PASS_COMPLETED) {
		if ((slave_lsp->ls_ms_state & LS_SLAVE_COMPLETION_STOP)) {
			/* if we need to simply finish all this, just release the lock and move on */
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
			slave_lsp->ls_slave_loop_counter = 0;
			slave_lsp->ls_ms_state &= LS_SLAVE_FINISHED; // Slave is no done 
		} 
		pthread_mutex_unlock(&slave_lsp->ls_mutex);
	} else { // At this point it looks like the SLAVE needs to wait for the MASTER to tell us to do something 
		pthread_mutex_unlock(&slave_lsp->ls_mutex);
		xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Index],&p->occupant,0);
	}
	return(XDD_RC_GOOD);
} // xdd_lockstep_slave_after_io_op()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_master_after_io_op(ptds_t *p)
//  This subroutine is called by xdd_lockstep_after_io_op() and is responsible
//  for processing the MASTER side of a lockstep operation after each I/O Operation.
//  After every I/O operation the MASTER needs to check to see if it has reached
//  a point where it needs to release the SLAVE. 
//  When the MASTER has reached a point where it needs to release the SLAVE
//  then it will check first to see if it has completed a PASS and set a flag
//  in the SLAVE's ls_ms_state that indicates PASS COMPLETED.
//  To release the SLAVE the MASTER will enter the SLAVE's 'slave' barrier. 
//  If the slave is waiting at that barrier, then it will be released to 
//  perform its next task. Otherwise, the MASTER will wait inside that barrier
//  until the SLAVE enters it. 
//  After the MASTER has released the SLAVE and returns from the SLAVE's
//  slave barrier it will enter the SALVE's 'master' barrier and wait for the
//  SLAVE to complete its task. 
//  Once the SLAVE has completed its current task, it will enter the SLAVE's
//  'master' barrier thereby releasing the MASTER to continue with its tasks. 
//  The SLAVE then enters its 'slave' barrier to wait for the MASTER to come
//  back to this routine.
//
int32_t
xdd_lockstep_master_after_io_op(ptds_t *p) {
	int32_t		release_slave;		// Indicates whether or not to release the SLAVE 
	nclk_t   	time_now;			// Used by the lock step functions 
	lockstep_t 	*slave_lsp;			// Pointer to the SLAVE's lock step struct
	lockstep_t 	*master_lsp;		// Pointer to the MASTER's lock step struct


	master_lsp = p->master_lsp;
	slave_lsp = master_lsp->ls_slave_lsp;

	release_slave = FALSE;

	/* Check to see if it is time to ping the slave to do something */
	if (master_lsp->ls_interval_type & LS_INTERVAL_TIME) {
		// If we are past the start time then signal the specified target to start.
		nclk_now(&time_now);
		if (time_now > (master_lsp->ls_interval_value + master_lsp->ls_interval_base_value)) {
			release_slave = TRUE;
			master_lsp->ls_interval_base_value = time_now; /* reset the new base time */
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_OP) {
		// If we are past the specified operation, then signal the specified target to start.
		if (p->my_current_op_number >= (master_lsp->ls_interval_value + master_lsp->ls_interval_base_value)) {
			release_slave = TRUE;
			master_lsp->ls_interval_base_value = p->my_current_op_number;
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
		// If we have completed percentage of operations then signal the specified target to start.
		if (p->my_current_op_number >= ((master_lsp->ls_interval_value*master_lsp->ls_interval_base_value) * p->qthread_ops)) {
			release_slave = TRUE;
			master_lsp->ls_interval_base_value++;
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_BYTES) {
		// If we have completed transferring the specified number of bytes, then signal the specified target to start.
		if (p->my_current_bytes_xfered >= (master_lsp->ls_interval_value + (int64_t)(master_lsp->ls_interval_base_value))) {
			release_slave = TRUE;
			master_lsp->ls_interval_base_value = p->my_current_bytes_xfered;
		}
	}
	// Note that the SLAVE owns the mutex and the counter used to tell it to do something 
	// Get the mutex lock for the ls_task_counter and increment the counter 
	// At this point the slave could be in one of three places. Thus this next section of code
	// must be carefully structured to prevent a race condition between the master and the slave.
	// If the slave is simply waiting for the master to release it, then the ls_task_counter lock
	// can be released and the master can enter the lock_step_barrier to get the slave going again.
	// If the slave is running, then it will have to wait for the ls_task_counter lock and
	// when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
	// decrement the counter by 1 and continue on executing a lock. 
	if (release_slave) { // Looks like it is time to release the SLAVE to do something 
		pthread_mutex_lock(&slave_lsp->ls_mutex);
		slave_lsp->ls_task_counter++;
		slave_lsp->ls_ms_state |= LS_MASTER_WAITING;
		if (p->bytes_remaining <= 0) 
			slave_lsp->ls_ms_state |= LS_MASTER_PASS_COMPLETED;
		pthread_mutex_unlock(&slave_lsp->ls_mutex);

		// Enter the SLAVE's barrier which will release the SLAVE
		xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Index],&p->occupant,0);
		// At this point the slave outght to be running. 
		// Now that the SLAVE is running, we (the MASTER) needs to wait for the SLAVE to finish
		// Enter the MASTER's barrier and wait for the SLAVE to release us
		xdd_barrier(&master_lsp->Lock_Step_Barrier[master_lsp->Lock_Step_Barrier_Index],&p->occupant,0);
		// At this point the SLAVE has released us and we are done at this point
	} // Done releasing the slave 
	return(XDD_RC_GOOD);

} // xdd_lockstep_master_after_io_op()

