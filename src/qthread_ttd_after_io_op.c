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
 *       Brad Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
   xdd - a basic i/o performance test
*/
#include "xdd.h"

//******************************************************************************
// After I/O Operation
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_threshold_after_io_op() - This subroutine will do 
 * threshold checking for the -threshold option to see if the last I/O 
 * operation took more than the specified time (specified as a threshold time)
 * This subroutine is called by the xdd_qthread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void
xdd_threshold_after_io_op(ptds_t *qp) {
	pclk_t   excess_time;        /* Used by the report_threshold timing */


	if (qp->report_threshold) {
		if (qp->my_current_op_elapsed_time > qp->report_threshold) {
			excess_time = (qp->my_current_op_elapsed_time - qp->report_threshold)/MILLION;
			fprintf(xgp->output, "%s: Target number %d QThread %d: INFO: Threshold, %lld, exceeded by, %lld, microseconds, IO time was, %lld, usec on block, %lld\n",
				xgp->progname, 
				qp->my_target_number, 
				qp->my_qthread_number,
				(long long)qp->report_threshold/MILLION, 
				(long long)excess_time, 
				(long long)qp->my_current_op_elapsed_time/MILLION, 
				(long long)qp->my_current_byte_location);
		}
	}
} // End of xdd_threshold_after_io_op(qp)

/*----------------------------------------------------------------------------*/
/* xdd_status_after_io_op() - This subroutine will do 
 * status checking after the I/O operation to find out if anything went wrong.
 * This subroutine is called by the xdd_qthread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void
xdd_status_after_io_op(ptds_t *qp) {

	/* Check for errors in the last operation */
	if ((qp->my_current_error_count > 0) && (xgp->global_options & GO_STOP_ON_ERROR)) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d QThread %d: ERROR: Error on this target caused a Stop_On_Error Event - exiting all threads and targets\n",
			xgp->progname,
			qp->my_target_number, 
			qp->my_qthread_number);
		xgp->run_ring = 1;
	}

	if (qp->my_current_error_count >= xgp->max_errors) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d QThread %d: WARNING: Maximum error threshold reached on target - current error count is %lld, maximum count is %lld\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			(long long int)qp->my_current_error_count,
			(long long int)xgp->max_errors);
		qp->my_error_break = 1;
	}

	if ((qp->my_current_io_status == 0) && (qp->my_current_io_errno == 0)) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d QThread %d: WARNING: End-Of-File reached on target named '%s' status=%d, errno=%d\n",
			xgp->progname,
			qp->my_target_number,
			qp->my_qthread_number,
			qp->target_full_pathname,
			qp->my_current_io_status, 
			qp->my_current_io_errno);
		qp->my_error_break = 1; /* fake an exit */
	}

} // End of xdd_status_after_io_op(qp) 

/*----------------------------------------------------------------------------*/
/* xdd_raw_after_io_op() - This subroutine will do 
 * all the processing necessary for a read-after-write operation.
 * This subroutine is called by the xdd_qthread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void
xdd_raw_after_io_op(ptds_t *qp) {

	if ((qp->target_options & TO_READAFTERWRITE) && 
	    (qp->target_options & TO_RAW_WRITER)) {
		/* Since I am the writer in a read-after-write operation, and if 
		 * we are using a socket connection to the reader for write-completion 
		 * messages then I need to send the reader a message of what I just 
		 * wrote - starting location and length of write.
		 */
	}
	if ( (qp->my_current_io_status > 0) && (qp->target_options & TO_READAFTERWRITE) ) {
		if (qp->target_options & TO_RAW_READER) { 
			qp->raw_data_ready -= qp->my_current_io_status;
		} else { /* I must be the writer, send a message to the reader if requested */
			if (qp->raw_trigger & PTDS_RAW_MP) {
				qp->raw_msg.magic = PTDS_RAW_MAGIC;
				qp->raw_msg.length = qp->my_current_io_status;
				qp->raw_msg.location = qp->my_current_byte_location;
				xdd_raw_writer_send_msg(qp);
			}
		}
	}
} // End of xdd_raw_after_io_op(qp) 

/*----------------------------------------------------------------------------*/
/* xdd_e2e_after_io_op() - This subroutine will do 
 * all the processing necessary for an end-to-end operation.
 * This subroutine is called by the xdd_qthread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void
xdd_e2e_after_io_op(ptds_t *qp) {
	ptds_t	*p;			// Pointer to the Target PTDS for this QThread
	pclk_t	beg_time_tmp;
	pclk_t	end_time_tmp;
	pclk_t	now;


	p = qp->target_ptds;
	if ( (qp->my_current_io_status > 0) && (qp->target_options & TO_ENDTOEND) ) {
		if (qp->target_options & TO_E2E_SOURCE) {
			// SOURCE - THINGS TO DO - 
			qp->e2e_header.magic = PTDS_E2E_MAGIC;
			pclk_now(&now);
			if (qp->time_limit) { /* times-up, signal DESTINATION to quit */
				if ((now - qp->my_pass_start_time)  >= (pclk_t)(qp->time_limit*TRILLION)) {
					qp->e2e_header.magic = PTDS_E2E_MAGIQ;
					qp->my_pass_ring = 1;
				}
			}
			qp->my_current_state |= CURRENT_STATE_SRC_SEND;

			pclk_now(&beg_time_tmp);
			xdd_e2e_src_send(qp);
			pclk_now(&end_time_tmp);
			qp->e2e_sr_time = (end_time_tmp - beg_time_tmp); // Time spent sending to the destination machine

			qp->my_current_state &= ~CURRENT_STATE_SRC_SEND;
		} // End of me being the SOURCE in an End-to-End test 
	} // End of processing a End-to-End

} // End of xdd_e2e_after_io_op(qp) 

/*----------------------------------------------------------------------------*/
/* xdd_extended_stats() - 
 * Used to update the longest/shorts operation statistics of the -extstats 
 * options was specified
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void 
xdd_extended_stats(ptds_t *qp) {


	if ((xgp->global_options & GO_EXTENDED_STATS) == 0)
		return;

	// Longest op time
	if (qp->my_current_op_elapsed_time > qp->my_longest_op_time) {
		qp->my_longest_op_time = qp->my_current_op_elapsed_time;
		qp->my_longest_op_number = qp->my_current_op_number;
		if (qp->seekhdr.seeks[qp->my_current_op_number].operation == SO_OP_WRITE) {  		// Write Operation
			if (qp->my_current_op_elapsed_time > qp->my_longest_write_op_time) {
				qp->my_longest_write_op_time = qp->my_current_op_elapsed_time;
				qp->my_longest_write_op_number = qp->my_current_op_number;
			}
		} else if (qp->seekhdr.seeks[qp->my_current_op_number].operation == SO_OP_READ) {  // READ Operation
			if (qp->my_current_op_elapsed_time > qp->my_longest_read_op_time) {
				qp->my_longest_read_op_time = qp->my_current_op_elapsed_time;
				qp->my_longest_read_op_number = qp->my_current_op_number;
			}
		} else { 																		// NOOP 
			if (qp->my_current_op_elapsed_time > qp->my_longest_noop_op_time) {
				qp->my_longest_noop_op_time = qp->my_current_op_elapsed_time;
				qp->my_longest_noop_op_number = qp->my_current_op_number;
			}
		}
	}
	// Shortest op time
	if (qp->my_current_op_elapsed_time < qp->my_shortest_op_time) {
		qp->my_shortest_op_time = qp->my_current_op_elapsed_time;
		qp->my_shortest_op_number = qp->my_current_op_number;
		if (qp->seekhdr.seeks[qp->my_current_op_number].operation == SO_OP_WRITE) {  		// Write Operation
			if (qp->my_current_op_elapsed_time < qp->my_shortest_write_op_time) {
				qp->my_shortest_write_op_time = qp->my_current_op_elapsed_time;
				qp->my_shortest_write_op_number = qp->my_current_op_number;
			}
		} else if (qp->seekhdr.seeks[qp->my_current_op_number].operation == SO_OP_READ) {  // READ Operation
			if (qp->my_current_op_elapsed_time < qp->my_shortest_read_op_time) {
				qp->my_shortest_read_op_time = qp->my_current_op_elapsed_time;
				qp->my_shortest_read_op_number = qp->my_current_op_number;
			}
		} else { 																		// NOOP 
			if (qp->my_current_op_elapsed_time < qp->my_shortest_noop_op_time) {
				qp->my_shortest_noop_op_time = qp->my_current_op_elapsed_time;
				qp->my_shortest_noop_op_number = qp->my_current_op_number;
			}
		}
	}
} // End of xdd_extended_stats()
/*----------------------------------------------------------------------------*/
/* xdd_qthread_ttd_after_io_op() - This subroutine will call all the 
 * subroutines that need to get involved after each I/O Operation.
 * This subroutine is called by qthread_io() after it completes its assigned
 * I/O task. 
 * 
 * This subroutine is called within the context of a QThread.
 *
 */
void
xdd_qthread_ttd_after_io_op(ptds_t *qp) {


	// Threshold Checking
	xdd_threshold_after_io_op(qp);

	// I/O Operation Status Checking
	xdd_status_after_io_op(qp);

	// Read-After_Write Processing
	xdd_raw_after_io_op(qp);

	// End-to-End Processing
	xdd_e2e_after_io_op(qp);

	// Extended Statistics 
	xdd_extended_stats(qp);

} // End of xdd_qthread_ttd_after_io_op()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
