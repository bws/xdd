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
struct heartbeat {
	uint32_t		hb_interval;			// Number of seconds to sleep between Heartbeat updates
	FILE			*hb_file_pointer;		// File pointer for the heartbeat output file
	char			*hb_filename;			// Name of the heartbeat file
	uint64_t		hb_options;				// Option flags
};
typedef struct heartbeat heartbeat_t;
// heartbeat.h hb_options bit definitions
#define HB_OPS					0x0000000000000001ULL  /* Display Current number of OPS performed */
#define HB_BYTES				0x0000000000000002ULL  /* Display Current number of BYTES transferred */
#define HB_KBYTES				0x0000000000000004ULL  /* Display Current number of KILOBYTES transferred */
#define HB_MBYTES				0x0000000000000008ULL  /* Display Current number of MEGABYTES transferred */
#define HB_GBYTES				0x0000000000000010ULL  /* Display Current number of GIGABYTES transferred */
#define HB_BANDWIDTH			0x0000000000000020ULL  /* Display Current Aggregate BANDWIDTH */
#define HB_IOPS	 				0x0000000000000040ULL  /* Display Current Aggregate IOPS */
#define HB_PERCENT	 			0x0000000000000080ULL  /* Display Percent Complete */
#define HB_ET		 			0x0000000000000100ULL  /* Display Estimated Time to Completion*/
#define HB_IGNORE_RESTART		0x0000000000000200ULL  /* Ignore the restart adjustments */
#define HB_LF					0x0000000000000400ULL  /* LineFeed/CarriageReturn/NewLine */
#define HB_TOD					0x0000000000000800ULL  /* Time of Day */
#define HB_ELAPSED				0x0000000000001000ULL  /* Elapsed Seconds */
#define HB_TARGET				0x0000000000002000ULL  /* Target Number */
#define HB_HOST					0x0000000000004000ULL  /* Host name */
