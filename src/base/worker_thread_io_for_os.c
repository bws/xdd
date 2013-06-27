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
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"
#include "xint_wd.h"

#if defined LINUX
/*----------------------------------------------------------------------------*/
/* xdd_io_for_os<linux>() - This subroutine is only used on Linux systems
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_os(worker_data_t *wdp) {
	target_data_t		*tdp;		// Pointer to the parent Target Data Structure
	xint_timestamp_t	*tsp;		// Time Stamp pointer
	xdd_tthdr_t			*ttp;		// Time Stamp Table Header pointer


	tdp = wdp->wd_tdp;		// Get pointer to the Target Data 
	tsp = tdp->td_tsp;		// Get pointer to the Time Stamp Variables 
	ttp = tdp->td_ttp;		// Get pointer to the Time Stamp Table

fprintf(xgp->errout,"\n%s: xdd_io_for_os: Target %d Worker Thread %d: ENTER: tdp=%p, wdp=%p, target_options=0x%016llx, wd_task.task_op_type=%x\n", xgp->progname, tdp->td_target_number, wdp->wd_thread_number,tdp,wdp, (long long int)tdp->td_target_options,wdp->wd_task.task_op_type);
	// Record the starting time for this write op
	nclk_now(&wdp->wd_counters.tc_current_op_start_time);
	// Time stamp if requested
	if ((tsp) && (tsp->ts_options & (TS_ON | TS_TRIGGERED))) {
		ttp->tte[wdp->wd_ts_entry].disk_start = wdp->wd_counters.tc_current_op_start_time;
		ttp->tte[wdp->wd_ts_entry].disk_processor_start = xdd_get_processor();
	}

	/* Do the deed .... */
	wdp->wd_counters.tc_current_op_end_time = 0;
	if (wdp->wd_task.task_op_type == TASK_OP_TYPE_WRITE) {  // Write Operation
		wdp->wd_task.task_op_string = "WRITE";
		// Call xdd_datapattern_fill() to fill the buffer with any required patterns
		xdd_datapattern_fill(wdp);

		if (tdp->td_target_options & TO_NULL_TARGET) { // If this is a NULL target then we fake the I/O
			wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		} else { // Issue the actual operation
			if ((tdp->td_target_options & TO_SGIO)) 
			 	wdp->wd_task.task_io_status = xdd_sg_io(wdp,'w'); // Issue the SGIO operation 
			else if (!(tdp->td_target_options & TO_NULL_TARGET)) {
fprintf(stdout,"%lld:io_for_os:tdp:%p:wdp:%p: WRITE target_number %d, offset %lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp,tdp->td_target_number,(long long int)wdp->wd_task.task_byte_offset);
                wdp->wd_task.task_io_status = pwrite(wdp->wd_file_desc,
                                                          wdp->wd_rwbuf,
                                                          wdp->wd_task.task_xfer_size,
                                                   (off_t)wdp->wd_task.task_byte_offset); // Issue a positioned write operation
			} else { 
				wdp->wd_task.task_io_status = write(wdp->wd_file_desc, wdp->wd_rwbuf, wdp->wd_task.task_xfer_size); // Issue a normal write() op
			}

		}
	} else if (wdp->wd_task.task_op_type == TASK_OP_TYPE_READ) {  // READ Operation
		wdp->wd_task.task_op_string = "READ";

		if (tdp->td_target_options & TO_NULL_TARGET) { // If this is a NULL target then we fake the I/O
			wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		} else { // Issue the actual operation
			if ((tdp->td_target_options & TO_SGIO)) 
			 	wdp->wd_task.task_io_status = xdd_sg_io(wdp,'r'); // Issue the SGIO operation 
			else if (!(tdp->td_target_options & TO_NULL_TARGET)) {
fprintf(stdout,"%lld:io_for_os:tdp:%p:wdp:%p: READ target_number %d, offset %lld\n",(long long int)pclk_now()-xgp->debug_base_time,tdp,wdp,tdp->td_target_number,(long long int)wdp->wd_task.task_byte_offset);
                            wdp->wd_task.task_io_status = pread(wdp->wd_file_desc,
                                                               wdp->wd_rwbuf,
                                                               wdp->wd_task.task_xfer_size,
                                                               (off_t)wdp->wd_task.task_byte_offset);// Issue a positioned read operation
			} else wdp->wd_task.task_io_status = read(wdp->wd_file_desc,
                                                              wdp->wd_rwbuf,
                                                              wdp->wd_task.task_xfer_size);// Issue a normal read() operation
		}
	
// FIXME _ NEED TO FIX THIS 
//		if (p->td_target_options & (TO_VERIFY_CONTENTS | TO_VERIFY_LOCATION)) {
//			wdp->dpp->data_pattern_compare_errors += xdd_verify(wdp, wdp->wdrget_op_number);
//		}
	
	} else {  // Must be a NOOP
		// The NOOP is used to test the overhead usage of XDD when no actual I/O is done
		wdp->wd_task.task_op_string = "NOOP";

		// Make it look like a successful I/O
		wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		errno = 0;
	} // End of NOOP operation

	// Record the ending time for this op 
	nclk_now(&wdp->wd_counters.tc_current_op_end_time);
	// Time stamp if requested
	if ((tsp) && (tsp->ts_options & (TS_ON | TS_TRIGGERED))) {
		ttp->tte[tsp->ts_current_entry].disk_end = wdp->wd_counters.tc_current_op_end_time;
		ttp->tte[tsp->ts_current_entry].disk_xfer_size = wdp->wd_task.task_io_status;
		ttp->tte[tsp->ts_current_entry].disk_processor_end = xdd_get_processor();
	}
fprintf(xgp->errout,"\n%s: xdd_io_for_os: Target %d Worker Thread %d: EXIT: tdp=%p, wdp=%p, wd_task.task_io_status=%d\n", xgp->progname, tdp->td_target_number, wdp->wd_thread_number,tdp,wdp,wdp->wd_task.task_io_status);

} // End of xdd_io_for_linux()
#endif 

#if defined(AIX) || defined(DARWIN)
/*----------------------------------------------------------------------------*/
/* xdd_io_for_os() - This subroutine is only used on AIX systems
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_os(worker_data_t *wdp) {
	target_data_t		*tdp;		// Pointer to the parent Target Data Structure
	xint_timestamp_t	*tsp;		// Time Stamp pointer
	xdd_tthdr_t			*ttp;		// Time Stamp Table Header pointer


	tdp = wdp->wd_tdp;		// Get pointer to the Target Data 
	tsp = tdp->td_tsp;		// Get pointer to the Time Stamp Variables 
	ttp = tdp->td_ttp;		// Get pointer to the Time Stamp Table

	// Record the starting time for this write op
	nclk_now(&wdp->wd_counters.tc_current_op_start_time);
	// Time stamp if requested
	if ((tsp) && (tsp->ts_options & (TS_ON | TS_TRIGGERED))) {
		ttp->tte[tsp->ts_current_entry].disk_start = wdp->wd_counters.tc_current_op_start_time;
		ttp->tte[tsp->ts_current_entry].disk_processor_start = xdd_get_processor();
	}

	/* Do the deed .... */
	wdp->wd_counters.tc_current_op_end_time = 0;
	if (wdp->wd_task.task_op_type == TASK_OP_TYPE_WRITE) {  // Write Operation
		wdp->wd_task.task_op_string = "WRITE";
		// Call xdd_datapattern_fill() to fill the buffer with any required patterns
		xdd_datapattern_fill(wdp);

		if (tdp->td_target_options & TO_NULL_TARGET) { // If this is a NULL target then we fake the I/O
			wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		} else { // Issue the actual operation
			if (!(tdp->td_target_options & TO_NULL_TARGET))
                            wdp->wd_task.task_io_status = pwrite(wdp->wd_file_desc,
                                                               wdp->wd_rwbuf,
                                                               wdp->wd_task.task_xfer_size,
                                                               (off_t)wdp->wd_task.task_byte_offset); // Issue a positioned write operation
                        else wdp->wd_task.task_io_status = write(wdp->wd_file_desc, wdp->wd_rwbuf, wdp->wd_task.task_xfer_size); // Issue a normal write() op

		}
	} else if (wdp->wd_task.task_op_type == TASK_OP_TYPE_READ) {  // READ Operation
		wdp->wd_task.task_op_string = "READ";

		if (tdp->td_target_options & TO_NULL_TARGET) { // If this is a NULL target then we fake the I/O
			wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		} else { // Issue the actual operation
			if (!(tdp->td_target_options & TO_NULL_TARGET))
                            wdp->wd_task.task_io_status = pread(wdp->wd_file_desc,
							     wdp->wd_rwbuf,
							     wdp->wd_task.task_xfer_size,
							     (off_t)wdp->wd_task.task_byte_offset);// Issue a positioned read operation
			else wdp->wd_task.task_io_status = read(wdp->wd_file_desc,
                                                              wdp->wd_rwbuf,
                                                              wdp->wd_task.task_xfer_size);// Issue a normal read() operation
		}
/* FIXME	
		if (tdp->td_target_options & (TO_VERIFY_CONTENTS | TO_VERIFY_LOCATION)) {
			wdp->wd_tdp->data_pattern_compare_errors += xdd_verify(wdp, wdp->wdrget_op_number);
		}
*/	
	} else {  // Must be a NOOP
		// The NOOP is used to test the overhead usage of XDD when no actual I/O is done
		wdp->wd_task.task_op_string = "NOOP";

		// Make it look like a successful I/O
		wdp->wd_task.task_io_status = wdp->wd_task.task_xfer_size;
		errno = 0;
	} // End of NOOP operation

	// Record the ending time for this op 
	nclk_now(&wdp->wd_counters.tc_current_op_end_time);
	// Time stamp if requested
	if ((tsp) && (tsp->ts_options & (TS_ON | TS_TRIGGERED))) {
		ttp->tte[tsp->ts_current_entry].disk_end = wdp->wd_counters.tc_current_op_end_time;
		ttp->tte[tsp->ts_current_entry].disk_xfer_size = wdp->wd_task.task_io_status;
		ttp->tte[tsp->ts_current_entry].disk_processor_end = xdd_get_processor();
	}		
} // End of xdd_io_for_os()
#endif

#ifdef WIN32
/*----------------------------------------------------------------------------*/
/* xdd_io_for_os() - This subroutine is only used on Windows
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_os(ptds_t *p) {
	nclk_t			start_time;			// Used for calculating elapsed times of ops
	nclk_t			end_time;			// Used for calculating elapsed times of ops
	uint64_t 		current_position;	/* seek location read from device */
	uint32_t 		uj;                	/* Random unsigned variable */
	LPVOID 			lpMsgBuf;
	uint32_t 		bytesxferred; 		/* Bytes transferred */
	unsigned long 	plow;
	unsigned long 	phi;


	plow = (unsigned long)p->wd_task.task_byte_offset;
	phi = (unsigned long)(p->wd_task.task_byte_offset >> 32);

	/* Position to the correct place on the storage device/file */
	SetFilePointer(p->file_desc, plow, &phi, FILE_BEGIN);
	/* Check to see if there is supposed to be a sequenced data pattern in the data buffer */
	/* For write operations, this means that we should update the data pattern in the buffer to
	 * the expected value which is the relative byte offset of each block in this request.
	 * For read operations, we need to check the block offset with what we think it should be.
	 * For read operations, it is assumed that the data read was previously written by xdd
	 * and is in the expected format.
	 */
	nclk_now(&p->tgtstp->wd_current_start_time);
	if (p->tgtstp->target_op_number == 0) /* record our starting time */
		p->my_start_time = p->tgtstp->wd_current_start_time;
	if (p->seekhdr.seeks[p->tgtstp->wd_current_op].operation == SO_OP_WRITE) {
		if (p->dpp) {
			if (p->dpp->data_pattern_options & DP_SEQUENCED_PATTERN) {
				posp = (uint64_t *)p->wd_rwbuf;
				for (uj=0; uj<(p->tgtstp->wd_task.task_xfer_size/sizeof(p->tgtstp->wd_task.task_byte_offset)); uj++) {
					*posp = p->tgtstp->wd_task.task_byte_offset + (uj * sizeof(p->tgtstp->wd_task.task_byte_offset));
					*posp |= p->dpp->data_pattern_prefix_binary;
					if (p->data_pattern_options & DP_INVERSE_PATTERN)
						*posp ^= 0xffffffffffffffffLL; // 1's compliment of the pattern
					posp++;
				}
			}
		}
		/* Actually write the data to the storage device/file */
			p->my_io_status = WriteFile(p->file_desc, p->wd_rwbuf, p->tgtstp->wd_task.task_xfer_size, &bytesxferred, NULL);
		} else { /* Simply do the normal read operation */
			p->my_io_status = ReadFile(p->file_desc, p->wd_rwbuf, p->tgtstp->wd_task.task_xfer_size, &bytesxferred, NULL);
			if (p->td_target_options & (TO_VERIFY_CONTENTS | TO_VERIFY_LOCATION)) {
				posp = (uint64_t *)p->wd_rwbuf;
				current_position = *posp; 
			}
		}
        nclk_now(&p->tgtstp->wd_current_end_time);
		/* Take a time stamp if necessary */
		if ((p->tsp->ts_options & TS_ON) && (p->tsp->ts_options & TS_TRIGGERED)) {  
			ttp->tte[ttp->tte_indx++].disk_end = p->tgtstp->wd_current_end_time;
			if (ttp->tte_indx == ttp->tt_size) { /* Check to see if we are at the end of the buffer */
				if (p->tsp->ts_options & TS_ONESHOT) 
					p->tsp->ts_options &= ~TS_ON; /* Turn off Time Stamping now that we are at the end of the time stamp buffer */
				else if (p->tsp->ts_options & TS_WRAP) 
					ttp->tte_indx = 0; /* Wrap to the beginning of the time stamp buffer */
			}
		}
		/* Let's check the status of the last operation to see if things went well.
		 * If not, tell somebody who cares - like the poor soul running this program.
		 */
		if ((p->my_io_status == FALSE) || (bytesxferred != (unsigned long)p->tgtstp->wd_task.task_xfer_size)) { 
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(xgp->errout,"%s: I/O error: could not %s target %s\n",
				xgp->progname,(p->seekhdr.seeks[p->tgtstp->wd_current_op].operation == SO_OP_WRITE)?"write":"read",p->target);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			p->tgtstp->wd_current_error_count++;
		}
		p->my_io_status = bytesxferred;
} // End of xdd_io_for_os()
#endif

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
