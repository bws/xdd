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
/*
 * This file contains the subroutines that support the Target threads.
 */
#include "xdd.h"

#ifdef LINUX
/*----------------------------------------------------------------------------*/
/* xdd_io_for_os<linux>() - This subroutine is only used on Linux systems
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_os(ptds_t *qp) {
	ptds_t		*p;			// Pointer to the parent Target's PTDS


	p = qp->target_ptds;
	/* In Linux the -D_FILE_OFFSET_BITS=64 makes the off_t type be a 64-bit integer */
	if (!(qp->target_options & TO_SGIO))  // If this is NOT and SGIO operation... SGIO will do its own seek
		lseek(qp->fd, (off_t)qp->my_current_byte_location, SEEK_SET);
	/* Do the deed .... */
	qp->my_current_op_end_time = 0;
	if (qp->my_current_op_type == OP_TYPE_WRITE) {  // Write Operation
		qp->my_current_op_str = "WRITE";
		// Call xdd_datapattern_fill() to fill the buffer with any required patterns
		xdd_datapattern_fill(qp);

		// Record the starting time for this write op
		pclk_now(&qp->my_current_op_start_time);

		// Issue the actual operation
		if ((qp->target_options & TO_SGIO)) 
			 qp->my_current_io_status = xdd_sg_io(qp,'w'); // Issue the SGIO operation 
		else qp->my_current_io_status = write(qp->fd, qp->rwbuf, qp->my_current_io_size);// Issue a normal write operation

		// Record the ending time for this write op and update counters
       	pclk_now(&qp->my_current_op_end_time);
	} else if (qp->my_current_op_type == OP_TYPE_READ) {  // READ Operation
		qp->my_current_op_str = "READ";
		// Record the starting time for this read op
		pclk_now(&qp->my_current_op_start_time);

		// Issue the actual operation
		if ((qp->target_options & TO_SGIO)) 
			 qp->my_current_io_status = xdd_sg_io(qp,'r'); // Issue the SGIO operation 
		else qp->my_current_io_status = read(qp->fd, qp->rwbuf, qp->my_current_io_size);// Issue a normal read() operation
	
		// Record the ending time for this read operation and update counters
		pclk_now(&qp->my_current_op_end_time);
	} else {  // Must be a NOOP
		// The NOOP is used to test the overhead usage of XDD when no actual I/O is done
		qp->my_current_op_str = "NOOP";
		// Record the starting time for this no op
		pclk_now(&qp->my_current_op_start_time);

		// Make it look like a successful I/O
		qp->my_current_io_status = qp->my_current_io_size;
		errno = 0;

		// Record the ending time for this read operation and update counters
		pclk_now(&qp->my_current_op_end_time);

	} // End of NOOP operation
		
	// Time stamp if requested
	if (p->ts_options & (TS_ON | TS_TRIGGERED)) {
		p->ttp->tte[qp->ts_current_entry].qthread_number = qp->my_qthread_number;
		p->ttp->tte[qp->ts_current_entry].op_type = qp->my_current_op_type;
		p->ttp->tte[qp->ts_current_entry].op_number = qp->target_op_number;
		p->ttp->tte[qp->ts_current_entry].byte_location = qp->my_current_byte_location;
		p->ttp->tte[qp->ts_current_entry].start = qp->my_current_op_start_time;
		p->ttp->tte[qp->ts_current_entry].end = qp->my_current_op_end_time;
	}

} // End of xdd_io_for_linux()
#endif 

#ifdef AIX
/*----------------------------------------------------------------------------*/
/* xdd_io_for_os() - This subroutine is only used on AIX systems
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_os(ptds_t *qp) {

	/* Perform the seek */
	lseek64(p->fd, (off_t)p->my_current_byte_location, SEEK_SET);

	/* Do the deed .... */
	if (qp->my_current_op_type == OP_TYPE_WRITE) {  // Write Operation
		qp->my_current_op_str = "WRITE";
		// Call xdd_datapattern_fill() to fill the buffer with any required patterns
		xdd_datapattern_fill(p);
		// Record the starting time for this write op
		pclk_now(&p->my_current_op_start_time);

		// Issue the actual operation
		p->my_io_status = write(p->fd, p->rwbuf, p->my_current_io_size);// Issue a normal write operation

		// Record the ending time for this write op and update counters
		pclk_now(&p->my_current_op_end_time);
	} else if (qp->my_current_op_type == OP_TYPE_READ) {  // READ Operation
		// Record the starting time for this read op
		pclk_now(&p->my_current_op_start_time);

		// Issue the actual operation
		p->my_io_status = read(p->fd, p->rwbuf, p->my_current_io_size);// Issue a normal read() operation
		// Record the ending time for this read operation and update counters
		pclk_now(&p->my_current_op_end_time);
	} else {  // Must be a NOOP
		// The NOOP is used to test the overhead usage of XDD when no actual I/O is done
		qp->my_current_op_str = "NOOP";
		// Record the starting time for this no op
		pclk_now(&qp->my_current_op_start_time);

		// Record the ending time for this read operation and update counters
		pclk_now(&qp->my_current_op_end_time);
	} // end of WRITE/READ operation

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
	pclk_t			start_time;			// Used for calculating elapsed times of ops
	pclk_t			end_time;			// Used for calculating elapsed times of ops
	uint64_t 		current_position;	/* seek location read from device */
	uint32_t 		uj;                	/* Random unsigned variable */
	LPVOID 			lpMsgBuf;
	uint32_t 		bytesxferred; 		/* Bytes transferred */
	unsigned long 	plow;
	unsigned long 	phi;


	plow = (unsigned long)p->my_current_byte_location;
	phi = (unsigned long)(p->my_current_byte_location >> 32);

	/* Position to the correct place on the storage device/file */
	SetFilePointer(p->fd, plow, &phi, FILE_BEGIN);
	/* Check to see if there is supposed to be a sequenced data pattern in the data buffer */
	/* For write operations, this means that we should update the data pattern in the buffer to
	 * the expected value which is the relative byte offset of each block in this request.
	 * For read operations, we need to check the block offset with what we think it should be.
	 * For read operations, it is assumed that the data read was previously written by xdd
	 * and is in the expected format.
	 */
	pclk_now(&p->my_current_start_time);
	if (p->target_op_number == 0) /* record our starting time */
		p->my_start_time = p->my_current_start_time;
	if (p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE) {
		if (p->target_options & TO_SEQUENCED_PATTERN) {
			posp = (uint64_t *)p->rwbuf;
			for (uj=0; uj<(p->my_current_io_size/sizeof(p->my_current_byte_location)); uj++) {
				*posp = p->my_current_byte_location + (uj * sizeof(p->my_current_byte_location));
				*posp |= p->data_pattern_prefix_binary;
				if (p->target_options & TO_INVERSE_PATTERN)
					*posp ^= 0xffffffffffffffffLL; // 1's compliment of the pattern
				posp++;
			}
		}
		/* Actually write the data to the storage device/file */
			p->my_io_status = WriteFile(p->fd, p->rwbuf, p->my_current_io_size, &bytesxferred, NULL);
		} else { /* Simply do the normal read operation */
			p->my_io_status = ReadFile(p->fd, p->rwbuf, p->my_current_io_size, &bytesxferred, NULL);
			if (p->target_options & (TO_VERIFY_CONTENTS | TO_VERIFY_LOCATION)) {
				posp = (uint64_t *)p->rwbuf;
				current_position = *posp; 
			}
		}
        pclk_now(&p->my_current_end_time);
		/* Take a time stamp if necessary */
		if ((p->ts_options & TS_ON) && (p->ts_options & TS_TRIGGERED)) {  
			p->ttp->tte[p->ttp->tte_indx++].end = p->my_current_end_time;
			if (p->ttp->tte_indx == p->ttp->tt_size) { /* Check to see if we are at the end of the buffer */
				if (p->ts_options & TS_ONESHOT) 
					p->ts_options &= ~TS_ON; /* Turn off Time Stamping now that we are at the end of the time stamp buffer */
				else if (p->ts_options & TS_WRAP) 
					p->ttp->tte_indx = 0; /* Wrap to the beginning of the time stamp buffer */
			}
		}
		/* Let's check the status of the last operation to see if things went well.
		 * If not, tell somebody who cares - like the poor soul running this program.
		 */
		if ((p->my_io_status == FALSE) || (bytesxferred != (unsigned long)p->my_current_io_size)) { 
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
				xgp->progname,(p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE)?"write":"read",p->target);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			p->my_current_error_count++;
		}
		p->my_io_status = bytesxferred;
} // End of xdd_io_for_os()
#endif
