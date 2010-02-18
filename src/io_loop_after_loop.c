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
// After I/O Loop
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_lockstep_ after_io_loop() - This subroutine will do all the stuff needed to 
 * be done for lockstep operations after an entire pass is complete.
 */
int32_t
xdd_lockstep_after_io_loop(ptds_t *p) {
	ptds_t	*p2;		// Secondary PTDS pointer

	/* This ought to return something other than 0 but this is it for now... */
	/* If there is a slave to this target then we need to tell the slave that we (the master) are finished
	 * and that it ought to abort or finish (depending on the command-line option) but in either case it
	 * should no longer wait for the master to tell it to do something.
	 */
	if (p->ls_master >= 0) {
		/* If this is the slave and we are finishing first, turn off the SLAVE_WAITING flag so the 
		 * master does not inadvertently wait for the slave to complete.
		 */
		pthread_mutex_lock(&p->ls_mutex);
		p->ls_ms_state &= ~LS_SLAVE_WAITING;
		p->ls_ms_state |= LS_SLAVE_FINISHED;
		pthread_mutex_unlock(&p->ls_mutex);
	}
	if (p->ls_slave >= 0) {  
		/* Get the mutex lock for the ls_task_counter and increment the counter */
		p2 = xgp->ptdsp[p->ls_slave];
		pthread_mutex_lock(&p2->ls_mutex);
		p2->ls_ms_state |= LS_MASTER_FINISHED;
		/* At this point the slave could be in one of three places. Thus this next section of code
			* must be carefully structured to prevent a race condition between the master and the slave.
			* If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			* can be released and the master can enter the lock_step_barrier to get the slave going again.
			* If the slave is running, then it will have to wait for the ls_task_counter lock and
			* when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			* decrement the counter by 1 and continue on executing a lock. 
			*/
		p2->ls_task_counter++;
		if (p2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
			p2->ls_ms_state &= ~LS_SLAVE_WAITING;
			pthread_mutex_unlock(&p2->ls_mutex);
			xdd_barrier(&p2->Lock_Step_Barrier[p2->Lock_Step_Barrier_Master_Index]);
			p2->Lock_Step_Barrier_Master_Index ^= 1;
		} else {
			pthread_mutex_unlock(&p2->ls_mutex);
		}
	}

	return(0);

} /* end of xdd_lockstep_after_ioloop() */

/*----------------------------------------------------------------------------*/
/* xdd_io_loop_after_loop() - This subroutine will do all the stuff needed to 
 * be done after the inner-most loop has done does all the I/O operations
 * that constitute a "pass" or some portion of a pass if it terminated early.
 */
int32_t
xdd_io_loop_after_loop(ptds_t *p) {
	int32_t  status;


	/* Get the ending time stamp */
	pclk_now(&p->my_pass_end_time);
	p->my_elapsed_pass_time = p->my_pass_end_time - p->my_pass_start_time;

	/* Get the current CPU user and system times and the effective current wall clock time using pclk_now() */
	times(&p->my_current_cpu_times);

	status = xdd_lockstep_after_io_loop(p);

	if (status) return(1);

	return(0);
} // End of xdd_io_loop_after_loop()

 
