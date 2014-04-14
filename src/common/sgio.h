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
//
// ------------------ SGIO stuff --------------------------------------------------
// The following variables are used by the SCSI Generic I/O (SGIO) Routines
//
struct	xdd_sgio	{
#define SENSE_BUFF_LEN 64					// Number of bytes for the Sense buffer 
	uint64_t			sg_from_block;			// Starting block location for this operation 
	uint32_t			sg_blocks;				// The number of blocks to transfer 
	uint32_t			sg_blocksize;			// The size of a single block for an SG operation - generally == p->blocksize 
    unsigned char 		sg_sense[SENSE_BUFF_LEN]; // The Sense Buffer  
	uint32_t   			sg_num_sectors;			// Number of Sectors from Read Capacity command 
	uint32_t   			sg_sector_size;			// Sector Size in bytes from Read Capacity command 
};
typedef struct xdd_sgio xdd_sgio_t;

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
