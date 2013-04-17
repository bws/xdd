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
#ifndef TARGET_OFFSET_TABLE_H
#define TARGET_OFFSET_TABLE_H
#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include "xint_nclk.h"

/** typedef unsigned long long iotimer_t; */
struct tot_entry {
    pthread_mutex_t tot_mutex;		// Mutex that is locked when updating items in this entry
    pthread_cond_t tot_condition;
    int is_released;
    
    nclk_t tot_wait_ts;			// Time that another QThread starts to wait on this
    nclk_t tot_post_ts;			// Time that the responsible QThread posts this semaphore
    nclk_t tot_update_ts;			// Time that the responsible QThread updates the byte_location and io_size
    int64_t tot_byte_location;		// Byte Location that was just processed
    int64_t tot_op_number;			// Target Operation Number for the op that processed this block
    int32_t tot_io_size;			// Size of I/O in bytes that was just processed
    int32_t tot_wait_qthread_number;	// Number of the QThread that is waiting for this TOT entry to be posted
    int32_t tot_post_qthread_number;	// Number of the QThread that posted this TOT entry 
    int32_t tot_update_qthread_number;	// Number of the QThread that last updated this TOT Entry
};
typedef struct tot_entry tot_entry_t;

/**
 * Time stamp Trace Table Header - this gets written out before
 * the time stamp trace table data 
 */
struct tot {
	int tot_entries;  			// Number of tot entries
	struct tot_entry tot_entry[1]; 	// The ToT
};
typedef struct tot tot_t;

/**
 * Initialize the target offset table
 *
 * @return 0 on success, non-zero on failure
 */
int tot_init(tot_t** table, size_t queue_depth, size_t num_reqs);

/**
 * Deallocate resources acquired for target offset table
 *
 * @return 0 on success, non-zero on failure
 */
int tot_destroy(tot_t* table);

/**
 * Update the target offset table
 *
 * @return 0 on success, non-zero on failure
 */
int tot_update(tot_t* table,
	       int64_t req_number,
	       int qthread_number,
	       int64_t offset,
	       int32_t size);

#endif

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
