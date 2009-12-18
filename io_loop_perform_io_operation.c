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

void xdd_data_pattern_fill(ptds_t *p); // Used in this subroutine only
void xdd_io_for_windows(ptds_t *p); // Used in this subroutine only
void xdd_io_for_linux(ptds_t *p); // Used in this subroutine only

//******************************************************************************
// I/O Operation
//******************************************************************************

/*----------------------------------------------------------------------------*/
/* xdd_io_loop_perform_io_operation() 
 * This routine is called within the inner I/O loop for every I/O.
 */
int32_t
xdd_io_loop_perform_io_operation(ptds_t *p) {
	pclk_t			start_time;			// Used for calculating elapsed times of ops
	pclk_t			end_time;			// Used for calculating elapsed times of ops


	p->my_current_state = CURRENT_STATE_IO;	// Starting an I/O Operation now...

	if ((p->bytes_to_xfer_per_pass - p->my_current_bytes_xfered) < p->iosize)
		p->actual_iosize = (p->bytes_to_xfer_per_pass - p->my_current_bytes_xfered);
	else p->actual_iosize = p->iosize;
		  
// If a data pattern PREFIX was specified then set the local pattern prefix value.
//	if (p->target_options & TO_PATTERN_PREFIX){
//		data_pattern_prefix_value = p->data_pattern_prefix_binary;
//	} else {
//		data_pattern_prefix_value = 0;
//	}

	// Call the OS-appropriate IO routine which will set p->my_io_status properly
#ifdef WIN32
		xdd_io_for_windows(p);
#elif LINUX
		xdd_io_for_linux(p);
#endif

	p->my_current_state = CURRENT_STATE_BTW;	// Between I/O Operations now...

	//  Return the number of bytes transferred on this I/O Operation
	return(p->my_io_status);

} // End of xdd_io_loop_perform_io_operation()

/*----------------------------------------------------------------------------*/
/* xdd_io_for_linux() - This subroutine is only used on Linux systems
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_linux(ptds_t *p) {
#ifdef LINUX
	pclk_t			start_time;			// Used for calculating elapsed times of ops
	pclk_t			end_time;			// Used for calculating elapsed times of ops


	/* In Linux the -D_FILE_OFFSET_BITS=64 makes the off_t type be a 64-bit integer */
	if (!(p->target_options & TO_SGIO))  // If this is NOT and SGIO operation... SGIO will do its own seek
		lseek(p->fd, (off_t)p->my_current_byte_location, SEEK_SET);
	/* Do the deed .... */
	if (p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE) {  // Write Operation
		// Call xdd_data_pattern_fill() to fill the buffer with any required patterns
		xdd_data_pattern_fill(p);

		// Record the starting time for this write op
		pclk_now(&p->my_current_op_start_time);
		if (p->my_current_op == 0) // Record the start time for the first op
			p->my_first_op_start_time = p->my_current_op_start_time;

		// Issue the actual operation
		if ((p->target_options & TO_SGIO)) 
			 p->my_io_status = xdd_sg_io(p,'w'); // Issue the SGIO operation 
		else p->my_io_status = write(p->fd, p->rwbuf, p->actual_iosize);// Issue a normal write operation

		// Record the ending time for this write op and update counters
       	pclk_now(&p->my_current_op_end_time);
		p->my_current_op_elapsed_time = (p->my_current_op_end_time - p->my_current_op_start_time);
		p->my_accumulated_op_time += p->my_current_op_elapsed_time;
		p->my_accumulated_write_op_time += p->my_current_op_elapsed_time;
		p->my_current_write_op_count++;
		p->my_io_errno = errno;
		if (p->my_io_status == p->actual_iosize) {
			p->my_current_bytes_written += p->actual_iosize;
			p->my_current_bytes_xfered += p->actual_iosize;
			p->my_current_op_count++;
		}
		p->flushwrite_current_count++;
	} else { // Read operation 
		// Record the starting time for this read op
		pclk_now(&p->my_current_op_start_time);
		if (p->my_current_op == 0) // Record the start time for the first op
			p->my_first_op_start_time = p->my_current_op_start_time;

		// Issue the actual operation
		if ((p->target_options & TO_SGIO)) 
			 p->my_io_status = xdd_sg_io(p,'r'); // Issue the SGIO operation 
		else p->my_io_status = read(p->fd, p->rwbuf, p->actual_iosize);// Issue a normal read() operation
	
		// Record the ending time for this read operation and update counters
		pclk_now(&p->my_current_op_end_time);
		p->my_current_op_elapsed_time = (p->my_current_op_end_time - p->my_current_op_start_time);
		p->my_accumulated_op_time += p->my_current_op_elapsed_time;
		p->my_accumulated_read_op_time += p->my_current_op_elapsed_time;
		p->my_current_read_op_count++;
		p->my_io_errno = errno;
		if (p->my_io_status == p->actual_iosize) {
			p->my_current_bytes_read += p->actual_iosize;
			p->my_current_bytes_xfered += p->actual_iosize;
			p->my_current_op_count++;
		}
	} // end of WRITE/READ operation

	// issue a sync() to flush these buffers every so many operations
	if ((p->flushwrite > 0) && (p->flushwrite_current_count >= p->flushwrite)) {
		pclk_now(&start_time);
		fsync(p->fd);
		pclk_now(&end_time);
		p->my_accumulated_flush_time += (end_time - start_time);
		p->flushwrite_current_count = 0;
	}

	/* Time stamp! */
	if ((p->ts_options & TS_ON) && (p->ts_options & TS_TRIGGERED)) {
		p->ttp->tte[p->ttp->tte_indx++].end = p->my_current_op_end_time;
		if (p->ts_options & TS_ONESHOT) { // Check to see if we are at the end of the ts buffer 
			if (p->ttp->tte_indx == p->ttp->tt_size)
				p->ts_options &= ~TS_ON; // Turn off Time Stamping now that we are at the end of the time stamp buffer 
		} else if (p->ts_options & TS_WRAP) 
				p->ttp->tte_indx = 0; // Wrap to the beginning of the time stamp buffer 
	}

	// Check status of the last operation 
	if ((p->my_io_status < 0) || (p->my_io_status != p->actual_iosize)) {
		fprintf(xgp->errout, "(%d.%d) %s: I/O error on target %s - status %d, iosize %d, operation number %d\n",
			p->my_target_number,p->my_qthread_number,xgp->progname,p->target,p->my_io_status,p->actual_iosize,p->my_current_op);
		if (!(p->target_options & TO_SGIO)) {
			if ((p->my_io_status == 0) && (errno == 0)) { // Indicate this is an end-of-file condition
				fprintf(xgp->errout, "(%d.%d) %s: End-of-file reached on target %s at operation number %d\n",
					p->my_target_number,p->my_qthread_number,xgp->progname,p->target,p->my_current_op);
			} else {
				perror("reason"); // Only print the reason (aka errno text) if this is not an SGIO request
			}
		}
		p->my_current_error_count++;
	}
#endif 
} // End of xdd_io_for_linux()

/*----------------------------------------------------------------------------*/
/* xdd_io_for_windows() - This subroutine is only used on Windows
 * This will initiate the system calls necessary to perform an I/O operation
 * and any error recovery or post processing necessary.
 */
void
xdd_io_for_windows(ptds_t *p) {
#ifdef WIN32
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
	if (p->my_current_op == 0) /* record our starting time */
		p->my_start_time = p->my_current_start_time;
	if (p->seekhdr.seeks[p->my_current_op].operation == SO_OP_WRITE) {
		if (p->target_options & TO_SEQUENCED_PATTERN) {
			posp = (uint64_t *)p->rwbuf;
			for (uj=0; uj<(p->actual_iosize/sizeof(p->my_current_byte_location)); uj++) {
				*posp = p->my_current_byte_location + (uj * sizeof(p->my_current_byte_location));
				*posp |= p->data_pattern_prefix_binary;
				if (p->target_options & TO_INVERSE_PATTERN)
					*posp ^= 0xffffffffffffffffLL; // 1's compliment of the pattern
				posp++;
			}
		}
		/* Actually write the data to the storage device/file */
			p->my_io_status = WriteFile(p->fd, p->rwbuf, p->actual_iosize, &bytesxferred, NULL);
		} else { /* Simply do the normal read operation */
			p->my_io_status = ReadFile(p->fd, p->rwbuf, p->actual_iosize, &bytesxferred, NULL);
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
		if ((p->my_io_status == FALSE) || (bytesxferred != (unsigned long)p->actual_iosize)) { 
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
#endif
} // End of xdd_io_for_windows()

/*----------------------------------------------------------------------------*/
/* xdd_data_pattern_fill() - This subroutine will fill the buffer with a 
 * specific pattern. 
 * This routine is called within the inner I/O loop for every I/O if the data
 * pattern changes from IO to IO
 */
void
xdd_data_pattern_fill(ptds_t *p) {
	int32_t  		j;					/* random variables */
	uint64_t 		*posp;             	/* Position Pointer */
	pclk_t			start_time;			// Used for calculating elapsed times of ops
	pclk_t			end_time;			// Used for calculating elapsed times of ops

	/* Sequenced Data Pattern */
	if (p->target_options & TO_SEQUENCED_PATTERN) {
		pclk_now(&start_time);
		posp = (uint64_t *)p->rwbuf;
		for (j=0; j<(p->actual_iosize/sizeof(p->my_current_byte_location)); j++) {
			*posp = p->my_current_byte_location + (j * sizeof(p->my_current_byte_location));
			*posp |= p->data_pattern_prefix_binary;
			if (p->target_options & TO_INVERSE_PATTERN)
				*posp ^= 0xffffffffffffffffLL; // 1's compliment of the pattern
			posp++;
		}
		pclk_now(&end_time);
		p->my_accumulated_pattern_fill_time += (end_time - start_time);
	}
} // End of xdd_data_pattern_fill() 
