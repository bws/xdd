/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
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
	unsigned char		*task_datap;				// The data section of the I/O buffer
	char				task_op_type;				// Operation to perform
	char				*task_op_string;			// Operation to perform in ASCII
	uint64_t			task_op_number;				// The operation number for this target relative to the start of the file
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
