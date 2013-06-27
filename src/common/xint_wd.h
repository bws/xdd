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
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

// Worker Thread Data Structure - one for each worker thread 
struct xint_worker_data {
	target_data_t 				*wd_tdp;	 		// Pointer back to the Parent Target Data
	struct xint_worker_data		*wd_next_wdp;		// Pointer to the next worker_data struct in the list
	pthread_t  					wd_thread;			// Handle for this Worker_Thread
	int32_t   					wd_thread_number;	// My queue number within this target 
	int32_t   					wd_thread_id;  		// My system thread ID (like a process ID) 
	int32_t   					wd_pid;   			// My process ID 
	int32_t   					wd_file_desc; 		// File Descriptor for the target device/file 
	unsigned char 				*wd_rwbuf;   		// Pointer to the current read/write buffer
	int32_t						wd_rwbuf_shmid; 	// Shared Memory ID
	int64_t						wd_ts_entry;		// The TimeStamp entry to use when time-stamping an operation
	struct xint_task			wd_task;			// Task Structure
	struct xint_target_counters	wd_counters;		// Pointer to the target counters

	// Worker Thread-specific locks and associated pointers
	pthread_mutex_t	wd_worker_thread_target_sync_mutex;	// Used to serialize access to the Worker_Thread-Target Synchronization flags
	int32_t			wd_worker_thread_target_sync;		// Flags used to synchronize a Worker_Thread with its Target
#define	WDSYNC_AVAILABLE			0x00000001		// This Worker_Thread is available for a task, set by qthread, reset by xdd_get_specific_qthread.
#define	WDSYNC_BUSY					0x00000002		// This Worker_Thread is busy
#define	WDSYNC_TARGET_WAITING		0x00000004		// The parent Target is waiting for this Worker_Thread to become available, set by xdd_get_specific_qthread, reset by qthread.
#define	WDSYNC_EOF_RECEIVED			0x00000008		// This Worker_Thread received an EOF packet from the Source Side of an E2E Operation
    pthread_cond_t 	wd_this_worker_thread_is_available_condition;
	xdd_barrier_t	wd_thread_targetpass_wait_for_task_barrier;	// The barrier where the Worker_Thread waits for targetpass() to release it with a task to perform
	xdd_occupant_t	wd_occupant;		// Used by the barriers to keep track of what is in a barrier at any given time
	char			wd_occupant_name[XDD_BARRIER_MAX_NAME_LENGTH];	// For a Target thread this is "TARGET####", for a Worker_Thread it is "TARGET####WORKER####"
	xint_e2e_t		*wd_e2ep;			// Pointer to the e2e struct when needed
	xdd_sgio_t		*wd_sgiop;			// SGIO Structure Pointer
};
typedef struct xint_worker_data worker_data_t;
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
