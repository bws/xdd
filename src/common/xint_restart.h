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
#ifndef RESTART_H
#define RESTART_H

struct xint_restart {
    char			*restart_filename;		// Name of the restart file
    off_t                       initial_restart_offset;
	FILE			*fp;					// File pointer for the restart file
	char			*source_host;			// Name of the source host
	char			*source_filename;		// Name of the file on the source side of an xddcp
	char			*destination_host;		// Name of the destination host
	char			*destination_filename;	// Name of the file on the destination side of an xddcp
	int64_t			byte_offset;			// Offset into the file of a "resume" operation
	int64_t			last_committed_byte_offset;// Byte offset into file that was last sent/written
	int64_t			last_committed_length;	// Length of data that was last sent/written
	int64_t			last_committed_op;		// Location (aka byte offset into file) that was last sent/written
	nclk_t			last_update;			// Time stamp of last update to restart file
	nclk_t			starting_time;			// When this restart operation started
	struct tm		tm;						// The time structure contains the time the restart files were created
	uint64_t		flags;					// Flags with various information as defined below
	pthread_mutex_t	restart_lock;			// Lock on this structure to serialize updates 
};
typedef struct xint_restart xint_restart_t;
// Restart.h flag bit definitions
#define	RESTART_FLAG_ISSOURCE					0x0000000000000001		// Indicates this is the source side of a copy
#define	RESTART_FLAG_RESUME_COPY				0x0000000000000002		// Indicates that this is a resumption of a previous copy
#define	RESTART_FLAG_SUCCESSFUL_COMPLETION		0x0000000000000004		// Indicates that the e2e (aka copy) operation completed successfully
#define	RESTART_FLAG_RESTART_FILE_NOW_CLOSED	0x0000000000000008		// Indicates that the restart file has been closed

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
