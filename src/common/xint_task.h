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

// Per Thread Data Structure - one for each thread 
#define TASK_REQ_IO				0x01					// Perform an IO Operation 
#define TASK_REQ_REOPEN			0x02					// Re-Open the target device/file
#define TASK_REQ_STOP			0x03					// Stop doing work and exit
#define TASK_REQ_EOF			0x04					// Send an EOF to the Destination or Revceive an EOF from the Source
struct xint_task {
	char				task_request;					// Type of Task to perform
	char				task_op;						// Operation to perform
	uint32_t			task_xfer_size;					// Number of bytes to transfer
	uint64_t			task_byte_location;				// Offset into the file where this transfer starts
	uint64_t			task_e2e_sequence_number;		// Sequence number of this task when part of an End-to-End operation
	nclk_t				task_time_to_issue;				// Time to issue the I/O operation or 0 if not used
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
