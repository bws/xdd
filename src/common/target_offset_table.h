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
    
    nclk_t tot_wait_ts;						// Time that another Worker Thread starts to wait on this
    nclk_t tot_post_ts;						// Time that the responsible Worker Thread posts this semaphore
    nclk_t tot_update_ts;					// Time that the responsible Worker Thread updates the byte_location and io_size
    int64_t tot_byte_offset;				// Byte Location that was just processed
    int64_t tot_op_number;					// Target Operation Number for the op that processed this block
    int32_t tot_io_size;					// Size of I/O in bytes that was just processed
    int32_t tot_wait_worker_thread_number;	// Number of the Worker Thread that is waiting for this TOT entry to be posted
    int32_t tot_post_worker_thread_number;	// Number of the Worker Thread that posted this TOT entry 
    int32_t tot_update_worker_thread_number;// Number of the Worker Thread that last updated this TOT Entry
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
	       int worker_thread_number,
	       int64_t offset,
	       int32_t size);

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
