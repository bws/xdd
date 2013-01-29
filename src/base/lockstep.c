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
// Lockstep operations use the barriers in the SLAVE's lockstep struct. 
// The barriers in the MASTER lockstep struct are "uninitialized" just to make
// sure nothing strange happens. 
// Assuming nothing bad happens whilst initializing things, this subroutine
// will return XDD_RC_GOOD.
//

int32_t
xdd_lockstep_init(ptds_t *p) {
	int32_t			i;		// loop counter
	int32_t			status;	// Status of a function call
	lockstep_t		*lsp;	// Pointer to Lock Step Struct
	char 			errmsg[256];


	if (p->lockstepp == 0)
		return(XDD_RC_GOOD); // No Lockstep Operation Requested

	lsp = p->lockstepp;
	// This section will initialize the SLAVE side of the lockstep mutex and barriers 
	if (lsp->ls_master >= 0) { // This means that this target has a master and therefore THIS TARGET must be a slave 
		// Init the task-counter mutex and the lockstep barrier 
fprintf(stderr,"xdd_lockstep_init: ENTER, I am a slave p=%p, lsp=%p, the master is target %d\n",p,lsp,lsp->ls_master);
		status = pthread_mutex_init(&lsp->ls_mutex, 0);
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
		// There are two lockstep barriers. Don't ask.
		// The Lock_Step_Barrier_Slave_Index determines which of the two get used at any given time.
		for (i=0; i<=1; i++) {
				sprintf(lsp->Lock_Step_Barrier[i].name,"LockStep_T%d_%d",lsp->ls_master,p->my_target_number);
				xdd_init_barrier(&lsp->Lock_Step_Barrier[i], 2, lsp->Lock_Step_Barrier[i].name);
		}
		lsp->Lock_Step_Barrier_Slave_Index = 0;
	} else { // Make sure the MASTER lockstep barriers are uninitialized 
		for (i=0; i<=1; i++) {
			lsp->Lock_Step_Barrier[i].flags &= ~XDD_BARRIER_FLAG_INITIALIZED;
		}
	}
	return(XDD_RC_GOOD);
} // End of xdd_lockstep_init()

/*----------------------------------------------------------------------------*/
// xdd_lockstep_master_before_io_op(ptds_t *p)
//  This subroutine is called by xdd_lockstep_before_io_op() and is responsible
//  for processing the MASTER side of a lockstep operation. 
// It is important to note that the barrier used to coordinate with the SLAVE
//  is the SLAVE's barrier. The MASTER's barrier is never used. 
//
int32_t
xdd_lockstep_master_before_io_op(ptds_t *p) {
	int32_t		ping_slave;			// indicates whether or not to ping the slave 
	nclk_t   	time_now;			// used by the lock step functions 
	ptds_t		*slave_p;			// Slave PTDS pointer
	lockstep_t 	*master_lsp;		// Pointer to the master lock step struct
	lockstep_t 	*slave_lsp;			// Pointer to the slave lock step struct


	master_lsp = p->lockstepp;
	slave_p = xgp->ptdsp[master_lsp->ls_slave]; 	
	slave_lsp = slave_p->lockstepp;
	ping_slave = FALSE;

fprintf(stderr,"xdd_lockstep_master_before_io_op: ENTER master_p=%p, master_lsp=%p, the SLAVE target is %d, slave_p=%p, slave_lsp=%p\n",p,master_lsp,master_lsp->ls_slave,slave_p,slave_lsp);
	/* Check to see if it is time to ping the slave to do something */
	if (master_lsp->ls_interval_type & LS_INTERVAL_TIME) {
fprintf(stderr,"xdd_lockstep_master_before_io_op: Time Interval\n");
		// If we are past the start time then signal the specified target to start.
		nclk_now(&time_now);
		if (time_now > (master_lsp->ls_interval_value + master_lsp->ls_interval_base_value)) {
			ping_slave = TRUE;
			master_lsp->ls_interval_base_value = time_now; /* reset the new base time */
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_OP) {
fprintf(stderr,"xdd_lockstep_master_before_io_op: OP Interval\n");
		// If we are past the specified operation, then signal the specified target to start.
fprintf(stderr,"xdd_lockstep_master_before_io_op: MASTER OP Interval: my_current_op_number=%lld, ls_interval_value=%lld, ls_interval_base_value=%lld\n",(long long int)p->my_current_op_number,(long long int)master_lsp->ls_interval_value,(long long int)master_lsp->ls_interval_base_value);
		if (p->my_current_op_number >= (master_lsp->ls_interval_value + master_lsp->ls_interval_base_value)) {
			ping_slave = TRUE;
			master_lsp->ls_interval_base_value = p->my_current_op_number;
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_PERCENT) {
fprintf(stderr,"xdd_lockstep_master_before_io_op: Percent Interval\n");
		// If we have completed percentage of operations then signal the specified target to start.
		if (p->my_current_op_number >= ((master_lsp->ls_interval_value*master_lsp->ls_interval_base_value) * p->qthread_ops)) {
			ping_slave = TRUE;
			master_lsp->ls_interval_base_value++;
		}
	}
	if (master_lsp->ls_interval_type & LS_INTERVAL_BYTES) {
fprintf(stderr,"xdd_lockstep_master_before_io_op: Bytes Interval\n");
		// If we have completed transferring the specified number of bytes, then signal the specified target to start.
		if (p->my_current_bytes_xfered >= (master_lsp->ls_interval_value + (int64_t)(master_lsp->ls_interval_base_value))) {
			ping_slave = TRUE;
			master_lsp->ls_interval_base_value = p->my_current_bytes_xfered;
		}
	}
	if (ping_slave) { // Looks like it is time to ping the slave to do something 
fprintf(stderr,"xdd_lockstep_master_before_io_op: pinging slave\n");
		// Note that the SLAVE owns the mutex and the counter used to tell it to do something 
		// Get the mutex lock for the ls_task_counter and increment the counter 
		pthread_mutex_lock(&slave_lsp->ls_mutex);
		// At this point the slave could be in one of three places. Thus this next section of code
		// must be carefully structured to prevent a race condition between the master and the slave.
		// If the slave is simply waiting for the master to release it, then the ls_task_counter lock
		// can be released and the master can enter the lock_step_barrier to get the slave going again.
		// If the slave is running, then it will have to wait for the ls_task_counter lock and
		// when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
		// decrement the counter by 1 and continue on executing a lock. 
		slave_lsp->ls_task_counter++;
		// Now that we have updated the slave's task counter, we need to check to see if the slave is 
	 	// waiting for us to give it a kick. If so, enter the barrier which will release the slave and
	 	// us as well. 
	 	// ****************Currently this is set up simply to do overlapped lockstep.*******************
		if (slave_lsp->ls_ms_state & LS_SLAVE_WAITING) { // looks like the slave is waiting for something to do 
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_master_before_io_op: pinging slave... entering the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
			xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
fprintf(stderr,"xdd_lockstep_master_before_io_op: pinging slave... return from the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
			slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
			// At this point the slave outght to be running. 
		} else {
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
		}
fprintf(stderr,"xdd_lockstep_master_before_io_op: done pinging slave\n");
	} // Done pinging the slave 
fprintf(stderr,"xdd_lockstep_master_before_io_op: RETURN XDD_RC_GOOD\n");
	return(XDD_RC_GOOD);

} // xdd_lockstep_master_before_io_op()
/*----------------------------------------------------------------------------*/
// xdd_lockstep_slave_before_io_op(ptds_t *p)
//  This subroutine is called by xdd_lockstep_before_io_op() and is responsible
//  for processing the SLAVE side of a lockstep operation. 
// It is important to note that the barrier used to coordinate with the MASTER
//  is the SLAVE's barrier. The MASTER's barrier is never used. 
// 
int32_t
xdd_lockstep_slave_before_io_op(ptds_t *p) {
	int32_t		slave_wait;			// Indicates that we (the slave)  should wait before continuing on 
	nclk_t   	time_now;			// Used by the lock step functions 
	lockstep_t 	*slave_lsp;			// Pointer to slave lock step struct


	// The "slave_lsp" variable always points to our lockstep structure.
	slave_lsp = p->lockstepp;
	// As a slave, we need to check to see if we should stop running at this point. If not, keep on truckin'.
	// Look at the type of task that we need to perform and the quantity. If we have exhausted
	// the quantity of operations for this interval then enter the lockstep barrier.
fprintf(stderr,"xdd_lockstep_slave_before_io_op: ENTER I am a SLAVE, p=%p, slave_lsp=%p, my master is target %d, slave_lsp->ls_task_counter=%d\n",p,slave_lsp,slave_lsp->ls_master,slave_lsp->ls_task_counter);
	pthread_mutex_lock(&slave_lsp->ls_mutex);
	if (slave_lsp->ls_task_counter > 0) {
		// Ahhhh... we must be doing something... 
		slave_wait = FALSE;
		if (slave_lsp->ls_task_type & LS_TASK_TIME) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: SLAVE processing TIME task\n");
			// If we are past the start time then signal the specified target to start.
			nclk_now(&time_now);
			if (time_now > (slave_lsp->ls_task_value + slave_lsp->ls_task_base_value)) {
				slave_wait = TRUE;
				slave_lsp->ls_task_base_value = time_now;
				slave_lsp->ls_task_counter--;
			}
		}
		if (slave_lsp->ls_task_type & LS_TASK_OP) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: SLAVE  processing OP task\n");
				// If we are past the specified operation, then indicate slave to wait.
fprintf(stderr,"xdd_lockstep_slave_before_io_op: SLAVE OP Interval: my_current_op_number=%lld, ls_task_value=%lld, ls_task_base_value=%lld\n",(long long int)p->my_current_op_number,(long long int)slave_lsp->ls_task_value,(long long int)slave_lsp->ls_task_base_value);
				if (p->my_current_op_number >= (slave_lsp->ls_task_value + slave_lsp->ls_task_base_value)) {
					slave_wait = TRUE;
					slave_lsp->ls_task_base_value = p->my_current_op_number;
					slave_lsp->ls_task_counter--;
				}
			}
			if (slave_lsp->ls_task_type & LS_TASK_PERCENT) {
				// If we have completed percentage of operations then indicate slave to wait.
fprintf(stderr,"xdd_lockstep_slave_before_io_op: ENTER I am a SLAVE, p=%p, processing PERCENT task\n",p);
				if (p->my_current_op_number >= ((slave_lsp->ls_task_value * slave_lsp->ls_task_base_value) * p->qthread_ops)) {
					slave_wait = TRUE;
					slave_lsp->ls_task_base_value++;
					slave_lsp->ls_task_counter--;
				}
			}
			if (slave_lsp->ls_task_type & LS_TASK_BYTES) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: ENTER I am a SLAVE, p=%p, processing BYTES task\n",p);
				// If we have completed transferring the specified number of bytes, then indicate slave to wait.
				if (p->my_current_bytes_xfered >= (slave_lsp->ls_task_value + (int64_t)(slave_lsp->ls_task_base_value))) {
					slave_wait = TRUE;
					slave_lsp->ls_task_base_value = p->my_current_bytes_xfered;
					slave_lsp->ls_task_counter--;
				}
			}
	} else {
		slave_wait = TRUE;
	}
	if (slave_wait) { 
		// At this point it looks like the slave needs to wait for the master to tell us to do something 
		// Get the lock on the task counter and check to see if we need to actually wait or just slide on through to the next task.
		// slave_wait will be set for one of two reasons: either the task counter is zero or the slave
		// has finished the last task and needs to wait for the next ping from the master.
		// If the master has finished then don't bother waiting for it to enter this barrier because it never will 
fprintf(stderr,"xdd_lockstep_slave_before_io_op: I am a SLAVE, p=%p, slave_lsp=%p, waiting....\n",p,slave_lsp);
			if ((slave_lsp->ls_ms_state & LS_MASTER_FINISHED) && (slave_lsp->ls_ms_state & LS_SLAVE_COMPLETE)) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: I am a SLAVE, p=%p, slave_lsp=%p, MASTER is finished and SLAVE is Complete....\n",p,slave_lsp);
				/* if we need to simply finish all this, just release the lock and move on */
				slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (slave_lsp->ls_ms_state & LS_MASTER_WAITING) {
					slave_lsp->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_FINISHED+MASTER_WAITING... entering the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
					xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_FINISHED+MASTER_WAITING... return from the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
					slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
					slave_lsp->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&slave_lsp->ls_mutex);
				}
			} else if ((slave_lsp->ls_ms_state & LS_MASTER_FINISHED) && (slave_lsp->ls_ms_state & LS_SLAVE_STOP)) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: I am a SLAVE, p=%p, slave_lsp=%p, MASTER is finished and SLAVE is Stopped....\n",p,slave_lsp);
				/* This is the case where the master is finished and we need to stop NOW.
				 * Therefore, release the lock and break this loop.
				 */
				slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (slave_lsp->ls_ms_state & LS_MASTER_WAITING) {
					slave_lsp->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_FINISHED+COMPLETE_NOW... entering the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
					xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_FINISHED+COMPLETE_NOW... return from the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
					slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
					slave_lsp->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&slave_lsp->ls_mutex);
				}
				return(XDD_RC_GOOD);
			} else { /* At this point the master is not finished and we need to wait for something to do */
				if (slave_lsp->ls_ms_state & LS_MASTER_WAITING) {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: I am a SLAVE - MASTER is waiting so we turn off the master_waiting bit in slave state\n");
					slave_lsp->ls_ms_state &= ~LS_MASTER_WAITING;
				}
				slave_lsp->ls_ms_state |= LS_SLAVE_WAITING;
				pthread_mutex_unlock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_NOT_FINISHED+SLAVE_WAITING_FOR_TASK... entering the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
				xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
fprintf(stderr,"xdd_lockstep_slave_before_io_op: MASTER_NOT_FINISHED+SLAVE_WAITING_FOR_TASK... return from the slave lock_step_barrier %p...\n",&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index]);
				slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
				slave_lsp->ls_slave_loop_counter++;
			}
fprintf(stderr,"xdd_lockstep_slave_before_io_op: I am a SLAVE - done waiting....\n");
		} else {
fprintf(stderr,"xdd_lockstep_slave_before_io_op: NO SLAVE WAIT - done with xdd_lockstep_slave_before_io_op...\n");
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
		}
		/* At this point the slave does not have to wait for anything so keep on truckin... */

fprintf(stderr,"xdd_lockstep_slave_before_io_op: RETURN XDD_RC_GOOD.\n");
	return(XDD_RC_GOOD);

} // xdd_lockstep_slave_before_io_op()
/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_op(ptds_t *p) 
 */
int32_t
xdd_lockstep_before_io_op(ptds_t *p) {
	lockstep_t 	*lsp;	// Pointer to the lock step struct


	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
	//
	if (p->lockstepp == 0)
		return(XDD_RC_GOOD);

	lsp = p->lockstepp;

	// At this point we know that this target is part of a lockstep operation.
	// Therefore this process is either a MASTER or a SLAVE target or both and
	// "lsp" points to our lockstep structure. 
	// The "ls_slave" and "ls_master" members of our lockstep structure contain
	// the target number of our slave or our master target. 
	// If we are a MASTER, then "ls_slave" will be the target number of our SLAVE.
	// Likewise, if we are a SLAVE, then "ls_master" will be the target number 
	// of our MASTER. 
	// It is possible that we are a MASTER to one target an a SLAVE to another 
	// target. We cannot be a MASTER or SLAVE to ourself.
	//
	if (lsp->ls_slave >= 0) 
		xdd_lockstep_master_before_io_op(p);

	// If there is a pointer to a MASTER lockstep structure then we must be a SLAVE.
	//
	if (lsp->ls_master >= 0) 
		xdd_lockstep_slave_before_io_op(p);

	return(XDD_RC_GOOD);

} // xdd_lockstep_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_io_op(ptds_t *p) 
 *  It is important to note that this subroutine could process a SLAVE and
 *  a MASTER in the same call because a target can be a slave to one master
 *  *and* a master to a different slave. 
 */
int32_t
xdd_lockstep_after_io_op(ptds_t *p) {
	ptds_t		*slave_p;		// Pointer to the SLAVE's PTDS
	lockstep_t 	*lsp;			// Pointer to the  lock step struct
	lockstep_t 	*master_lsp;	// Pointer to the MASTER's lock step struct
	lockstep_t 	*slave_lsp;		// Pointer to SLAVE's lock step struct


	// Check to see if we are in lockstep with another target.
	// If so, then if we are a master, then do the master thing.
	// Then check to see if we are also a slave and do the slave thing.
	//
	if (p->lockstepp == 0)
		return(XDD_RC_GOOD);

	lsp = p->lockstepp;

	// This ought to return something other than 0 but this is it for now... 
	// If there is a slave to this target then we need to tell the slave that we (the master) are finished
	// and that it ought to abort or finish (depending on the command-line option) but in either case it
	// should no longer wait for the master to tell it to do something.
	//
	//////// SLAVE Processing /////////
	if (lsp->ls_master >= 0) {
		slave_lsp = lsp; // makes it easier to read this code
		// If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the
		// master does not inadvertently wait for the slave to complete.
		pthread_mutex_lock(&slave_lsp->ls_mutex);
		slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
		slave_lsp->ls_ms_state |= LS_SLAVE_FINISHED;
		pthread_mutex_unlock(&slave_lsp->ls_mutex);
	} // Done with SLAVE processing 

	//////// MASTER Processing /////////
	if (lsp->ls_slave >= 0) {
		master_lsp = lsp; // makes it easier to read this code
		slave_p = xgp->ptdsp[master_lsp->ls_slave]; 	
		slave_lsp = slave_p->lockstepp;
		/* Get the mutex lock for the ls_task_counter and increment the counter */
		pthread_mutex_lock(&slave_lsp->ls_mutex);
		slave_lsp->ls_ms_state |= LS_MASTER_FINISHED;
		/* At this point the slave could be in one of three places. Thus this next section of code
		** must be carefully structured to prevent a race condition between the master and the slave.
		** If the slave is simply waiting for the master to release it, then the ls_task_counter lock
		** can be released and the master can enter the lock_step_barrier to get the slave going again.
		** If the slave is running, then it will have to wait for the ls_task_counter lock and
		** when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
		** decrement the counter by 1 and continue on executing a lock.
		**/
		slave_lsp->ls_task_counter++;
		if (slave_lsp->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
			xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
			slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
		} else {
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
		}
	} // Done with MASTER processing

} // xdd_lockstep_after_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_pass() - This subroutine initializes the variables that
 * are used by the lockstep option.
 */
void
xdd_lockstep_before_pass(ptds_t *p) {
	lockstep_t *lsp;			// Pointer to the lock step struct


fprintf(stderr,"xdd_lockstep_before_pass: ENTER, p=%p\n",p);
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

fprintf(stderr,"xdd_lockstep_before_pass: EXIT, p=%p\n",p);
} // xdd_lockstep_before_pass()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_after_pass() - This subroutine will do all the stuff needed to 
 * be done for lockstep operations after an entire pass is complete.
 */
int32_t
xdd_lockstep_after_pass(ptds_t *p) {
	ptds_t	*p2;		// Secondary PTDS pointer
	lockstep_t *lsp;			// Pointer to the lock step struct
	lockstep_t *slave_lsp;			// Pointer to the lock step struct

	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
fprintf(stderr,"xdd_lockstep_after_pass: ENTER1, p=%p, lockstepp=%p\n",p,p->lockstepp);
	if (p->lockstepp == 0)
		return(XDD_RC_GOOD);
	lsp = p->lockstepp;
	if (lsp->ls_master >= 0) {
		/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 * master does not inadvertently wait for the slave to complete.
		 */
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p=%p, lsp=%p, I am a SLAVE and I have finished\n",p,lsp);
		pthread_mutex_lock(&lsp->ls_mutex);
		lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
		lsp->ls_ms_state |= LS_SLAVE_FINISHED;
		pthread_mutex_unlock(&lsp->ls_mutex);
	}
	if (lsp->ls_slave >= 0) {  
		/* Get the mutex lock for the ls_task_counter and increment the counter */
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p=%p, lsp=%p, I am a MASTER and I have finished\n",p,lsp);
		p2 = xgp->ptdsp[lsp->ls_slave];
		slave_lsp = p2->lockstepp;
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p2=%p, slave_lsp=%p, I am a MASTER and I have finished - slave's task counter is %d\n",p2,slave_lsp,slave_lsp->ls_task_counter);
		pthread_mutex_lock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p=%p, lsp=%p, I am a MASTER and I have finished - got the slave's lock\n",p,lsp);
		slave_lsp->ls_ms_state |= LS_MASTER_FINISHED;
		/* At this point the slave could be in one of three places. Thus this next section of code
			* must be carefully structured to prevent a race condition between the master and the slave.
			* If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			* can be released and the master can enter the lock_step_barrier to get the slave going again.
			* If the slave is running, then it will have to wait for the ls_task_counter lock and
			* when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			* decrement the counter by 1 and continue on executing a lock. 
			*/
		slave_lsp->ls_task_counter++;
		if (slave_lsp->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
			slave_lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p=%p, lsp=%p, I am a MASTER Entering slave's lock step barrier - index %d\n",p,lsp,slave_lsp->Lock_Step_Barrier_Slave_Index);
			xdd_barrier(&slave_lsp->Lock_Step_Barrier[slave_lsp->Lock_Step_Barrier_Slave_Index],&p->occupant,0);
fprintf(stderr,"xdd_lockstep_after_pass: ENTER2, p=%p, lsp=%p, I am a MASTER out of slave's lock step barrier - index %d\n",p,lsp,slave_lsp->Lock_Step_Barrier_Slave_Index);
			slave_lsp->Lock_Step_Barrier_Slave_Index ^= 1;
		} else {
			pthread_mutex_unlock(&slave_lsp->ls_mutex);
		}
	}

fprintf(stderr,"xdd_lockstep_after_pass: EXIT, p=%p\n",p);
	return(XDD_RC_GOOD);

} /* end of xdd_lockstep_after_pass() */
