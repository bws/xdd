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
// Before I/O Operation
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_syncio_before_io_operation() - This subroutine will enter the syncio 
 * barrier if it has reached the syncio-number-of-ops. Once all the other 
 * threads enter this barrier then they will all get released and I/O starts 
 * up again.
 */
void
xdd_syncio_before_io_operation(ptds_t *p) {


	if ((xgp->syncio > 0) && 
	    (xgp->number_of_targets > 1) && 
	    (p->my_current_op % xgp->syncio == 0)) {
		xdd_barrier(&xgp->syncio_barrier[p->syncio_barrier_index]);
		p->syncio_barrier_index ^= 1; /* toggle barrier index */
	}


} // End of xdd_syncio_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_start_trigger_before_io_operation() - This subroutine will wait for a 
 * trigger and signal another target to start if necessary.
 * Return Values: 0 is good
 *                1 is bad
 */
int32_t
xdd_start_trigger_before_io_operation(ptds_t *p) {
	ptds_t	*p2;	// Ptr to the ptds that we need to trigger
	pclk_t	tt;	// Trigger Time


	/* Check to see if we need to wait for another target to trigger us to start.
 	* If so, then enter the trigger_start barrier and have a beer until we get the signal to
 	* jump into action!
 	*/
	if ((p->target_options & TO_WAITFORSTART) && (p->run_status == 0)) { 
		/* Enter the barrier and wait to be told to go */
		xdd_barrier(&p->Start_Trigger_Barrier[p->Start_Trigger_Barrier_index]);
		p->Start_Trigger_Barrier_index ^= 1;
		p->run_status = 1; /* indicate that we have been released */
		return(1);
	}

	/* Check to see if we need to signal some other target to start, stop, or pause.
	 * If so, tickle the appropriate semaphore for that target and get on with our business.
	 */
	if (p->trigger_types) {
		p2 = xgp->ptdsp[p->start_trigger_target];
		if (p2->run_status == 0) {
			if (p->trigger_types & TRIGGER_STARTTIME) {
			/* If we are past the start time then signal the specified target to start */
				pclk_now(&tt);
				if (tt > (p->start_trigger_time + p->my_pass_start_time)) {
					xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
				}
			}
			if (p->trigger_types & TRIGGER_STARTOP) {
				/* If we are past the specified operation, then signal the specified target to start */
				if (p->my_current_op > p->start_trigger_op) {
					xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
				}
			}
			if (p->trigger_types & TRIGGER_STARTPERCENT) {
				/* If we have completed percentage of operations then signal the specified target to start */
				if (p->my_current_op > (p->start_trigger_percent * p->target_ops)) {
					xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
				}
			}
			if (p->trigger_types & TRIGGER_STARTBYTES) {
				/* If we have completed transferring the specified number of bytes, then signal the 
				* specified target to start 
				*/
				if (p->my_current_bytes_xfered > p->start_trigger_bytes) {
					xdd_barrier(&p2->Start_Trigger_Barrier[p2->Start_Trigger_Barrier_index]);
				}
			}
		}
	} /* End of the trigger processing */

} // End of xdd_start_trigger_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_lockstep_before_io_operation(ptds_t *p) {
 */
int32_t	// Lock Step Processing
xdd_lockstep_before_io_operation(ptds_t *p) {
	int32_t	ping_slave;			// indicates whether or not to ping the slave 
	int32_t	slave_wait;			// indicates the slave should wait before continuing on 
	pclk_t   time_now;			// used by the lock step functions 
	ptds_t	*p2;				// Secondary PTDS pointer

	/* Check to see if we are in lockstep with another target.
	 * If so, then if we are a master, then do the master thing.
	 * Then check to see if we are also a slave and do the slave thing.
	 */
	if (p->ls_slave >= 0) { /* Looks like we are the MASTER in lockstep with a slave target */
		p2 = xgp->ptdsp[p->ls_slave]; /* This is a pointer to the slave ptds */
		ping_slave = FALSE;
		/* Check to see if it is time to ping the slave to do something */
		if (p->ls_interval_type & LS_INTERVAL_TIME) {
			/* If we are past the start time then signal the specified target to start.
			 */
			pclk_now(&time_now);
			if (time_now > (p->ls_interval_value + p->ls_interval_base_value)) {
				ping_slave = TRUE;
				p->ls_interval_base_value = time_now; /* reset the new base time */
			}
		}
		if (p->ls_interval_type & LS_INTERVAL_OP) {
			/* If we are past the specified operation, then signal the specified target to start.
			 */
			if (p->my_current_op >= (p->ls_interval_value + p->ls_interval_base_value)) {
				ping_slave = TRUE;
				p->ls_interval_base_value = p->my_current_op;
			}
		}
		if (p->ls_interval_type & LS_INTERVAL_PERCENT) {
			/* If we have completed percentage of operations then signal the specified target to start.
			 */
			if (p->my_current_op >= ((p->ls_interval_value*p->ls_interval_base_value) * p->target_ops)) {
				ping_slave = TRUE;
				p->ls_interval_base_value++;
			}
		}
		if (p->ls_interval_type & LS_INTERVAL_BYTES) {
			/* If we have completed transferring the specified number of bytes, then signal the 
			 * specified target to start.
			 */
			if (p->my_current_bytes_xfered >= (p->ls_interval_value + (int64_t)(p->ls_interval_base_value))) {
				ping_slave = TRUE;
				p->ls_interval_base_value = p->my_current_bytes_xfered;
			}
		}
		if (ping_slave) { /* Looks like it is time to ping the slave to do something */
			/* note that the SLAVE owns the mutex and the counter used to tell it to do something */
			/* Get the mutex lock for the ls_task_counter and increment the counter */
			pthread_mutex_lock(&p2->ls_mutex);
			/* At this point the slave could be in one of three places. Thus this next section of code
			 * must be carefully structured to prevent a race condition between the master and the slave.
			 * If the slave is simply waiting for the master to release it, then the ls_task_counter lock
			 * can be released and the master can enter the lock_step_barrier to get the slave going again.
			 * If the slave is running, then it will have to wait for the ls_task_counter lock and
			 * when it gets the lock, it will discover that the ls_task_counter is non-zero and will simply
			 * decrement the counter by 1 and continue on executing a lock. 
			 */
			p2->ls_task_counter++;
			/* Now that we have updated the slave's task counter, we need to check to see if the slave is 
			 * waiting for us to give it a kick. If so, enter the barrier which will release the slave and
			 * us as well. 
			 * ****************Currently this is set up simply to do overlapped lockstep.*******************
			 */
			if (p2->ls_ms_state & LS_SLAVE_WAITING) { /* looks like the slave is waiting for something to do */
				p2->ls_ms_state &= ~LS_SLAVE_WAITING;
				pthread_mutex_unlock(&p2->ls_mutex);
				xdd_barrier(&p2->Lock_Step_Barrier[p2->Lock_Step_Barrier_Master_Index]);
				p2->Lock_Step_Barrier_Master_Index ^= 1;
				/* At this point the slave outght to be running. */
			} else {
				pthread_mutex_unlock(&p2->ls_mutex);
			}
		} /* Done pinging the slave */
	} /* end of SLAVE processing as a MASTER */
	if (p->ls_master >= 0) { /* Looks like we are a slave to some other MASTER target */
		/* As a slave, we need to check to see if we should stop running at this point. If not, keep on truckin'.
		 */   
		/* Look at the type of task that we need to perform and the quantity. If we have exhausted
		 * the quantity of operations for this interval then enter the lockstep barrier.
		 */
		pthread_mutex_lock(&p->ls_mutex);
		if (p->ls_task_counter > 0) {
			/* Ahhhh... we must be doing something... */
			slave_wait = FALSE;
			if (p->ls_task_type & LS_TASK_TIME) {
				/* If we are past the start time then signal the specified target to start.
				*/
				pclk_now(&time_now);
				if (time_now > (p->ls_task_value + p->ls_task_base_value)) {
					slave_wait = TRUE;
					p->ls_task_base_value = time_now;
					p->ls_task_counter--;
				}
			}
			if (p->ls_task_type & LS_TASK_OP) {
				/* If we are past the specified operation, then indicate slave to wait.
				*/
				if (p->my_current_op >= (p->ls_task_value + p->ls_task_base_value)) {
					slave_wait = TRUE;
					p->ls_task_base_value = p->my_current_op;
					p->ls_task_counter--;
				}
			}
			if (p->ls_task_type & LS_TASK_PERCENT) {
				/* If we have completed percentage of operations then indicate slave to wait.
				*/
				if (p->my_current_op >= ((p->ls_task_value * p->ls_task_base_value) * p->target_ops)) {
					slave_wait = TRUE;
					p->ls_task_base_value++;
					p->ls_task_counter--;
				}
			}
			if (p->ls_task_type & LS_TASK_BYTES) {
				/* If we have completed transferring the specified number of bytes, then indicate slave to wait.
				*/
				if (p->my_current_bytes_xfered >= (p->ls_task_value + (int64_t)(p->ls_task_base_value))) {
					slave_wait = TRUE;
					p->ls_task_base_value = p->my_current_bytes_xfered;
					p->ls_task_counter--;
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
			if ((p->ls_ms_state & LS_MASTER_FINISHED) && (p->ls_ms_state & LS_SLAVE_COMPLETE)) {
				/* if we need to simply finish all this, just release the lock and move on */
				p->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (p->ls_ms_state & LS_MASTER_WAITING) {
					p->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&p->ls_mutex);
					xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
					p->Lock_Step_Barrier_Slave_Index ^= 1;
					p->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&p->ls_mutex);
				}
			} else if ((p->ls_ms_state & LS_MASTER_FINISHED) && (p->ls_ms_state & LS_SLAVE_STOP)) {
				/* This is the case where the master is finished and we need to stop NOW.
				 * Therefore, release the lock, set the my_pass_ring to true and break this loop.
				 */
				p->ls_ms_state &= ~LS_SLAVE_WAITING; /* Slave is no longer waiting for anything */
				if (p->ls_ms_state & LS_MASTER_WAITING) {
					p->ls_ms_state &= ~LS_MASTER_WAITING;
					pthread_mutex_unlock(&p->ls_mutex);
					xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
					p->Lock_Step_Barrier_Slave_Index ^= 1;
					p->ls_slave_loop_counter++;
				} else {
					pthread_mutex_unlock(&p->ls_mutex);
				}
				p->my_pass_ring = TRUE;
				return(1);
			} else { /* At this point the master is not finished and we need to wait for something to do */
				if (p->ls_ms_state & LS_MASTER_WAITING)
					p->ls_ms_state &= ~LS_MASTER_WAITING;
				p->ls_ms_state |= LS_SLAVE_WAITING;
				pthread_mutex_unlock(&p->ls_mutex);
				xdd_barrier(&p->Lock_Step_Barrier[p->Lock_Step_Barrier_Slave_Index]);
				p->Lock_Step_Barrier_Slave_Index ^= 1;
				p->ls_slave_loop_counter++;
			}
		} else {
			pthread_mutex_unlock(&p->ls_mutex);
		}
		/* At this point the slave does not have to wait for anything so keep on truckin... */

	} // End of being the master in a lockstep op
	return(0);

} // xdd_lockstep_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_raw_before_io_operation(ptds_t *p) {
 */
void	// Read-After_Write Processing
xdd_raw_before_io_operation(ptds_t *p) {
	int	status;

#if (IRIX || SOLARIS || AIX )
	struct stat64	statbuf;
#elif (LINUX || OSX || FREEBSD)
	struct stat	statbuf;
#endif

#if (LINUX || IRIX || SOLARIS || AIX || OSX || FREEBSD)
		if ((p->target_options & TO_READAFTERWRITE) && (p->target_options & TO_RAW_READER)) { 
// fprintf(stderr,"Reader: RAW check - dataready=%lld, trigger=%x\n",(long long)data_ready,p->raw_trigger);
			/* Check to see if we can read more data - if not see where we are at */
			if (p->raw_trigger & PTDS_RAW_STAT) { /* This section will continually poll the file status waiting for the size to increase so that it can read more data */
				while (p->raw_data_ready < p->iosize) {
					/* Stat the file so see if there is data to read */
#if (LINUX || OSX || FREEBSD)
					status = fstat(p->fd,&statbuf);
#else
					status = fstat64(p->fd,&statbuf);
#endif
					if (status < 0) {
						fprintf(xgp->errout,"%s: RAW: Error getting status on file\n", xgp->progname);
						p->raw_data_ready = p->iosize;
					} else { /* figure out how much more data we can read */
						p->raw_data_ready = statbuf.st_size - p->my_current_byte_location;
						if (p->raw_data_ready < 0) {
							/* The result of this should be positive, otherwise, the target file
							* somehow got smaller and there is a problem. 
							* So, fake it and let this loop exit 
							*/
							fprintf(xgp->errout,"%s: RAW: Something is terribly wrong with the size of the target file...\n",xgp->progname);
							p->raw_data_ready = p->iosize;
						}
					}
				}
			} else { /* This section uses a socket connection to the Destination and waits for the Source to tell it to receive something from its socket */
				while (p->raw_data_ready < p->iosize) {
					/* xdd_raw_read_wait() will block until there is data to read */
					status = xdd_raw_read_wait(p);
					if (p->raw_msg.length != p->iosize) 

						fprintf(stderr,"error on msg recvd %d loc %lld, length %lld\n",
							p->raw_msg_recv-1, 
							(long long)p->raw_msg.location,  
							(long long)p->raw_msg.length);
					if (p->raw_msg.sequence != p->raw_msg_last_sequence) {

						fprintf(stderr,"sequence error on msg recvd %d loc %lld, length %lld seq num is %lld should be %lld\n",
							p->raw_msg_recv-1, 
							(long long)p->raw_msg.location,  
							(long long)p->raw_msg.length, 
							p->raw_msg.sequence, 
							p->raw_msg_last_sequence);
					}
					if (p->raw_msg_last_sequence == 0) { /* this is the first message so prime the prev_loc and length with the current values */
						p->raw_prev_loc = p->raw_msg.location;
						p->raw_prev_len = 0;
					} else if (p->raw_msg.location <= p->raw_prev_loc) 
						/* this message is old and can be discgarded */
						continue;
					p->raw_msg_last_sequence++;
					/* calculate the amount of data to be read between the end of the last location and the end of the current one */
					p->raw_data_length = ((p->raw_msg.location + p->raw_msg.length) - (p->raw_prev_loc + p->raw_prev_len));
					p->raw_data_ready += p->raw_data_length;
					if (p->raw_data_length > p->iosize) 
						fprintf(stderr,"msgseq=%lld, loc=%lld, len=%lld, data_length is %lld, data_ready is now %lld, iosize=%d\n",
							p->raw_msg.sequence, 
							(long long)p->raw_msg.location, 
							(long long)p->raw_msg.length, 
							(long long)p->raw_data_length, 
							(long long)p->raw_data_ready, 
							p->iosize );
					p->raw_prev_loc = p->raw_msg.location;
					p->raw_prev_len = p->raw_data_length;
				}
			}
		} /* End of dealing with a read-after-write */
#endif
} // xdd_raw_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_e2e_before_io_operation() - This routine only does something if this
 * is the Destination side of an End-to-End operation.
 */
int32_t	
xdd_e2e_before_io_operation(ptds_t *p) {
	pclk_t	beg_time_tmp; 	// Beginning time of a data xfer
	pclk_t	end_time_tmp; 	// End time of a data xfer
	pclk_t	now; 	// End time of a data xfer
	int32_t	status;			// Status of subroutine calls


if (xgp->global_options & GO_DEBUG) 
fprintf(stderr,"e2e_before_io_operation: target_options=0x%016x\n",p->target_options);
	// If there is no end-to-end operation then just skip all this...
	if (!(p->target_options & TO_ENDTOEND)) 
		return(SUCCESS); 
	// We are the Source side - nothing to do - leave now...
	if (p->target_options & TO_E2E_SOURCE)
		return(SUCCESS);

	/* ------------------------------------------------------ */
	/* Start of destination's dealing with an End-to-End op   */
	/* This section uses a socket connection to the source    */
	/* to wait for data from the source, which it then writes */
	/* to the target file associated with this rarget 	  */
	/* ------------------------------------------------------ */
	// We are the Destination side of an End-to-End op
	if (xgp->global_options & GO_DEBUG) {
		fprintf(stderr,"e2e_before_io_operation: data_ready=%lld, current_op=%d,prev_loc=%lld, prev_len=%lld, iosize=%d\n",
			p->e2e_data_ready, 
			p->my_current_op,
			p->e2e_prev_loc, 
			p->e2e_prev_len, 
			p->iosize);
	}
	// Lets read all the dta from the source 
	while (p->e2e_data_ready < p->iosize) {
		/* xdd_e2e_dest_wait() will block until there is data to read */
		pclk_now(&beg_time_tmp);
		status = xdd_e2e_dest_wait(p);
		pclk_now(&end_time_tmp);
		p->e2e_sr_time += (end_time_tmp - beg_time_tmp); // Time spent reading from the source machine
		// If status is "FAILED" then soemthing happened to the connection - time to leave
		if (status == FAILED) {
			fprintf(stderr,"%s: [mythreadnum %d]:e2e_before_io_operation: Connection closed prematurely by source!\n",
				xgp->progname,p->my_qthread_number);
			return(FAILED);
		}
			
		// Check to see that the length of the msg is what we think it should be
		if (p->e2e_msg.length != p->iosize) {
			fprintf(stderr,"%s: [my_qthread_number %d]:e2e_before_io_operation: error on msg recvd %d loc %lld, length %lld\n",
				xgp->progname,p->my_qthread_number,p->e2e_msg_recv-1, p->e2e_msg.location,  p->e2e_msg.length);
			if (p->e2e_useUDP) {
				fprintf(stderr,"%s: [mythreadnum %d]:e2e_before_io_operation: UDP:error on msg recvd %d ...lost header...dont write!!!\n",
					xgp->progname,p->mythreadnum,p->e2e_msg_recv-1);
				continue;
			}
		}
		// Check to see of this is the last message in the transmission
		// If so, then set the "my_pass_ring" so that we exit gracefully
		if (p->e2e_msg.magic == PTDS_E2E_MAGIQ) 
			p->my_pass_ring = 1;

		// Check the sequence number of the reseived msg to what we expect it to be
		// Also note that the msg magic number should not be MAGIQ (end of transmission)
		if ((p->e2e_msg.sequence != p->e2e_msg_last_sequence) && (p->e2e_msg.magic != PTDS_E2E_MAGIQ )) {
			fprintf(stderr,"%s: [my_qthread_number %d]:sequence error on msg recvd %d loc %lld, length %lld seq num is %lld should be %lld\n",
				xgp->progname,
				p->my_qthread_number, 
				p->e2e_msg_recv-1, 
				p->e2e_msg.location,  
				p->e2e_msg.length, 
				p->e2e_msg.sequence, 
				p->e2e_msg_last_sequence);
			return(FAILED);
		}
		// Check for a timeout condition - this only happens if we are using UDP
		if (p->e2e_timedout) {
			fprintf(stderr,"%s: [mythreadnum %d]:timedout...go on to next pass or quit if last pass\n", 
			xgp->progname,
			p->mythreadnum);
			pclk_now(&now);
			return(FAILED);
		}
		// Display some useful information if we are debugging this thing
		if (xgp->global_options & GO_DEBUG) {
			fprintf(stderr, "[mythreadnum %d]:e2e_before_io_operation: XXXXXXXX  msg.sequence=%lld, msg.location=%lld, msg.length=%lld, msg_last_sequence=%lld\n",
				p->mythreadnum, p->e2e_msg.sequence, p->e2e_msg.location, p->e2e_msg.length, p->e2e_msg_last_sequence);
			fprintf(stderr, "[mythreadnum %d]:e2e_before_io_operation: XXXXXXXX  data_length=%lld, data_ready=%lld, iosize=%d\n",
				p->mythreadnum, p->e2e_data_length, p->e2e_data_ready, p->iosize );
			fprintf(stderr, "[mythreadnum %d]:e2e_before_io_operation: XXXXXXXX  prev_loc=%lld, prev_len=%lld\n",
				p->mythreadnum, p->e2e_prev_loc, p->e2e_prev_len );
		}
		// Check to see which message we are on and set up the msg counters properly
		if (p->e2e_msg_last_sequence == 0) { 
			// This is the first message so prime the prev_loc and length with the current values 
			p->e2e_prev_loc = p->e2e_msg.location;
			p->e2e_prev_len = 0;
		} else if (p->e2e_msg.location <= p->e2e_prev_loc) {
			fprintf(stderr,"[mythreadnum %d]:e2e_before_io_operation: OLD MESSAGE\n", p->mythreadnum); 
			// This message is old and can be discgarded 
			continue;
		}

		// The e2e_msg_last_sequence variable is the local value of what we think e2e_msg.sequence
		//   should be in the incoming msg
		p->e2e_msg_last_sequence++;

		// Calculate the amount of data to be read between the end of 
		//   the last location and the end of the current one 
		p->e2e_data_length = p->e2e_msg.length;
		p->e2e_data_ready += p->e2e_data_length;
		p->e2e_prev_loc = p->e2e_msg.location;
		p->e2e_prev_len = p->e2e_data_length;
	}  // End of WHILE loop that processes an End-to-End test

	// For End-to-End, set the relative location of this data to where the SOURCE 
	//  machine thinks it should be.
	p->my_current_byte_location = p->e2e_msg.location;

	return(SUCCESS);

} // xdd_e2e_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_ts_before_io_operation() - This routine will resord information in the
 * timestamp table if needed.
 */
void	
xdd_ts_before_io_operation(ptds_t *p) {
	pclk_t	tt;	// Temp Time


	/* If time stamping is no on then just return */
	if ((p->ts_options & TS_ON) == 0)
		return;

	/* We record information only if the trigger time or operation has been reached or if we record all */
	pclk_now(&tt);
	if ((p->ts_options & TS_TRIGGERED) || 
	    (p->ts_options & TS_ALL) ||
	    ((p->ts_options & TS_TRIGTIME) && (tt >= p->ts_trigtime)) ||
	    ((p->ts_options & TS_TRIGOP) && (p->ts_trigop == p->my_current_op))) {
		p->ts_options |= TS_TRIGGERED;
		p->ttp->tte[p->ttp->tte_indx].rwvop = p->seekhdr.seeks[p->my_current_op].operation;
		p->ttp->tte[p->ttp->tte_indx].pass = p->my_current_pass_number;
		p->ttp->tte[p->ttp->tte_indx].byte_location = p->my_current_byte_location;
		p->ttp->tte[p->ttp->tte_indx].opnumber = p->my_current_op;
		p->ttp->tte[p->ttp->tte_indx].start = tt;
		p->timestamps++;
	}
} // xdd_ts_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_throttle_before_io_operation() - This routine implements the throttling
 * mechanism which is essentially a delay before the next I/O operation such
 * that the overall bandwdith or IOP rate meets the throttled value.
 */
void	
xdd_throttle_before_io_operation(ptds_t *p) {
	pclk_t   sleep_time;         /* This is the number of pico seconds to sleep between I/O ops */
	int32_t  sleep_time_dw;     /* This is the amount of time to sleep in milliseconds */
	pclk_t	now;


	if (p->throttle <= 0.0)
		return;

	/* If this is a 'throttled' operation, check to see what time it is relative to the start
	 * of this pass, compare that to the time that this operation was supposed to begin, and
	 * go to sleep for how ever many milliseconds is necessary until the next I/O needs to be
	 * issued. If we are past the issue time for this operation, just issue the operation.
	 */
	if (p->throttle > 0.0) {
		pclk_now(&now);
		if (p->throttle_type & PTDS_THROTTLE_DELAY) {
			sleep_time = p->throttle*1000000;
fprintf(stderr,"throttle delay for %d seconds\n",sleep_time);
		} else { // Process the throttle for IOPS or BW
			now -= p->my_pass_start_time;
			if (now < p->seekhdr.seeks[p->my_current_op].time1) { /* Then we may need to sleep */
				sleep_time = (p->seekhdr.seeks[p->my_current_op].time1 - now) / BILLION; /* sleep time in milliseconds */
				if (sleep_time > 0) {
					sleep_time_dw = sleep_time;
#ifdef WIN32
					Sleep(sleep_time_dw);
#elif (LINUX || IRIX || AIX || OSX || FREEBSD) /* Change this line to use usleep */
					if ((sleep_time_dw*CLK_TCK) > 1000) /* only sleep if it will be 1 or more ticks */
#if (IRIX )
						sginap((sleep_time_dw*CLK_TCK)/1000);
#elif (LINUX || AIX || OSX || FREEBSD) /* Change this line to use usleep */
fprintf(stderr,"throttle sleeping for %d seconds\n",sleep_time_dw*1000);
						usleep(sleep_time_dw*1000);
#endif
#endif
				}
			}
		}
	}
} // xdd_throttle_before_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_io_loop_before_io_operation() - This subroutine will do all the stuff 
 * needed to be done before an I/O operation is issued.
 * This routine is called within the inner I/O loop before every I/O.
 */
int32_t
xdd_io_loop_before_io_operation(ptds_t *p) {
	int32_t	status;	// Return status from various subroutines

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling syncio barrier\n");
	// Syncio barrier - wait for all others to get here 
	xdd_syncio_before_io_operation(p);

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling start trigger\n");
	// Check to see if we need to wait for another target to trigger us to start.
	xdd_start_trigger_before_io_operation(p);

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling lockstep\n");
	// Lock Step Processing
	status = xdd_lockstep_before_io_operation(p);
	if (status) return(FAILED);

	/* init the error number and break flag for good luck */
	errno = 0;
	p->my_error_break = 0;
	/* Get the location to seek to */
	if (p->seekhdr.seek_options & SO_SEEK_NONE) /* reseek to starting offset if noseek is set */
		p->my_current_byte_location = (uint64_t)((p->my_target_number * xgp->target_offset) + 
											p->seekhdr.seeks[0].block_location) * 
											p->block_size;
	else p->my_current_byte_location = (uint64_t)((p->my_target_number * xgp->target_offset) + 
											p->seekhdr.seeks[p->my_current_op].block_location) * 
											p->block_size;
	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling raw\n");
	// Read-After_Write Processing
	xdd_raw_before_io_operation(p);

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling e2e\n");
	// End-to-End Processing
	status = xdd_e2e_before_io_operation(p);
	if (status == FAILED) {
		fprintf(xgp->errout,"%s: [mythreadnum %d]: io_loop_before_io_operation: Requesting termination due to previous error.\n",
			xgp->progname, p->mythreadnum);
		xgp->abort_io;
		return(FAILED);
	}

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling timestamp\n");
	// Time Stamp Processing
	xdd_ts_before_io_operation(p);

	if (xgp->global_options & GO_DEBUG) 
		fprintf(stderr,"before_io_operation: calling throttle\n");
	// Throttle Processing
	xdd_throttle_before_io_operation(p);

	return(SUCCESS);

} // End of xdd_io_loop_before_io_operation()

 
