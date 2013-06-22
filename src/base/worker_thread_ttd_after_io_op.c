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
#include "xint.h"

//******************************************************************************
// After I/O Operation
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_threshold_after_io_op() - This subroutine will do 
 * threshold checking for the -threshold option to see if the last I/O 
 * operation took more than the specified time (specified as a threshold time)
 * This subroutine is called by the xdd_worker_thread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void
xdd_threshold_after_io_op(worker_data_t *wdp) {
	nclk_t   excess_time;        /* Used by the report_threshold timing */
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	if (tdp->td_report_threshold) {
		if (tdp->td_tgtstp->my_current_op_elapsed_time > tdp->td_report_threshold) {
			excess_time = (tdp->td_tgtstp->my_current_op_elapsed_time - tdp->td_report_threshold)/MILLION;
			fprintf(xgp->output, "%s: Target number %d Worker Thread %d: INFO: Threshold, %lld, exceeded by, %lld, microseconds, IO time was, %lld, usec on block, %lld\n",
				xgp->progname, 
				tdp->td_target_number, 
				wdp->wd_thread_number,
				(long long)tdp->td_report_threshold/MILLION, 
				(long long)excess_time, 
				(long long)tdp->td_tgtstp->my_current_op_elapsed_time/MILLION, 
				(long long)tdp->td_tgtstp->my_current_byte_location);
		}
	}
} // End of xdd_threshold_after_io_op(wdp)

/*----------------------------------------------------------------------------*/
/* xdd_status_after_io_op() - This subroutine will do 
 * status checking after the I/O operation to find out if anything went wrong.
 * This subroutine is called by the xdd_worker_thread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void
xdd_status_after_io_op(worker_data_t *wdp) {
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;

	/* Check for errors in the last operation */
	if ((tdp->td_tgtstp->my_current_error_count > 0) && (xgp->global_options & GO_STOP_ON_ERROR)) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d Worker Thread %d: ERROR: Error on this target caused a Stop_On_Error Event\n",
			xgp->progname,
			tdp->td_target_number, 
			wdp->wd_thread_number);
	}

	if (tdp->td_tgtstp->my_current_error_count >= xgp->max_errors) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d Worker Thread %d: WARNING: Maximum error threshold reached on target - current error count is %lld, maximum count is %lld\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			(long long int)tdp->td_tgtstp->my_current_error_count,
			(long long int)xgp->max_errors);
	}

	if ((tdp->td_tgtstp->my_current_io_status == 0) && (tdp->td_tgtstp->my_current_io_errno == 0)) {
		fprintf(xgp->errout, "%s: status_after_io_op: Target %d Worker Thread %d: WARNING: End-Of-File reached on target named '%s' status=%d, errno=%d\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			tdp->td_target_full_pathname,
			tdp->td_tgtstp->my_current_io_status, 
			tdp->td_tgtstp->my_current_io_errno);
	}

} // End of xdd_status_after_io_op(wdp) 

/*----------------------------------------------------------------------------*/
/* xdd_dio_after_io_op - This subroutine will check several conditions to 
 * make sure that DIO will work for this particular I/O operation. 
 * If any of the DIO conditions are not met then DIO is turned off for this 
 * operation and all subsequent operations by this Worker Thread. 
 *
 * This subroutine is called under the context of a Worker Thread.
 *
 */
void
xdd_dio_after_io_op(worker_data_t *wdp) {
	int		status;
	int             pagesize;
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	// Check to see if DIO is enable for this I/O - return if no DIO required
	if (!(tdp->td_target_options & TO_DIO)) {
		return;
	}
	// If this is an SG device with DIO turned on for whatever reason then just exit
	if (tdp->td_target_options & TO_SGIO) {
		return;
	}
	// Check to see if this I/O location is aligned on the proper boundary
	pagesize = getpagesize();

	// If the current I/O transfer size is an integer multiple of the page size *AND*
	// if the current byte location (aka offset into the file/device) is an integer multiple
	// of the page size then this I/O operation is fine - just return.
	if ((tdp->td_tgtstp->my_current_io_size % pagesize == 0) && 
		(tdp->td_tgtstp->my_current_byte_location % pagesize) == 0) {
	    return;
	}

	// We reached this point because this Worker Thread had been placed in Buffered I/O mode 
	// just before this I/O started. Since we are supposed to be in Direct I/O mode, 
	// we need to close this file descriptor and reopen the file in Direct I/O mode.

	// Close this Worker Thread's file descriptor
	close(wdp->wd_file_desc);
	wdp->wd_file_desc = 0;

	// Reopen the file descriptor for this Worker Thread
#if (SOLARIS || WIN32)
	// For this OS, we need to issue a full open to the target device/file.
	status = xdd_target_open(wdp);
#else // LINUX, AIX, DARWIN
	// In this OS, we do not do an actual open because the Worker Threads share the File Descriptor
	// with the Target Thread. Therefore, we issue a "shallow" open.
	status = xdd_target_shallow_open(wdp);
#endif
	    
	// Check to see if the open worked
	if (status != 0 ) { // error openning target 
	    fprintf(xgp->errout,"%s: xdd_dio_after_io_op: ERROR: Target %d Worker Thread %d: Reopen of target '%s' failed\n",
		    xgp->progname,
		    tdp->td_target_number,
		    wdp->wd_thread_number,
		    tdp->td_target_full_pathname);
	    fflush(xgp->errout);
	    xgp->canceled = 1;
	}
} // End of xdd_dio_after_io_op()

/*----------------------------------------------------------------------------*/
/* xdd_raw_after_io_op() - This subroutine will do 
 * all the processing necessary for a read-after-write operation.
 * This subroutine is called by the xdd_worker_thread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void
xdd_raw_after_io_op(worker_data_t *wdp) {
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	if ((tdp->td_target_options & TO_READAFTERWRITE) && 
	    (tdp->td_target_options & TO_RAW_WRITER)) {
		/* Since I am the writer in a read-after-write operation, and if 
		 * we are using a socket connection to the reader for write-completion 
		 * messages then I need to send the reader a message of what I just 
		 * wrote - starting location and length of write.
		 */
	}
	if ( (tdp->td_tgtstp->my_current_io_status > 0) && (tdp->td_target_options & TO_READAFTERWRITE) ) {
		if (tdp->td_target_options & TO_RAW_READER) { 
			tdp->td_rawp->raw_data_ready -= tdp->td_tgtstp->my_current_io_status;
		} else { /* I must be the writer, send a message to the reader if requested */
			if (tdp->td_rawp->raw_trigger & RAW_MP) {
				tdp->td_rawp->raw_msg.magic = RAW_MAGIC;
				tdp->td_rawp->raw_msg.length = tdp->td_tgtstp->my_current_io_status;
				tdp->td_rawp->raw_msg.location = tdp->td_tgtstp->my_current_byte_location;
				xdd_raw_writer_send_msg(wdp);
			}
		}
	}
} // End of xdd_raw_after_io_op(wdp) 

/*----------------------------------------------------------------------------*/
/* xdd_e2e_after_io_op() - This subroutine will do 
 * all the processing necessary for an end-to-end operation.
 * This subroutine is called by the xdd_worker_thread_ttd_after_io_op() for every I/O.
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void
xdd_e2e_after_io_op(worker_data_t *wdp) {
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	if ( (tdp->td_tgtstp->my_current_io_status > 0) && (tdp->td_target_options & TO_ENDTOEND) ) {
		if (tdp->td_target_options & TO_E2E_SOURCE) {
			// For Serial Ordering, wait for the Previous I/O to complete before the associated Worker Thread releases this Worker Thread. 
			// It is important to note that for Srial Ordering, when we get released by the Previous Worker Thread
			// we are gauranteed that the previous Worker Thread has completed its sendto() operation. That only means that
			// the data is at the Destination machine memory and not necessarily on the Destination storage. 
			// For Loose Ordering we get released just *before* the previous Worker Thread actually performs its sendto() 
			// operation. 
//Therefore, in order to ensure that the previous Worker Thread's I/O operation actually completes [properly], 
// we need to wait again *after* we have completed our sendto() operation for the previous Worker Thread to 
// release us *after* it completes its sendto(). 

			// Send the data to the Destination machine
			wdp->wd_e2ep->e2e_header.magic = PTDS_E2E_MAGIC;
			tdp->td_tgtstp->my_current_state |= CURRENT_STATE_SRC_SEND;

			xdd_e2e_src_send(wdp);

			tdp->td_tgtstp->my_current_state &= ~CURRENT_STATE_SRC_SEND;

			
// If Loose Ordering is in effect then we need to wait for the Previous Worker Thread to complete
// its sendto() operation and release us before we continue. This is done to prevent this Worker Thread and
// subsequent Worker Threads from getting too far ahead of the Previous Worker Thread.
//			if (tdp->td_target_options & TO_LOOSE_ORDERING) {
//				xdd_worker_thread_wait_for_previous_worker_thread(wdp,0);
//			}
	
		} // End of me being the SOURCE in an End-to-End test 
	} // End of processing a End-to-End

} // End of xdd_e2e_after_io_op(wdp) 

/*----------------------------------------------------------------------------*/
/* xdd_extended_stats() - 
 * Used to update the longest/shorts operation statistics of the -extstats 
 * options was specified
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void 
xdd_extended_stats(worker_data_t *wdp) {
	xint_extended_stats_t	*esp;
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	if ((xgp->global_options & GO_EXTENDED_STATS) == 0)
		return;

	if (tdp->td_esp == NULL) {
		fprintf(xgp->errout, "%s: xdd_extended_stats: Target %d Worker Thread %d: INTERNAL ERROR: No pointer to Extended Stats Structure for target named '%s'\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_thread_number,
			tdp->td_target_full_pathname);
		return;
	}
	esp = tdp->td_esp;
	// Longest op time
	if (tdp->td_tgtstp->my_current_op_elapsed_time > esp->my_longest_op_time) {
		esp->my_longest_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
		esp->my_longest_op_number = tdp->td_tgtstp->my_current_op_number;
		if (tdp->td_seekhdr.seeks[tdp->td_tgtstp->my_current_op_number].operation == SO_OP_WRITE) {  		// Write Operation
			if (tdp->td_tgtstp->my_current_op_elapsed_time > esp->my_longest_write_op_time) {
				esp->my_longest_write_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_longest_write_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		} else if (tdp->td_seekhdr.seeks[tdp->td_tgtstp->my_current_op_number].operation == SO_OP_READ) {  // READ Operation
			if (tdp->td_tgtstp->my_current_op_elapsed_time > esp->my_longest_read_op_time) {
				esp->my_longest_read_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_longest_read_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		} else { 																		// NOOP 
			if (tdp->td_tgtstp->my_current_op_elapsed_time > esp->my_longest_noop_op_time) {
				esp->my_longest_noop_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_longest_noop_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		}
	}
	// Shortest op time
	if (tdp->td_tgtstp->my_current_op_elapsed_time < esp->my_shortest_op_time) {
		esp->my_shortest_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
		esp->my_shortest_op_number = tdp->td_tgtstp->my_current_op_number;
		if (tdp->td_seekhdr.seeks[tdp->td_tgtstp->my_current_op_number].operation == SO_OP_WRITE) {  		// Write Operation
			if (tdp->td_tgtstp->my_current_op_elapsed_time < esp->my_shortest_write_op_time) {
				esp->my_shortest_write_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_shortest_write_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		} else if (tdp->td_seekhdr.seeks[tdp->td_tgtstp->my_current_op_number].operation == SO_OP_READ) {  // READ Operation
			if (tdp->td_tgtstp->my_current_op_elapsed_time < esp->my_shortest_read_op_time) {
				esp->my_shortest_read_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_shortest_read_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		} else { 																		// NOOP 
			if (tdp->td_tgtstp->my_current_op_elapsed_time < esp->my_shortest_noop_op_time) {
				esp->my_shortest_noop_op_time = tdp->td_tgtstp->my_current_op_elapsed_time;
				esp->my_shortest_noop_op_number = tdp->td_tgtstp->my_current_op_number;
			}
		}
	}
} // End of xdd_extended_stats()
/*----------------------------------------------------------------------------*/
/* xdd_worker_thread_ttd_after_io_op() - This subroutine will call all the 
 * subroutines that need to get involved after each I/O Operation.
 * This subroutine is called by worker_thread_io() after it completes its assigned
 * I/O task. 
 * 
 * This subroutine is called within the context of a Worker Thread.
 *
 */
void
xdd_worker_thread_ttd_after_io_op(worker_data_t *wdp) {
	target_data_t	*tdp;


	tdp = wdp->wd_tdp;
	// I/O Operation Status Checking
	xdd_status_after_io_op(wdp);

	if (tdp->td_tgtstp->abort)
		return;

	// Threshold Checking
	xdd_threshold_after_io_op(wdp);

	// DirectIO Handling
	xdd_dio_after_io_op(wdp);

	// Read-After_Write Processing
	xdd_raw_after_io_op(wdp);

	// End-to-End Processing
	xdd_e2e_after_io_op(wdp);

	// Extended Statistics 
	xdd_extended_stats(wdp);

} // End of xdd_worker_thread_ttd_after_io_op()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
