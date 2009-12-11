/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *  I/O Perofrmance, Inc.
 */
/*
   xdd - a basic i/o performance test
*/
#include "xdd.h"

//******************************************************************************
// After I/O Operation
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_threshold_after_io_operation() - This subroutine will do 
 * threshold checking for the -threshold option to see if the last I/O 
 * operation took more than the specified time (specified as a threshold time)
 * This routine is called by the xdd_io_loop_after_io_operation() for every I/O.
 */
void // Threshold Checking
xdd_threshold_after_io_operation(ptds_t *p) {
	pclk_t   excess_time;        /* Used by the report_threshold timing */


	if (p->report_threshold) {
		if (p->my_current_op_elapsed_time > p->report_threshold) {
			excess_time = (p->my_current_op_elapsed_time - p->report_threshold)/MILLION;
			fprintf(xgp->output, "%s: Target %d <%s> Threshold, %lld, exceeded by, %lld, microseconds, IO time was, %lld, usec on block, %lld\n",
				xgp->progname, 
				p->my_target_number, 
				p->target, 
				(long long)p->report_threshold/MILLION, 
				(long long)excess_time, 
				(long long)p->my_current_op_elapsed_time/MILLION, 
				(long long)p->my_current_byte_location);
		}
	}
} // End of xdd_threshold_after_io_operation(p)

/*----------------------------------------------------------------------------*/
/* xdd_status_after_io_operation() - This subroutine will do 
 * status checking after the I/O operation to find out if anything went wrong.
 * This routine is called by the xdd_io_loop_after_io_operation() for every I/O.
 */
void // I/O Operation Status Checking
xdd_status_after_io_operation(ptds_t *p) {

	/* Check for errors in the last operation */
	if ((p->my_current_error_count > 0) && (xgp->global_options & GO_STOP_ON_ERROR)) {
		fprintf(xgp->errout, "(%d) %s: Error on target %s caused a Stop_On_Error Event - exiting all threads and targets\n",
			p->my_target_number,xgp->progname,p->target);
		xgp->run_ring = 1;
	}

	if (p->my_current_error_count >= xgp->max_errors) {
		fprintf(xgp->errout, "(%d) %s: Maximum error threshold reached on target %s - exiting\n",
			p->my_target_number,xgp->progname,p->target);
		p->my_error_break = 1;
	}

	if ((p->my_io_status == 0) && (p->my_io_errno == 0)) {
		fprintf(xgp->errout, "(%d) %s: End-Of-File reached on target %s, status=%d, errno=%d\n",
			p->my_target_number,xgp->progname,p->target,p->my_io_status, p->my_io_errno);
		p->my_error_break = 1; /* fake an exit */
	}

} // End of xdd_status_after_io_operation(p) 

/*----------------------------------------------------------------------------*/
/* xdd_raw_after_io_operation() - This subroutine will do 
 * all the processing necessary for a read-after-write operation.
 * This routine is called by the xdd_io_loop_after_io_operation() for every I/O.
 */
void // Read-After_Write Processing
xdd_raw_after_io_operation(ptds_t *p) {

	if ((p->target_options & TO_READAFTERWRITE) && 
	    (p->target_options & TO_RAW_WRITER)) {
		/* Since I am the writer in a read-after-write operation, and if 
		 * we are using a socket connection to the reader for write-completion 
		 * messages then I need to send the reader a message of what I just 
		 * wrote - starting location and length of write.
		 */
	}
	if ( (p->my_io_status > 0) && (p->target_options & TO_READAFTERWRITE) ) {
		if (p->target_options & TO_RAW_READER) { 
			p->raw_data_ready -= p->my_io_status;
		} else { /* I must be the writer, send a message to the reader if requested */
			if (p->raw_trigger & PTDS_RAW_MP) {
				p->raw_msg.magic = PTDS_RAW_MAGIC;
				p->raw_msg.length = p->my_io_status;
				p->raw_msg.location = p->my_current_byte_location;
				xdd_raw_writer_send_msg(p);
			}
		}
	}
} // End of xdd_raw_after_io_operation(p) 

/*----------------------------------------------------------------------------*/
/* xdd_e2e_after_io_operation() - This subroutine will do 
 * all the processing necessary for an end-to-end operation.
 * This routine is called by the xdd_io_loop_after_io_operation() for every I/O.
 */
void // End-to-End Processing
xdd_e2e_after_io_operation(ptds_t *p) {
	pclk_t	beg_time_tmp;
	pclk_t	end_time_tmp;
	pclk_t	now;


	if ( (p->my_io_status > 0) && (p->target_options & TO_ENDTOEND) ) {
		if (p->target_options & TO_E2E_DESTINATION) {
			p->e2e_data_ready -= p->my_io_status;
			if((p->e2e_msg.sequence%1000 == 0) && 
			   (xgp->global_options & GO_REALLYVERBOSE)) {
				fprintf(stderr,"[mythreadnum %d]:e2e_after_io_operation: op %04d %04d: e2e-postion-destination: message %d, seq# %lld, len %ld, loc %ld, magic %08x sent %dB\n",
					p->mythreadnum,
					p->my_current_op,
					p->e2e_msg.sequence%1000,
					p->e2e_msg_recv,
					p->e2e_msg.sequence,
					p->e2e_msg.length,
					p->e2e_msg.location,
					p->e2e_msg.magic, 
					p->iosize);
			}
		} else { /* I must be the SOURCE, send current data to the DESTINATION */
			p->e2e_msg.magic = PTDS_E2E_MAGIC;
			pclk_now(&now);
			if (p->time_limit) { /* times-up, signal DESTINATION to quit */
				if ((now - p->my_pass_start_time)  >= (pclk_t)(p->time_limit*TRILLION)) {
					p->e2e_msg.magic = PTDS_E2E_MAGIQ;
					p->my_pass_ring = 1;
				}
			}
			p->e2e_msg.length = p->my_io_status;
			p->e2e_msg.location = p->my_current_byte_location;
			p->e2e_msg.sequence = p->my_current_op;
			if (xgp->global_options & GO_DEBUG) {
				fprintf(stderr,"[mythreadnum %d]:e2e_after_io_operation: op %04d:e2e-postion-source: message %d, seq# %lld, len %ld, loc %ld, magic %08x sent %d\n",
					p->mythreadnum,
					p->my_current_op,
					p->e2e_msg_sent,
					p->e2e_msg.sequence,
					p->e2e_msg.length,
					p->e2e_msg.location,
					p->e2e_msg.magic, 
					p->iosize);
			} // End of DEBUG print
			pclk_now(&beg_time_tmp);
			xdd_e2e_src_send_msg(p);
			pclk_now(&end_time_tmp);
			p->e2e_sr_time += (end_time_tmp - beg_time_tmp); // Time spent sending to the destination machine
		} // End of me being the SOURCE in an End-to-End test 
	} // End of processing a End-to-End

} // End of xdd_e2e_after_io_operation(p) 

/*----------------------------------------------------------------------------*/
/* xdd_loopcheck_after_io_operation() - This subroutine will do 
 * the necessary checks to see if we should do another I/O operation or
 * leave at this point.
 * This routine is called by the xdd_io_loop_after_io_operation() for every I/O.
 * Return values:
 *    Zero means to continue with the next I/O Operation
 *    Non-zero means to exit the inner I/O Loop.
 */
int32_t 
xdd_loopcheck_after_io_operation(ptds_t *p) {

	if (p->time_limit && !(p->target_options & TO_ENDTOEND)) {
		p->my_elapsed_pass_time = p->my_current_op_end_time - p->my_pass_start_time;
		if (p->my_elapsed_pass_time >= (pclk_t)(p->time_limit*TRILLION)) {
			p->my_pass_end_time = p->my_current_op_end_time;
			p->my_pass_ring = 1;
		}
	}
	if ((xgp->estimated_end_time > 0) && 
		(p->my_current_op_end_time >= xgp->estimated_end_time)) {
			p->my_pass_end_time = p->my_current_op_end_time;
			xgp->run_ring = 1;
	}
	if (p->my_pass_ring || xgp->run_ring || p->my_error_break || xgp->deskew_ring) 
		return(1);

	return(0);

} // End of xdd_loopcheck_after_io_operation(p) 

/*----------------------------------------------------------------------------*/
/* xdd_extended_stats() - 
 * Used to update the longest/shorts operation statistics of the -extstats 
 * options was specified
 */
void 
xdd_extended_stats(ptds_t *p) {


	if ((xgp->global_options & GO_EXTENDED_STATS) == 0)
		return;

	// Longest op time
	if (p->my_current_op_elapsed_time > p->my_longest_op_time) {
		p->my_longest_op_time = p->my_current_op_elapsed_time;
		p->my_longest_op_number = p->my_current_op;
		if (p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE) {  // Write Operation
			if (p->my_current_op_elapsed_time > p->my_longest_write_op_time) {
				p->my_longest_write_op_time = p->my_current_op_elapsed_time;
				p->my_longest_write_op_number = p->my_current_op;
			}
		} else { // Read Op
			if (p->my_current_op_elapsed_time > p->my_longest_read_op_time) {
				p->my_longest_read_op_time = p->my_current_op_elapsed_time;
				p->my_longest_read_op_number = p->my_current_op;
			}
		}
			
	}
	// Shortest op time
	if (p->my_current_op_elapsed_time < p->my_shortest_op_time) {
		p->my_shortest_op_time = p->my_current_op_elapsed_time;
		p->my_shortest_op_number = p->my_current_op;
		if (p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE) {  // Write Operation
			if (p->my_current_op_elapsed_time < p->my_shortest_write_op_time) {
				p->my_shortest_write_op_time = p->my_current_op_elapsed_time;
				p->my_shortest_write_op_number = p->my_current_op;
			}
		} else { // Read Op
			if (p->my_current_op_elapsed_time < p->my_shortest_read_op_time) {
				p->my_shortest_read_op_time = p->my_current_op_elapsed_time;
				p->my_shortest_read_op_number = p->my_current_op;
			}
		}
	}
} // End of xdd_extended_stats()
/*----------------------------------------------------------------------------*/
/* xdd_io_loop_after_io_operation() - This subroutine will call all the 
 * subroutines that need to get involved after each I/O Operation.
 * This routine is called within the inner I/O loop after every I/O.
 */
int32_t
xdd_io_loop_after_io_operation(ptds_t *p) {
	int32_t  status;

	// Threshold Checking
	xdd_threshold_after_io_operation(p);

	// I/O Operation Status Checking
	xdd_status_after_io_operation(p);

	// Read-After_Write Processing
	xdd_raw_after_io_operation(p);

	// End-to-End Processing
	xdd_e2e_after_io_operation(p);

	// Extended Statistics 
	xdd_extended_stats(p);

	// End of loop checking
	status = xdd_loopcheck_after_io_operation(p);

	return(status);

} // End of xdd_io_loop_after_io_operation()

 
