/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
////////////////////////////////////////////////////////////////////
//  Runtime constants - HACK
#define MAX_NUMBER_OF_BUFFERS	128
#define MAX_BUFFER_SIZE			(8*1024*1024)
#define MAX_TARGETS				128
#define MAX_WORKER_THREADS		128
#define MAX_SEQUENCE_ENTRIES	MAX_TARGETS

#define THOUSAND 1000
#define MILLION 1000000
#define BILLION 1000000000

typedef unsigned long long int nclk_t;  /* Number of nanoseconds */

// The buffer header 
struct bx_buffer_header	{
	struct	bx_buffer_header		*bh_next;			// Pointer to the next Buffer Header in this list
	struct	bx_buffer_header		*bh_prev;			// Pointer to the previous Buffer Header in this list
	pthread_mutex_t		bh_mutex;			// Locking mutex for this Buffer Header
	unsigned char		*bh_startp;			// Start of the memory buffer
	int					bh_size;			// Size of memory buffer in bytes
	unsigned char		*bh_valid_startp;	// Start of valid data
	int					bh_valid_size;		// Size of valid data in buffer
	off_t				bh_file_offset;		// Offset into file of this chunk of data
	int					bh_transfer_size;	// Number of bytes to transfer on a given read/write
	long long int		bh_sequence;		// A sequence number of this operation
	char				*bh_from;			// Where this data came from
	char				*bh_to;				// Where this data goes to
	int					bh_target;		// Number of the target that filled this buffer
	long long int		bh_time;			// Time stamp of when this buffer was last accessed
	unsigned long long	bh_flags;			// Misc flags 
};
typedef struct bx_buffer_header bx_buffer_header_t;
#define BX_BUFHDR_INITIALIZED 0x0000000000000001
#define BX_BUFHDR_END_OF_FILE 0x0000000000000002


// The queue anchor
struct bx_buffer_queue	{
	char				*bq_name;			// An ASCII string that identifies this queue
	pthread_mutex_t		bq_mutex;			// Locking mutex for this queue
	pthread_cond_t		bq_conditional;		// The pthread conditional variable used by the target to wait for something to appear on this queue
	int					bq_counter;			// Number of Buffer Header structures on this queue
	int					bq_flags;			// Misc flags
	struct bx_buffer_header		*bq_first;			// Pointer to the first Buffer Header on this queue
	struct bx_buffer_header		*bq_last;			// Pointer to the last Buffer Header on this queue
};
typedef struct bx_buffer_queue bx_buffer_queue_t;
#define BX_DEQUEUE_WAITING 0x00000001

// The Worker Thread Data Structure is the Worker Thread Data Structure
// The Worker Thread Data Structure contains variables needed during an I/O operation
// The Buffer Header (bx_buffer_header) contains:
//     Data Buffer
//     Length of data in the data buffer (valid data)
//     Offset into the file where this block came from or goes to
// Each Worker Thread Data Structure is associated with a specific Worker Thread. 
// The Worker Thread Data Structure structures live on a queue that is owned by a target.
// The Worker Threads wait on the q_conditional variable at the base of 
//    the Worker Thread Data Structure queue. 
// When the target needs a Worker Thread, it will 
//    - lock the Worker Thread Data Structure queue base
//    - take a Worker Thread Data Structure (and effectively the associated Worker Thread) off the queue
//    - decrement the number of Worker Thread Data Structure (or Worker Threads) remaining on the queue
//    - assign a task to the Worker Thread
//    - set a flag in the Worker Thread Data Structure to indicate this Worker Thread is ACTIVE
//    - perform a pthread_broadcast() on the queue's q_conditional variable
//    - unlock the Worker Thread Data Structure queue base
// The pthread_broadcast() will cause all the waiting Worker Threads to check the
//    their "flags" to see if they are "ACTIVE" or "WAITING". If they are
//    "WAITING" then they will just go back into the pthread_cond_wait().
//    Otherwise, if they are "ACTIVE" then they will proceed to execute
//    the requested I/O operation.
// After the I/O operation has completed the Worker Thread will 
//     - place its buffer header on the "next_bx_buffer_queue"
//     - Set its flags to "WAITING" (and unset the "ACTIVE" bit)
//     - Place its Worker Thread Data Structure back on the Worker Thread Data Structure queue
//     - Perform a pthread_cond_wait() on the q_conditional
//
struct bx_wd {
	struct bx_wd		*bx_wd_next;			// Pointer to the next Worker Thread Data Structure in the list
	struct bx_wd		*bx_wd_prev;			// Pointer to the previous Worker Thread Data Structure in the list
	pthread_mutex_t		bx_wd_mutex;			// Locking mutex for this Worker Thread Data Structure
	pthread_cond_t		bx_wd_conditional;		// The pthread conditional variable used by the associated Worker Thread to wait
	char				bx_wd_released;			// Indicates that the associated Worker Thread has been released
	int					bx_wd_my_worker_thread_number;		// The number of this worker_thread for this target (relative to 0)
	struct bx_td			*bx_wd_my_bx_tdp;		// Pointer to the Target Data Structure of the target that owns this worker_thread
	int					bx_wd_fd;				// The File Descriptor to use when issuing a read() or write()
	unsigned int		bx_wd_flags;			// Misc flags 
	struct bx_buffer_header		*bx_wd_bufhdrp;			// Pointer to the Buffer Header to use for an I/O operation
	int				 	bx_wd_next_buffer_queue;	// Target number of the next target to queue this buffer to
	struct bx_wd_queue 	*bx_wd_my_queue;		// Pointer to my Worker Thread Data Structure Queue anchor
};
#define BX_WD_WAITING 	0x00000008	// Indicates that the associated Worker Thread is WAITING for something to do
#define BX_WD_TERMINATE 	0x00000010	// Indicates that the associated Worker Thread should terminate

// The bx_wd queue anchor
struct bx_wd_queue	{
	char				*q_name;			// An ASCII string that identifies this queue
	pthread_mutex_t		q_mutex;			// Locking mutex for this Worker Thread Data Structure queue anchor
	pthread_cond_t		q_conditional;		// Conditional variable used by the Worker Thread to wait for something to appear on this queue
	int					q_counter;			// Number of Worker Thread Data Structure structures on this queue
	unsigned int		q_flags;			// Misc flags for this queue
	struct bx_wd		*q_first;			// Pointer to the first Worker Thread Data Structure on this queue
	struct bx_wd		*q_last;			// Pointer to the last Worker Thread Data Structure on this queue
};

// The Target Data Structure - Per Target Data Structure
//
struct bx_td {
		int					bx_td_my_target_number; 		// The number of the target that owns this Target Data Structure
		char				*bx_td_file_name;			// The name of the file for this target
		int					bx_td_fd;					// The file descriptor for this target
		int					bx_td_number_of_worker_threads;	// Number of Worker Threads for this target
		long long int		bx_td_file_size;				// Size of a file to create (in bytes)
		int					bx_td_transfer_size;			// Data Transfer size for I/O
		pthread_t			bx_td_worker_threads[MAX_WORKER_THREADS];// The pthread structures for this target
		struct bx_wd 		bx_td_bx_wd[MAX_WORKER_THREADS];	// The Worker Thread Data Structure structures for this target
		unsigned int		bx_td_flags;					// Misc flags for this target
		char				*bx_td_network_user;			// The "user" portion of user@hostname:port
		char				*bx_td_network_hostname;		// The "hostname" portion of user@hostname:port
		char				*bx_td_network_port;			// The "port" portion of user@hostname:port
		char				*bx_td_sg_device_name;		// The fully qualified path name of the SG device
		struct bx_buffer_queue 	bx_td_buffer_queue;				// The Buffer Queue anchor for this target
		int				 	bx_td_next_buffer_queue;		// Target number of the next target to queue this buffer to
		struct bx_wd_queue 	bx_td_bx_wd_queue;			// The Worker Thread Data Structure Queue anchor for this target
};
#define BX_TD_INPUT 				0x00000001
#define BX_TD_OUTPUT 				0x00000002
#define BX_TD_INTERNAL 				0x00000004
#define BX_TD_TYPE_FILE 			0x00000008
#define BX_TD_TYPE_FILE_BIO 		0x00000010
#define BX_TD_TYPE_FILE_DIO 		0x00000020
#define BX_TD_TYPE_FILE_NULL 		0x00000040
#define BX_TD_TYPE_NETWORK			0x00000080
#define BX_TD_TYPE_NETWORK_CLIENT	0x00000100
#define BX_TD_TYPE_NETWORK_SERVER	0x00000200
#define BX_TD_TYPE_NETWORK_TCP		0x00000400
#define BX_TD_TYPE_NETWORK_UDP		0x00000800
#define BX_TD_TYPE_NETWORK_UDT		0x00001000
#define BX_TD_TYPE_NETWORK_RDMA		0x00002000
#define BX_TD_TYPE_SG				0x00004000
#define BX_TD_TARGET_DEFINED		0x80000000

int bx_wd_show(struct bx_wd *bx_wdp);
int bx_wd_queue_show(struct bx_wd_queue *qp);
int bx_wd_queue_init(struct bx_wd_queue *qp, char *q_name);
int bx_wd_enqueue(struct bx_wd *bx_wdp, struct bx_wd_queue *qp);
struct bx_wd *bx_wd_dequeue(struct bx_wd_queue *qp);
int bufhdr_show(struct bx_buffer_header *bp);
int bufqueue_show(struct bx_buffer_queue *bq);
int bufqueue_init(struct bx_buffer_queue *bq, char *bq_name);
int bufhdr_init(struct bx_buffer_header *bp, int bh_size);
int bh_enqueue(struct bx_buffer_header *bp, struct bx_buffer_queue *bq);
struct bx_buffer_header *bh_dequeue(struct bx_buffer_queue *bq);
void nclk_now(nclk_t *nclkp);
int ui(int argc, char *argv[]);
int get_target_file_options(int target_number, int index, int argc, char *argv[]);
int get_target_network_options(int target_number, int index, int argc, char *argv[]);
int get_target_sg_options(int target_number, int index, int argc, char *argv[]);
int get_target_options(int index, int argc, char *argv[]);
int get_sequence(int index, int current_argc, int argc, char *argv[]);
void usage(char *progname);
void *worker_thread_main(void *pin);
int target_init(struct bx_td *p);
int target_input(struct bx_td *p);
int target_output(struct bx_td *p);
void *target_main(void *pin);

// GLOBAL VARIABLES
#ifdef THIS_IS_A_SUBROUTINE
#define EXTERNAL extern
#else
#define EXTERNAL 
#endif
EXTERNAL 	pthread_barrier_t	Main_Barrier;
EXTERNAL 	pthread_barrier_t	Target_Barrier;
EXTERNAL 	pthread_t			Targets[MAX_TARGETS];
EXTERNAL 	struct bx_td 		bx_td[MAX_TARGETS];
EXTERNAL 	struct bx_buffer_header 		Buf_Hdrs[MAX_NUMBER_OF_BUFFERS];
EXTERNAL 	int					Number_Of_Targets;
EXTERNAL 	int					Number_Of_Buffers;
EXTERNAL 	int 				Buffer_Size;
EXTERNAL 	int					Debug_Level;
EXTERNAL 	int					Sequence[MAX_SEQUENCE_ENTRIES+1];
