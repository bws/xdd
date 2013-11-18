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
#ifndef XINT_WD_H
#define XINT_WD_H

// Worker Thread Data Structure - one for each worker thread 
struct xint_worker_data {
	target_data_t 				*wd_tdp;	 		// Pointer back to the Parent Target Data
	struct xint_worker_data		*wd_next_wdp;		// Pointer to the next worker_data struct in the list
	pthread_t  					wd_thread;			// Handle for this Worker_Thread
	int32_t   					wd_worker_number;	// My worker number within this target relative to 0
	int32_t   					wd_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   					wd_pid;   			// My process ID 
	unsigned char				*wd_bufp;			// Pointer to the generic I/O buffer
	int							wd_buf_size;		// Size in bytes of the generic I/O buffer
	int64_t						wd_ts_entry;		// The TimeStamp entry to use when time-stamping an operation
	struct xint_task			wd_task;			// Task Structure
	struct xint_target_counters	wd_counters;		// Counters specific to this worker for this target

	// Worker Thread-specific locks and associated pointers
	pthread_mutex_t				wd_worker_thread_target_sync_mutex;	// Used to serialize access to the Worker_Thread-Target Synchronization flags
	int32_t						wd_worker_thread_target_sync;		// Flags used to synchronize a Worker_Thread with its Target
#define	WTSYNC_AVAILABLE		0x00000001		// This Worker_Thread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
#define	WTSYNC_BUSY				0x00000002		// This Worker_Thread is busy
#define	WTSYNC_TARGET_WAITING	0x00000004		// The parent Target is waiting for this Worker_Thread to become available, set by xdd_get_specific_qthread, reset by qthread.
#define	WTSYNC_EOF_RECEIVED		0x00000008		// This Worker_Thread received an EOF packet from the Source Side of an E2E Operation
    pthread_cond_t 				wd_this_worker_thread_is_available_condition;
	xdd_barrier_t				*wd_current_barrier;	// The barrier where the Worker_Thread is currently at
	xdd_barrier_t				wd_thread_targetpass_wait_for_task_barrier;	// The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
	xdd_occupant_t				wd_occupant;		// Used by the barriers to keep track of what is in a barrier at any given time
	char						wd_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH];	// For a Target thread this is "TARGET####", for a Worker_Thread it is "TARGET####WORKER####"
	tot_wait_t					wd_tot_wait;		// The TOT Wait structure for this worker
	xint_e2e_t					*wd_e2ep;			// Pointer to the e2e struct when needed
	xdd_sgio_t					*wd_sgiop;			// SGIO Structure Pointer
	pthread_mutex_t 			wd_current_state_mutex; 	// Mutex for locking when checking or updating the state info
	uint32_t					wd_current_state;			// State of this thread at any given time (see Current State definitions below)
	// State Definitions for "my_current_state"
#define	WORKER_CURRENT_STATE_INIT									0x00000001	// Initialization 
#define	WORKER_CURRENT_STATE_IO										0x00000002	// Waiting for an I/O operation to complete
#define	WORKER_CURRENT_STATE_DEST_RECEIVE							0x00000004	// Waiting to receive data - Destination side of an E2E operation
#define	WORKER_CURRENT_STATE_SRC_SEND								0x00000008	// Waiting for "send" to send data - Source side of an E2E operation
#define	WORKER_CURRENT_STATE_BARRIER								0x00000010	// Waiting inside a barrier
#define	WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE			0x00000020	// Worker Thread is waiting for the TOT lock in order to update the block number
#define	WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE		0x00000040	// Worker Thread is waiting for the TOT lock in order to release the next I/O
#define	WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS				0x00000080	// Worker Thread is waiting for the TOT lock to set the "wait" time stamp
#define	WORKER_CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO				0x00000100	// Waiting on the previous I/O op semaphore
};
typedef struct xint_worker_data worker_data_t;

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
