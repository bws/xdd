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
/* xdd_lockstep_init(ptds_t *p) {
 */
int32_t
xdd_lockstep_init(ptds_t *p) {
	int32_t			i;		// loop counter
	int32_t			status;	// Status of a function call
	lockstep_t		*lsp;	// Pointer to Lock Step Struct
	char 			errmsg[256];


	if (p->lockstepp == 0)
		return(0); // No Lockstep Operation Requested

	lsp = p->lockstepp;
	/* This section will initialize the slave side of the lock step mutex and barriers */
	if (lsp->ls_master >= 0) { /* This means that this target has a master and therefore must be a slave */
		/* init the task counter mutex and the lock step barrier */
		status = pthread_mutex_init(&lsp->ls_mutex, 0);
		if (status) {
			sprintf(errmsg,"%s: io_thread_init:Error initializing lock step slave target %d task counter mutex",
				xgp->progname,p->my_target_number);
			perror(errmsg);
			fprintf(xgp->errout,"%s: io_thread_init: Aborting I/O for target %d due to lockstep mutex allocation failure\n",
				xgp->progname,p->my_target_number);
			fflush(xgp->errout);
			xgp->abort = 1;
			return(-1);
		}
		for (i=0; i<=1; i++) {
				sprintf(lsp->Lock_Step_Barrier[i].name,"LockStep_T%d_%d",lsp->ls_master,p->my_target_number);
				xdd_init_barrier(&lsp->Lock_Step_Barrier[i], 2, lsp->Lock_Step_Barrier[i].name);
		}
		lsp->Lock_Step_Barrier_Slave_Index = 0;
		lsp->Lock_Step_Barrier_Master_Index = 0;
	} else { /* Make sure these are uninitialized */
		for (i=0; i<=1; i++) {
			lsp->Lock_Step_Barrier[i].flags &= ~XDD_BARRIER_FLAG_INITIALIZED;
		}
		lsp->Lock_Step_Barrier_Slave_Index = 0;
		lsp->Lock_Step_Barrier_Master_Index = 0;
	}
	return(0);
} // End of xdd_lockstep_init()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_op(ptds_t *p) {
 */
int32_t
xdd_lockstep_before_io_op(ptds_t *p) {
	int32_t	ping_slave;			// indicates whether or not to ping the slave 
	int32_t	slave_wait;			// indicates the slave should wait before continuing on 
	pclk_t   time_now;			// used by the lock step functions 
	ptds_t	*p2;				// Secondary PTDS pointer
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t *lsp2;			// Pointer to the lock step struct

	/* Check to see if we are in lockstep with another target.
	 * If so, then if we are a master, then do the master thing.
	 * Then check to see if we are also a slave and do the slave thing.
	 */
	if (p->lockstepp == 0)
		return(0);
	lsp = p->lockstepp;
	if (lsp->ls_slave >= 0) { /* Looks like we are the MASTER in lockstep with a slave target */
		p2 = xgp->ptdsp[lsp->ls_slave]; /* This is a pointer to the slave ptds */
		lsp2 = p2->lockstepp;
		ping_slave = FALSE;
		/* Check to see if it is time to ping the slave to do something */
		if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
			/* If we are past the start time then signal the specified target to start.
			 */
			pclk_now(&time_now);
			if (time_now > (lsp->ls_interval_value + lsp->ls_interval_base_value)) {
				ping_slave = TRUE;
				lsp->ls_interval_base_value = time_now; /* reset the new base time */
			}
		}
		if (lsp->ls_interval_type & LS_INTERVAL_OP) {
			/* If we are past the specified operation, then signal the specified target to start.
			 */
			if (p->my_current_op_number >= (lsp->ls_interval_value + lsp->ls_interval_base_value)) {
				ping_slave = TRUE;
				lsp->ls_interval_base_value = p->my_current_op_number;
			}
		}
		if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
			/* If we have completed percentage of operations then signal the specified target to start.
			 */
			if (p->my_current_op_number >= ((lsp->ls_interval_value*lsp->ls_interval_base_value) * p->qthread_ops)) {
				ping_slave = TRUE;
				lsp->ls_interval_base_value++;
			}
		}
		if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
			/* If we have completed transferring the specified number of bytes, then signal the 
			 * specified target to start.
			 */
			if (p->my_current_bytes_xfered >= (lsp->ls_interval_value + (int64_t)(lsp->ls_interval_base_value))) {
				ping_slave = TRUE;
				lsp->ls_interval_base_value = p->my_current_bytes_xfered;
			}
		}
		if (ping_slave) { /* Looks like it is time to ping the slave to do something */
			/* note that the SLAVE owns the mutex and the counter used to tell it to do something */
			/* Get the mutex lock for the ls_task_counter and increment the counter */
			pthread_mutex_lock(&lsp2->ls_mutex);
			/* At this point the slave could be in one of three places. Thus this next section of code
			 * must be carefully structured to prevent a race condition between the master and the slave.
			 * If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			 * can be released and the master can enter the lock_step_barrier to get the slave going again.
			 * If the slave is running, then it will have to wait for the ls_task_counter lock and
			 * when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			 * decrement the counter by 1 and continue on executing a lock. 
			 */
			lsp2->ls_task_counter++;
			/* Now that we have updated the slave's task counter, we need to check to see if the slave is 
			 * waiting for us to give it a kick. If so, enter the barrier which will release the slave and
			 * us as well. 
			 * ****************Currently this is set up simply to do overlapped lockstep.*******************
			 */
			if (lsp2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
				lsp2->ls_ms_state &= ~LS_SLAVE_WAITING;
				pthread_mutex_unlock(&lsp2->ls_mutex);
				xdd_barrier(&lsp2->Lock_Step_Barrier[lsp2->Lock_Step_Barrier_Master_Index],&p->occupant,0);
				lsp2->Lock_Step_Barrier_Master_Index ^= 1;
				/* At this point the slave outght to be running. */
			} else {
				pthread_mutex_unlock(&lsp2->ls_mutex);
			}
		} /* Done pinging the slave */
	} /* end of SLAVE processing as a MASTER */
	if (lsp->ls_master >= 0) { /* Looks like we are a slave to some other MASTER target */
		/* As a slave, we need to check to see if we should stop running at this point. If not, keep on truckin'.
		 */   
		/* Look at the type of task that we need to perform and the quantity. If we have exhausted
		 * the quantity of operations for this interval then enter the lockstep barrier.
		 */
		pthread_mutex_lock(&lsp->ls_mutex);
		if (lsp->ls_task_counter > 0) {
			/* Ahhhh... we must be doing something... */
			slave_wait = FALSE;
			if (lsp->ls_task_type & LS_TASK_TIME) {
				/* If we are past the start time then signal the specified target to start.
				*/
				pclk_now(&time_now);
				if (time_now > (lsp->ls_task_value + lsp->ls_task_base_value)) {
					slave_wait = TRUE;
					lsp->ls_task_base_value = time_now;
					lsp->ls_task_counter--;
				}
			}
			if (lsp->ls_task_type & LS_TASK_OP) {
				/* If we are past the specified operation, then indicate slave to wait.
				*/
				if (p->my_current_op_number >= (lsp->ls_task_value + lsp->ls_task_base_value)) {
					slave_wait = TRUE;
					lsp->ls_task_base_value = p->my_current_op_number;
					lsp->ls_task_counter--;
				}
			}
			if (lsp->ls_task_type & LS_TASK_PERCENT) {
				/* If we have completed percentage of operations then indicate slave to wait.
				*/
				if (p->my_current_op_number >= ((lsp->ls_task_value * lsp->ls_task_base_value) * p->qthread_ops)) {
					slave_wait = TRUE;
					lsp->ls_task_base_value++;
					lsp->ls_task_counter--;
				}
			}
			if (lsp->ls_task_type & LS_TASK_BYTES) {
				/* If we have completed transferring the specified number of bytes, then indicate slave to wait.
				*/
				if (p->my_current_bytes_xfered >= (lsp->ls_task_value + (int64_t)(lsp->ls_task_base_value))) {
					slave_wait = TRUE;
					lsp->ls_task_base_value = p->my_current_bytes_xfered;
					lsp->ls_task_counter--;
				}
			}
		} else {
			slave_wait = TRUE;
		}
		if (slave_wait) { /* At this point it looks like the slave needs to wait for the master to tell us to do something */
			/* Get the lock on the task counter and check to see if we need to actually wait or just slide on
			 * through to the next task.
			 */
			/* slave_wait will be set for one of two reasons: either the task counter is zero or the slave
			 * has finished the last task and needs to wait for the next ping from the master.
			 */
			/* If the master has finished then don't bother waiting for it to enter this barrier because it never will */
			if ((lsp->ls_ms_state & LS_MASTER_FINISHED) && (lsp->ls_ms_state & LS_SLAVE_COMPLETE)) {
				/* if we need to simply finish all this, just release the lock and move on */
				lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (lsp->ls_ms_state & LS_MASTER_WAITING) {
					lsp->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&lsp->ls_mutex);
					xdd_barrier(&lsp->Lock_Step_Barrier[lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
					lsp->Lock_Step_Barrier_Slave_Index ^= 1;
					lsp->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&lsp->ls_mutex);
				}
			} else if ((lsp->ls_ms_state & LS_MASTER_FINISHED) && (lsp->ls_ms_state & LS_SLAVE_STOP)) {
				/* This is the case where the master is finished and we need to stop NOW.
				 * Therefore, release the lock and break this loop.
				 */
				lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (lsp->ls_ms_state & LS_MASTER_WAITING) {
					lsp->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&lsp->ls_mutex);
					xdd_barrier(&lsp->Lock_Step_Barrier[lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
					lsp->Lock_Step_Barrier_Slave_Index ^= 1;
					lsp->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&lsp->ls_mutex);
				}
				return(1);
			} else { /* At this point the master is not finished and we need to wait for something to do */
				if (lsp->ls_ms_state & LS_MASTER_WAITING)
					lsp->ls_ms_state &= ~LS_MASTER_WAITING;
				lsp->ls_ms_state |= LS_SLAVE_WAITING;
				pthread_mutex_unlock(&lsp->ls_mutex);
				xdd_barrier(&lsp->Lock_Step_Barrier[lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
				lsp->Lock_Step_Barrier_Slave_Index ^= 1;
				lsp->ls_slave_loop_counter++;
			}
		} else {
			pthread_mutex_unlock(&lsp->ls_mutex);
		}
		/* At this point the slave does not have to wait for anything so keep on truckin... */

	} // End of being the master in a lockstep op
	return(0);

} // xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_pass() - This subroutine initializes the variables that
 * are used by the end-to-end option
 */
void
xdd_lockstep_before_pass(ptds_t *p) {
	lockstep_t *lsp;			// Pointer to the lock step struct


	if (p->lockstepp == 0)
		return;
	lsp = p->lockstepp;
	lsp->ls_slave_loop_counter = 0;
	if (lsp->ls_slave >= 0) { /* I am a master */
		lsp->ls_interval_base_value = 0;
		if (lsp->ls_interval_type & LS_INTERVAL_TIME) {
			lsp->ls_interval_base_value = p->my_pass_start_time;
		}
		if (lsp->ls_interval_type & LS_INTERVAL_OP) {
			lsp->ls_interval_base_value = 0;
		}
		if (lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
			lsp->ls_interval_base_value = 1; 
		}
		if (lsp->ls_interval_type & LS_INTERVAL_BYTES) {
			lsp->ls_interval_base_value = 0;
		}
	} else { /* I am a slave */
		lsp->ls_task_base_value = 0;
		if (lsp->ls_task_type & LS_TASK_TIME) {
			lsp->ls_task_base_value = p->my_pass_start_time;
		}
		if (lsp->ls_task_type & LS_TASK_OP) {
			lsp->ls_task_base_value = 0;
		}
		if (lsp->ls_task_type & LS_TASK_PERCENT) {
			lsp->ls_task_base_value = 1; 
		}
		if (lsp->ls_task_type & LS_TASK_BYTES) {
			lsp->ls_task_base_value = 0;
		}
	}

} // xdd_lockstep_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_pass() - This subroutine will do all the stuff needed to 
 * be done for lockstep operations after an entire pass is complete.
 */
int32_t
xdd_lockstep_after_pass(ptds_t *p) {
	ptds_t	*p2;		// Secondary PTDS pointer
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t *lsp2;			// Pointer to the lock step struct

	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
	if (p->lockstepp == 0)
		return(0);
	lsp = p->lockstepp;
	if (lsp->ls_master >= 0) {
		/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 * master does not inadvertently wait for the slave to complete.
		 */
		pthread_mutex_lock(&lsp->ls_mutex);
		lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
		lsp->ls_ms_state |= LS_SLAVE_FINISHED;
		pthread_mutex_unlock(&lsp->ls_mutex);
	}
	if (lsp->ls_slave >= 0) {  
		/* Get the mutex lock for the ls_task_counter and increment the counter */
		p2 = xgp->ptdsp[lsp->ls_slave];
		lsp2 = p2->lockstepp;
		pthread_mutex_lock(&lsp2->ls_mutex);
		lsp2->ls_ms_state |= LS_MASTER_FINISHED;
		/* At this point the slave could be in one of three places. Thus this next section of code
			* must be carefully structured to prevent a race condition between the master and the slave.
			* If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			* can be released and the master can enter the lock_step_barrier to get the slave going again.
			* If the slave is running, then it will have to wait for the ls_task_counter lock and
			* when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			* decrement the counter by 1 and continue on executing a lock. 
			*/
		lsp2->ls_task_counter++;
		if (lsp2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
			lsp2->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&lsp2->ls_mutex);
			xdd_barrier(&lsp2->Lock_Step_Barrier[lsp2->Lock_Step_Barrier_Master_Index],&p->occupant,0);
			lsp2->Lock_Step_Barrier_Master_Index ^= 1;
		} else {
			pthread_mutex_unlock(&lsp2->ls_mutex);
		}
	}

	return(0);

} /* end of xdd_lockstep_after_pass() */
