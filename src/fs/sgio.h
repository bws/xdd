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
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
