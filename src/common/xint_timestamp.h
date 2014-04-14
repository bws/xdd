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

//------------------------------------------------------------------------------------------------//
// Timestamp structures
// The xint_timestamp structure lives inside the Target Data Structure and contains information
// relevant to time stamping when the timestamp option is specified. The members of xint_timestamp
// specify various timestamping suboptions (ts_options), the size of the timestamp table (ts_size),
// when to start timestamping (ts_trigop or ts_trigtime), where to put the timestamp results
// (ts_binary_filename and/or ts_output_filename), a pointer to the relevant output file (ts_tsfp),
// and a pinter to the actual timestamp header (ts_hdrp) which includes the array of timestamp 
// entries.
//
// The ts_hdrp member of the xint_timestamp structure points to the xdd_ts_header structure
// which is allocated only if the timestamp option has been specified and lives outside of the
// Target Data Structure. 
//
// The last member of the timestamp header is tsh_tte[] which is an array of timestamp table 
// entries. There is one timestamp table entry (tte) for each I/O operation that this target will
// perform over the course of an entire run. For example, if -numreqs is 100 and -passes is 3 
// then there will be a total of 3*100 or 300 timestamp table entries. 
//
//  +----- xint_target_data --------------+
//  |           :                         |
//  |           :                         |
//  | struct xint_timestamp  td_ts_table  |
//  |             ts_options              |
//  |             ts_size                 |
//  |             ts_trigop               |
//  |             ts_trigtime             |
//  |             *ts_binary_fliename     |
//  |             *ts_output_filename     |
//  |             *ts_tsfp                |
//  |             *ts_hdrp->>>>>>>>>>>>>>>>>+-- struct xdd_ts_header --------+
//  |             - - - - - - - - - - - - | |   tsh_magic                    |
//  |                                     | |      :                         |
//  |                                     | |      :                         |
//  |                                     | |   xdd_ts_tte_t  tsh_tte[]      |
//  |                                     | |      (time stamp table entries)|
//  |                                     | +--------------------------------+
//  +----- End of xint_target_data -------+
//
//------------------------------------------------------------------------------------------------//

#define MAX_IDLEN 8192 // This is the maximum length of the Run ID Length field
#define CTIME_BUFSZ 32  // Size of character buffer for the output from ctime(1)
                        // POSIX-2008 says this must be at least 26 bytes
#define XDD_VERSION_BUFSZ 64  // Size of the character buffer for the XDD version string

/** typedef unsigned long long iotimer_t; */
struct xdd_ts_tte {
    char 			tte_op_type;  	// operation: write=2, read=1, no-op=0
    char			tte_filler1;	// 
    short 			tte_pass_number;  	// Pass Number
    int32_t			tte_worker_thread_number;	// My Worker Thread Number
    int32_t         tte_thread_id;      // My system thread ID (like a process ID)
// 64 bits 8 bytes
    short 			tte_disk_processor_start;   // Processor number that this disk op was started on
    short 			tte_disk_processor_end;	// Processor number that this disk op ended on
    short 			tte_net_processor_start;    // Processor number that this net op was started on
    short 			tte_net_processor_end;	// Processor number that this net op ended on
// 128 bits 16 bytes
    int32_t			tte_disk_xfer_size;	   // Number of bytes transferred to/from disk
    int32_t		 	tte_net_xfer_size;     // Number of bytes transferred to/from network
    int32_t		 	tte_net_xfer_calls;    // Number of calls to send/recv to complete this op
// 192 bits 24 bytes
    int64_t		 	tte_op_number; 	// Operation number
// 256 bits 32 bytes
    int64_t		 	tte_byte_offset; 	// Location in bytes - aka Offset into the device/file
// 320 bits 40 bytes
    nclk_t 			tte_disk_start;  	// The starting time stamp of the disk operation
    nclk_t 			tte_disk_start_k;  	// The starting time stamp of the disk operation kernel
// 384 bits 48 bytes
    nclk_t 			tte_disk_end;  	    // The ending time stamp of the disk operation
    nclk_t 			tte_disk_end_k;     // The ending time stamp of the disk operation kernel
// 448 bits 56 bytes
    nclk_t 			tte_net_start;      // The starting time stamp of the net operation (e2e only)
    nclk_t 			tte_net_start_k;    // The starting time stamp of the net operation (e2e only) kernel
// 512 bits 64 bytes
    nclk_t 			tte_net_end;        // The ending time stamp of the net operation (e2e only)
    nclk_t 			tte_net_end_k;      // The ending time stamp of the net operation (e2e only) kernel
// 520 bits
//	struct timeval	usage_utime;	// usage_utime.tv_sec = usage.ru_utime.tv_sec;
//	struct timeval	usage_stime;	// usage_utime.tv_sec = usage.ru_utime.tv_sec;
//	long			nvcsw;			// Number of voluntary context switches so far
//	long			nivcsw;			// Number of involuntary context switches so far
};
typedef struct xdd_ts_tte xdd_ts_tte_t;

/**
 * Time stamp Trace Table Header - this gets written out before
 * the time stamp trace table data 
 */
struct xdd_ts_header {
    uint32_t	tsh_magic;          /**< Magic number indicating the beginning of timestamp data */
    char		tsh_version[XDD_VERSION_BUFSZ];        /**< Version string for the timestamp data format */
    int32_t		tsh_target_thread_id; // My system target thread ID (like a process ID)
    int32_t		tsh_reqsize; 	/**< size of these requests in 'blocksize'-byte blocks */
    int32_t 	tsh_blocksize; 	/**< size of each block in bytes */
    int64_t 	tsh_numents; 	/**< number of timestamp table entries */
    nclk_t		tsh_trigtime; 	/**< Time the time stamp started */
    int64_t 	tsh_trigop;  	/**< Operation number that timestamping started */
    int64_t 	tsh_res;  		/**< clock resolution - nano seconds per clock tick */
    int64_t 	tsh_range;  	/**< range over which the IO took place */
    int64_t 	tsh_start_offset;	/**< offset of the starting block */
    int64_t 	tsh_target_offset;	/**< offset of the starting block for each proc*/
    uint64_t 	tsh_global_options;	/**< options used */
    uint64_t 	tsh_target_options;	/**< options used */
    char 		tsh_id[MAX_IDLEN]; 	/**< ID string */
    char 		tsh_td[CTIME_BUFSZ];  	/**< time and date */
    nclk_t 		tsh_timer_oh; 	/**< Timer overhead in nanoseconds */
    nclk_t 		tsh_delta;  	/**< Delta used for normalization */
    int64_t 	tsh_tt_bytes; 	/**< Size of the entire time stamp table in bytes */
    size_t 		tsh_tt_size; 	/**< Size of the entire time stamp table in entries */
    int64_t 	tsh_tte_indx; 	/**< Index into the time stamp table */
    xdd_ts_tte_t tsh_tte[1]; 	/**< timestamp table entries */
};
typedef struct xdd_ts_header xdd_ts_header_t;

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

// The timestamp structure is pointed to from the Target Data Structure. 
// There is one timestamp structure for each Target that has timestamping enabled.
struct xint_timestamp {
	uint64_t			ts_options;  			// Time Stamping Options 
	int64_t				ts_current_entry; 		// Index into the Timestamp Table of the current entry
	int64_t				ts_size;  				// Time Stamping Size in number of entries 
	int64_t				ts_trigop;  			// Time Stamping trigger operation number 
	nclk_t				ts_trigtime; 			// Time Stamping trigger time 
	char				*ts_binary_filename; 	// Timestamp filename for the binary output file for this Target
	char				*ts_output_filename; 	// Timestamp report output filename for this Target
	FILE				*ts_tsfp;   			// Pointer to the time stamp output file 
	xdd_ts_header_t		*ts_hdrp;				// Pointer to the actual time stamp header and entries
};
typedef struct xint_timestamp xint_timestamp_t;

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
