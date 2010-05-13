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

#define MAX_IDLEN 8192 // This is the maximum length of the Run ID Length field

/** typedef unsigned long long iotimer_t; */
struct tte {
	short 		pass_number;  	// Pass Number
	char 		op_type;  		// operation: write=2, read=1, no-op=0
	char 		filler1; 		// Alignment
	int32_t		qthread_number;	// My QThread Number
	int64_t 	op_number; 		// Operation number
	int64_t 	byte_location; 	// Location in bytes - aka Offset into the device/file
	pclk_t 		start;  		// The starting time stamp of this operation
	pclk_t 		end;  			// The ending time stamp of this operation
};
typedef struct tte tte_t;

/**
 * Time stamp Trace Table Header - this gets written out before
 * the time stamp trace table data 
 */
struct tthdr {
	int32_t		reqsize; 		/**< size of these requests in 'blocksize'-byte blocks */
	int32_t 	blocksize; 		/**< size of each block in bytes */
	int64_t 	numents; 		/**< number of timestamp table entries */
	pclk_t 		trigtime; 		/**< Time the time stamp started */
	int64_t 	trigop;  		/**< Operation number that timestamping started */
	int64_t 	res;  			/**< clock resolution - pico seconds per clock tick */
	int64_t 	range;  		/**< range over which the IO took place */
	int64_t 	start_offset;	/**< offset of the starting block */
	int64_t 	target_offset;	/**< offset of the starting block for each proc*/
	uint64_t 	global_options;	/**< options used */
	uint64_t 	target_options;	/**< options used */
	char 		id[MAX_IDLEN]; 	/**< ID string */
	char 		td[32];  		/**< time and date */
	pclk_t 		timer_oh; 		/**< Timer overhead in nanoseconds */
	pclk_t 		delta;  		/**< Delta used for normalization */
	int64_t 	tt_bytes; 		/**< Size of the entire time stamp table in bytes */
	int64_t 	tt_size; 		/**< Size of the entire time stamp table in entries */
	int64_t 	tte_indx; 		/**< Index into the time stamp table */
	struct 		tte tte[1]; 	/**< timestamp table entries */
};
typedef struct tthdr tthdr_t;

/** ts_options bit settings */
#define TS_NORMALIZE          0x00000001 /**< Time stamping normalization of output*/
#define TS_ON                 0x00000002 /**< Time stamping is ON */
#define TS_SUMMARY            0x00000004 /**< Time stamping Summary reporting */
#define TS_DETAILED           0x00000008 /**< Time stamping Detailed reporting */
#define TS_APPEND             0x00000010 /**< Time stamping Detailed reporting */
#define TS_DUMP               0x00000020 /**< Time stamping Dumping */
#define TS_WRAP               0x00000040 /**< Wrap the time stamp buffer */
#define TS_ONESHOT            0x00000080 /**< Stop time stamping at the end of the buffer */
#define TS_STOP               0x00000100 /**< Stop time stamping  */
#define TS_ALL                0x00000200 /**< Time stamp all operations */
#define TS_TRIGTIME           0x00000400 /**< Time stamp trigger time */
#define TS_TRIGOP             0x00000800 /**< Time stamp trigger operation number */
#define TS_TRIGGERED          0x00001000 /**< Time stamping has been triggered */
#define TS_SUPPRESS_OUTPUT    0x00002000 /**< Suppress timestamp output */
#define DEFAULT_TS_OPTIONS 0x00000000

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
