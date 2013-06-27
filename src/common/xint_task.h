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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

// The xint_task structure is used to pass task parameters to a Worker Thread

#define TASK_REQ_IO				1	// Perform an IO Operation 
#define TASK_REQ_REOPEN			2	// Re-Open the target device/file
#define TASK_REQ_STOP			3	// Stop doing work and exit
#define TASK_REQ_EOF			4	// Send an EOF to the Destination or Revceive an EOF from the Source

#define TASK_OP_TYPE_READ		1	// Perform a READ operation
#define TASK_OP_TYPE_WRITE		2	// Perform a WRITE operation
#define TASK_OP_TYPE_NOOP		3	// Perform a NOOP operation
#define TASK_OP_TYPE_EOF		4	// End-of-File processing when present in the Time Stamp Table
struct xint_task {
	char				task_request;				// Type of Task to perform
	int					task_file_desc;				// File Descriptor
	unsigned char		*task_bufp;					// The data buffer
	char				task_op_type;				// Operation to perform
	char				*task_op_string;			// Operation to perform in ASCII
	uint64_t			task_op_number;				// Offset into the file where this transfer starts
	size_t				task_xfer_size;				// Number of bytes to transfer
	off_t				task_byte_offset;			// Offset into the file where this transfer starts
	uint64_t			task_e2e_sequence_number;	// Sequence number of this task when part of an End-to-End operation
	nclk_t				task_time_to_issue;			// Time to issue the I/O operation or 0 if not used
	ssize_t				task_io_status;				// Returned status of this I/O associated with this task
	int32_t				task_errno;					// Returned errno of this I/O associated with this task
};
typedef struct xint_task xint_task_t;

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
