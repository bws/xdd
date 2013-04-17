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
 *       Bradley Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

#define MAX_IDLEN 8192 // This is the maximum length of the Run ID Length field
#define CTIME_BUFSZ 32  // Size of character buffer for the output from ctime(1)
                        // POSIX-2008 says this must be at least 26 bytes
#define XDD_VERSION_BUFSZ 64  // Size of the character buffer for the XDD version string

/** typedef unsigned long long iotimer_t; */
struct tte {
    char 			op_type;  	// operation: write=2, read=1, no-op=0
    char			filler1;	// 
    short 			pass_number;  	// Pass Number
    int32_t			qthread_number;	// My QThread Number
    int32_t         thread_id;      // My system thread ID (like a process ID)
// 64 bits 8 bytes
    short 			disk_processor_start;   // Processor number that this disk op was started on
    short 			disk_processor_end;	// Processor number that this disk op ended on
    short 			net_processor_start;    // Processor number that this net op was started on
    short 			net_processor_end;	// Processor number that this net op ended on
// 128 bits 16 bytes
    int32_t			disk_xfer_size;	   // Number of bytes transferred to/from disk
    int32_t		 	net_xfer_size;     // Number of bytes transferred to/from network
    int32_t		 	net_xfer_calls;    // Number of calls to send/recv to complete this op
// 192 bits 24 bytes
    int64_t		 	op_number; 	// Operation number
// 256 bits 32 bytes
    int64_t		 	byte_location; 	// Location in bytes - aka Offset into the device/file
// 320 bits 40 bytes
    nclk_t 			disk_start;  	// The starting time stamp of the disk operation
    nclk_t 			disk_start_k;  	// The starting time stamp of the disk operation kernel
// 384 bits 48 bytes
    nclk_t 			disk_end;  	    // The ending time stamp of the disk operation
    nclk_t 			disk_end_k;     // The ending time stamp of the disk operation kernel
// 448 bits 56 bytes
    nclk_t 			net_start;      // The starting time stamp of the net operation (e2e only)
    nclk_t 			net_start_k;    // The starting time stamp of the net operation (e2e only) kernel
// 512 bits 64 bytes
    nclk_t 			net_end;        // The ending time stamp of the net operation (e2e only)
    nclk_t 			net_end_k;      // The ending time stamp of the net operation (e2e only) kernel
// 520 bits
//	struct timeval	usage_utime;	// usage_utime.tv_sec = usage.ru_utime.tv_sec;
//	struct timeval	usage_stime;	// usage_utime.tv_sec = usage.ru_utime.tv_sec;
//	long			nvcsw;			// Number of voluntary context switches so far
//	long			nivcsw;			// Number of involuntary context switches so far
};
typedef struct tte tte_t;

/**
 * Time stamp Trace Table Header - this gets written out before
 * the time stamp trace table data 
 */
struct tthdr {
    uint32_t    magic;          /**< Magic number indicating the beginning of timestamp data */
    char       version[XDD_VERSION_BUFSZ];        /**< Version string for the timestamp data format */
    int32_t    target_thread_id; // My system target thread ID (like a process ID)
    int32_t	reqsize; 	/**< size of these requests in 'blocksize'-byte blocks */
    int32_t 	blocksize; 	/**< size of each block in bytes */
    int64_t 	numents; 	/**< number of timestamp table entries */
    nclk_t 	trigtime; 	/**< Time the time stamp started */
    int64_t 	trigop;  	/**< Operation number that timestamping started */
    int64_t 	res;  		/**< clock resolution - nano seconds per clock tick */
    int64_t 	range;  	/**< range over which the IO took place */
    int64_t 	start_offset;	/**< offset of the starting block */
    int64_t 	target_offset;	/**< offset of the starting block for each proc*/
    uint64_t 	global_options;	/**< options used */
    uint64_t 	target_options;	/**< options used */
    char 	id[MAX_IDLEN]; 	/**< ID string */
    char 	td[CTIME_BUFSZ];  	/**< time and date */
    nclk_t 	timer_oh; 	/**< Timer overhead in nanoseconds */
    nclk_t 	delta;  	/**< Delta used for normalization */
    int64_t 	tt_bytes; 	/**< Size of the entire time stamp table in bytes */
    int64_t 	tt_size; 	/**< Size of the entire time stamp table in entries */
    int64_t 	tte_indx; 	/**< Index into the time stamp table */
    struct 	tte tte[1]; 	/**< timestamp table entries */
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
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
