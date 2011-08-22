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
// Things the Target Thread has to do before each I/O Operation is issued
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_syncio_before_io_op() - This subroutine will enter the syncio 
 * barrier if it has reached the syncio-number-of-ops. Once all the other 
 * threads enter this barrier then they will all get released and I/O starts 
 * up again.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
void
xdd_syncio_before_io_op(ptds_t *p) {


	if ((xgp->syncio > 0) && 
	    (xgp->number_of_targets > 1) && 
	    (p->my_current_op_number % xgp->syncio == 0)) {
		xdd_barrier(&xgp->main_targets_syncio_barrier,&p->occupant,0);
	}


} // End of xdd_syncio_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_start_trigger_before_io_op() - This subroutine will wait for a 
 * trigger and signal another target to start if necessary.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_start_trigger_before_io_op(ptds_t *p) {
	ptds_t			*p2;	// Ptr to the ptds that we need to trigger
	xdd_triggers_t	*trigp1;
	xdd_triggers_t	*trigp2;
	pclk_t			tt;	// Trigger Time


	/* Check to see if we need to wait for another target to trigger us to start.
 	* If so, then enter the trigger_start barrier and have a beer until we get the signal to
 	* jump into action!
 	*/
	trigp1 = p->trigp;
	if (trigp1 == NULL)
		return(0);

	if ((p->target_options & TO_WAITFORSTART) && (trigp1->run_status == 0)) { 
		/* Enter the barrier and wait to be told to go */
		xdd_barrier(&trigp1->target_target_starttrigger_barrier,&p->occupant,1);
		trigp1->run_status = 1; /* indicate that we have been released */
	}

	/* Check to see if we need to signal some other target to start, stop, or pause.
	 * If so, tickle the appropriate semaphore for that target and get on with our business.
	 */
	if (trigp1->trigger_types) {
		p2 = xgp->ptdsp[trigp1->start_trigger_target];
		trigp2 = p2->trigp;
		if (trigp2->run_status == 0) {
			if (trigp1->trigger_types & TRIGGER_STARTTIME) {
			/* If we are past the start time then signal the specified target to start */
				nclk_now(&tt);
				if (tt > (trigp1->start_trigger_time + p->my_pass_start_time)) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&p->occupant,0);
				}
			}
			if (trigp1->trigger_types & TRIGGER_STARTOP) {
				/* If we are past the specified operation, then signal the specified target to start */
				if (p->my_current_op_number > trigp1->start_trigger_op) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&p->occupant,0);
				}
			}
			if (trigp1->trigger_types & TRIGGER_STARTPERCENT) {
				/* If we have completed percentage of operations then signal the specified target to start */
				if (p->my_current_op_number > (trigp1->start_trigger_percent * p->qthread_ops)) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&p->occupant,0);
				}
			}
			if (trigp1->trigger_types & TRIGGER_STARTBYTES) {
				/* If we have completed transferring the specified number of bytes, then signal the 
				* specified target to start 
				*/
				if (p->my_current_bytes_xfered > trigp1->start_trigger_bytes) {
					xdd_barrier(&trigp2->target_target_starttrigger_barrier,&p->occupant,0);
				}
			}
		}
	} /* End of the trigger processing */
	return(0);

} // End of xdd_start_trigger_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_timelimit_before_io_op() - This subroutine will check to see if the
 * specified time limit for this pass has expired. 
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_timelimit_before_io_op(ptds_t *p) {
	pclk_t	current_time;		// What time is it *now*?
	pclk_t	elapsed_time;		// Elapsed time


	/* Check to see if a time limit (in seconds) was specified.
 	* If so, then check to see if we have exceeded that amount of time and
 	* set the global variable "time_limit_expired". 
    * Otherwise, return 0 and continue issuing I/Os.
 	*/
	if (p->time_limit_ticks) { 
		nclk_now(&current_time);
		elapsed_time = current_time - p->my_pass_start_time;
		if (elapsed_time >= p->time_limit_ticks) {
			p->my_time_limit_expired = 1;
			fprintf(xgp->output,"\n%s: xdd_timelimit_before_io_op: Target %d: Specified time limit of %f seconds exceeded.\n",
		 		xgp->progname,
			 	p->my_target_number,
			 	p->time_limit);
		}
	}

	return(0);

} // End of xdd_timelimit_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_runtime_before_io_op() - This subroutine will check to see if the
 * specified time limit for this run has expired. 
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_runtime_before_io_op(ptds_t *p) {
	pclk_t	current_time;		// What time is it *now*?
	pclk_t	elapsed_time;		// Elapsed time


	/* Check to see if a time limit (in seconds) was specified.
 	* If so, then check to see if we have exceeded that amount of time and
 	* set the global variable "time_limit_expired". 
    * Otherwise, return 0 and continue issuing I/Os.
 	*/
	if (xgp->run_time_ticks) { 
		nclk_now(&current_time);
		elapsed_time = current_time - xgp->run_start_time;
		if (elapsed_time >= xgp->run_time_ticks) {
			xgp->run_time_expired = 1;
			fprintf(xgp->output,"\n%s: xdd_runtime_before_io_op: Specified run time of %f seconds exceeded.\n",
		 		xgp->progname,
			 	xgp->run_time);
		}
	}

	return(0);

} // End of xdd_runtime_before_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_target_ttd_before_io_op() - This subroutine will do all the stuff 
 * needed to be done by the Target Thread before a QThread is issued with 
 * an I/O task.
 * 
 * This subroutine is called within the context of a Target Thread.
 *
 */
int32_t
xdd_target_ttd_before_io_op(ptds_t *p) {
	int32_t	status;	// Return status from various subroutines

	// Syncio barrier - wait for all others to get here 
	xdd_syncio_before_io_op(p);

	// Check to see if we need to wait for another target to trigger us to start.
	xdd_start_trigger_before_io_op(p);

	// Check to see if we exceeded the time limit for this pass
	xdd_timelimit_before_io_op(p);

	// Check to see if we exceeded the time limit for this run
	xdd_runtime_before_io_op(p);

	// Lock Step Processing (located in lockstep.c)
	status = xdd_lockstep_before_io_op(p);
	if (status) return(-1);

	/* init the error number and break flag for good luck */
	errno = 0;
	/* Get the location to seek to */
	if (p->seekhdr.seek_options & SO_SEEK_NONE) /* reseek to starting offset if noseek is set */
		p->my_current_byte_location = (uint64_t)((p->my_target_number * xgp->target_offset) + 
											p->seekhdr.seeks[0].block_location) * 
											p->block_size;
	else p->my_current_byte_location = (uint64_t)((p->my_target_number * xgp->target_offset) + 
											p->seekhdr.seeks[p->my_current_op_number].block_location) * 
											p->block_size;

	if (xgp->global_options & GO_INTERACTIVE)	
		xdd_barrier(&xgp->interactive_barrier,&p->occupant,0);

	return(0);

} // End of xdd_target_ttd_before_io_op()

 
