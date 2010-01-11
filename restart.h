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
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
struct restart {
	FILE		*fp;				// File pointer for the restart file
	char		*restart_filename;	// Name of the restart file
	char		*source_host;		// Name of the source host
	char		*source_filename;	// Name of the file on the source side of an xddcp
	char		*destination_host;	// Name of the destination host
	char		*destination_filename;	// Name of the file on the destination side of an xddcp
	uint64_t	byte_offset;		// Offset into the file of last good xmit/write
	uint64_t	low_byte_offset;	// Lowest Offset into the file 
	uint64_t	high_byte_offset;	// Highest Offset into the file 
	uint64_t	last_committed_location;	// Location (aka byte offset into file) that was last sent/written
	uint64_t	last_committed_length;	// Length of data that was last sent/written
	pclk_t		last_update;		// Time stamp of last update to restart file
	pclk_t		starting_time;		// When this restart operation started
	uint64_t	flags;				// Flags with various information as defined below
};
typedef struct restart restart_t;
// Restart.h flag bit definitions
#define	RESTART_FLAG_ISSOURCE		0x0000000000000001		// Indicates this is the source side of a copy
#define	RESTART_FLAG_RESUME_COPY	0x0000000000000002		// Indicates that this is a resumption of a previous copy
