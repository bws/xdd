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

// ------------------------------------------------------------------------- //
// The Target Offset Table (TOT) is used as a mechanism to quickly determine 
// the lowest numbered and highest numbered blocks that are still in flight.
// This only makes sense when there is more than one Worker Thread assigned
// to a target.
//
// There is one Target Offset Table per Target. Each TOT contains a fixed
// number of tot_entries. The number of tot_entries is an integer multiple
// of the number of Worker Threads assigned to the target (aka -queuedepth).
// For example, if the -queuedepth is 4, then there are 4 Worker Threads
// and the size of the TOT will be N*4 where N defaults to 1000 times 
// the DEFAULT_TOT_MULTIPLIER.
//
// The tot_entries are assigned based on the block number being processed.
// The tot_entry number is the block number mod the size of the block table.
//
// Each Worker Thread Data structure contains a tot_wait structure. The
// tot_wait structure contains the condition variable that is waited on
// by a Worker Thread that needs to use a particular tot_entry. 
// In the event that more than one Worker Thread needs to use a specific
// tot_entry, the tot_wait structures of each Worker Thread will be chained
// together with the tot_entry being the anchor of the chain.
//
// +----------------+
// | TOT ENTRY      |
// |    tot_mutex   |  +-------------------+
// |    tot_waitp----->| TOT WAIT Worker 0 |  +-------------------+
// |     :          |  |   totw_nextp-------->| TOT WAIT Worker 2 |
// +----------------+  |   totw_condition  |  |   totw_nextp=0    |
//                     +-------------------+  |   totw_condition  |
//                                            +-------------------+
// The purpose of the TOT WAIT chain is to prevent TOT Collisions which
// is the situation when one Worker Thread has "ownership" of a specific
// tot_entry and a second Worker Thread needs to use it. This TOT WAIT
// chain forces any subsequent Worker Threads to wait until the 
// current Worker Thread completes its I/O operation and releases the
// specific tot_entry.
//
// For example, consider the following, a target with 4 worker threads.
// For the sake of aregument, say the TOT has only 4 enties.
//     t=0: Worker0 is assigned to write block0 of a target tot_entry=0
//     t=1: Worker1 is assigned to write block1 of a target tot_entry=1
//     t=2: Worker2 is assigned to write block1 of a target tot_entry=2
//     t=3: Worker3 is assigned to write block1 of a target tot_entry=3
//      :
//      :
//     t=75 Worker0 completes block0, now assigned block 4 tot_entry 0
//      :
//     t=77 Worker2 completes block2, now assigned block 5 tot_entry 1
//      :
//     t=79 Worker0 completes block4, now assigned block 6 tot_entry 2
//      :
// The problem comes at time t=77 when Worker2 tries to get ownership
// of tot_entry 1 which is currently owned by Worker1. At this point 
// Worker2 will have to wait for Worker1 to complete. 
//
// In a real-world workload, some Worker Threads get starved for cycles
// and take an inordinate amount of time to complete. A starved Worker
// Thread can be thousands of blocks behind all the other Worker Threads.
// So, for example, Worker1 could be trying to process block 1 but 
// because it is getting starved, the other Worker Threads could be
// processing block numbers in the thousands (if there were no 
// mechanism to prevent it).
//
#define TOT_ENTRY_AVAILABLE 1
#define TOT_ENTRY_UNAVAILABLE 0
/**
 * The Target Offset Table WAIT structure 
 * There is one of these per worker thread.
 */
struct tot_wait {
	struct tot_wait 		*totw_nextp;	// Pointer to the next tot_wait 
	struct xint_worker_data	*totw_wdp;		// Pointer to the Worker that owns this tot_wait
    pthread_cond_t 			totw_condition; 
    int 					totw_is_released;
	
};
typedef struct tot_wait tot_wait_t;

/** typedef unsigned long long iotimer_t; */
struct tot_entry {
    pthread_mutex_t tot_mutex;		// Mutex that is locked when updating items in this entry
	struct	tot_wait	*tot_waitp;			// Pointer to the first tot_wait struct in the chain.
    nclk_t tot_wait_ts;						// Time that another Worker Thread starts to wait on this
    nclk_t tot_post_ts;						// Time that the responsible Worker Thread posts this semaphore
    nclk_t tot_update_ts;					// Time that the responsible Worker Thread updates the byte_location and io_size
    int64_t tot_byte_offset;				// Byte Location that was just processed
    int64_t tot_op_number;					// Target Operation Number for the op that processed this block
    int32_t tot_io_size;					// Size of I/O in bytes that was just processed
    int32_t tot_wait_worker_thread_number;	// Number of the Worker Thread that is waiting for this TOT entry to be posted
    int32_t tot_post_worker_thread_number;	// Number of the Worker Thread that posted this TOT entry 
    int32_t tot_update_worker_thread_number;// Number of the Worker Thread that last updated this TOT Entry
	int32_t	tot_status;						// Either TOT_ENTRY_AVAILABLE or TOT_ENTRY_UNAVAILABLE
};
typedef struct tot_entry tot_entry_t;

/**
 * The array of Target Offset Table entries - one arrat per target
 */
struct tot {
	int tot_entries;  			// Number of tot entries
    struct tot_entry tot_entry[1];  // The ToT
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
