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

#define SO_OP_WRITE 'w'        /**< Write seek entry type */
#define SO_OP_READ  'r'        /**< Read seek entry type */
#define SO_OP_WRITE_VERIFY 'v' /**< Write-Verify seek entry type */
#define SO_OP_NOOP  'n'        /**< NOOP seek entry type */
#define SO_OP_EOF  'e'        /**< EOF seek entry type */

/** A single seek entry */
struct seek_entries {
	int32_t operation; /**< read or write */
	int32_t reqsize; /**< Size of data transfer in blocks */
	uint64_t block_location; /**< Starting location in blocks */
	nclk_t time1;  /**< Relative time in nano seconds that this operation should start */
	nclk_t time2;  /**< not yet implemented */
};
typedef struct seek_entries seek_t;

/** Bit settings used for the vairous seek options */
#define SO_SEEK_SAVE      0x00000001 /**< Save seek locations in a file */
#define SO_SEEK_LOAD      0x00000002 /**< Load seek locations from a file */
#define SO_SEEK_RANDOM    0x00000004 /**< Random seek locations */
#define SO_SEEK_STAGGER   0x00000008 /**< Staggered seek locations */
#define SO_SEEK_NONE      0x00000010 /**< No seek locations */
#define SO_SEEK_DISTHIST  0x00000020 /**< Print the seek distance histogram */
#define SO_SEEK_SEEKHIST  0x00000040 /**< Print the seek location histogram */

/** The seek header contains all the information regarding seek locations */
struct seekhdr {
	uint64_t seek_options; /**< various seek option flags */
	int64_t  seek_range; /**< range of seek locations */
	int32_t  seek_seed; /**< seed used for generating random seek locations */
	int32_t  seek_interleave; /**< interleave used for generating sequential seek locations */
	int32_t  seek_stride;        /**< stride of each request...if > reqsize*/
	uint32_t seek_iosize; /**< The largest I/O size in the list */
	int32_t  seek_num_rw_ops;  /**< Number of read+write operations */
	int32_t  seek_total_ops;   /**< Total number of ops in the seek list including verifies */
	int32_t  seek_NumSeekHistBuckets;/**< Number of buckets for seek histogram */
	int32_t  seek_NumDistHistBuckets;/**< Number of buckets for distance histogram */
	char  *seek_savefile; /**< file to save seek locations into */
	char  *seek_loadfile; /**< file from which to load seek locations from */
	char  *seek_pattern; /**< The seek pattern used for this target */
	seek_t  *seeks;  /**< the seek list */
	char state[256];
	char *oldstate;
};
typedef struct seekhdr seekhdr_t;

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
