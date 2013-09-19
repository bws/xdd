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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#define THIS_IS_A_SUBROUTINE 
#include "bx_data_structures.h"
//
// Show all the members of a buffer header
int
bufhdr_show(struct bx_buffer_header *bp) {


	fprintf(stderr,"BUFHDR: bp=%p, next=%p, prev=%p, flags=0x%016llu\n", bp, bp->bh_next, bp->bh_prev, bp->bh_flags); 
	fprintf(stderr,"BUFHDR: bp=%p, startp=%p, size=%d\n", bp, bp->bh_startp, bp->bh_size);
	fprintf(stderr,"BUFHDR: bp=%p, valid_startp=%p, valid_size=%d\n", bp, bp->bh_valid_startp, bp->bh_valid_size);
	fprintf(stderr,"BUFHDR: bp=%p, file_offset=%lld, sequence=%lld\n", bp, (long long int)bp->bh_file_offset,(long long int)bp->bh_sequence);
	fprintf(stderr,"BUFHDR: bp=%p, from=%p, to=%p\n", bp, bp->bh_from, bp->bh_to);
	fprintf(stderr,"BUFHDR: bp=%p, target=%d, time=%lld\n\n", bp, bp->bh_target,(long long int)bp->bh_time);
	return(0);
} // End of bufhdr_show()

//
// Show all the buffer headers on a buffer queue 
int
bufqueue_show(struct bx_buffer_queue *bq) {
	struct bx_buffer_header	*bp;


	fprintf(stderr,"\n");
	fprintf(stderr,"QUEUE: bq=%p, name='%s', counter=%d, flags=0x%08x\n", bq, bq->bq_name, bq->bq_counter, bq->bq_flags); 
	fprintf(stderr,"QUEUE: bq=%p, first=%p, last=%p\n\n", bq, bq->bq_first, bq->bq_last); 
	bp = bq->bq_first;
	while (bp) {
		bufhdr_show(bp);
		bp = bp->bh_next;
	}
	return(0);
} // End of bufqueue_show()

//
// Initialize a buffer queue 
int
bufqueue_init(struct bx_buffer_queue *bq, char *bq_name) {
	int	status;

fprintf(stderr,"bufqueue_init: Initializing buffer queue %s...",bq_name);
	bq->bq_name = bq_name;
	bq->bq_counter = 0;
	bq->bq_flags = 0;
	bq->bq_first = 0;
	bq->bq_last = 0;
	status = pthread_mutex_init(&bq->bq_mutex, 0);
	if (status) {
		perror("Error initializing queue mutex");
		return(-1);
	}
	status = pthread_cond_init(&bq->bq_conditional, 0);
	if (status) {
		perror("Error initializing queue barrier");
		return(-1);
	}
fprintf(stderr,"Done.\n");
	return(0);
} // End of bufqueue_init()

//
// Initialize a buffer header 
int
bufhdr_init(struct bx_buffer_header *bp, int bh_size) {
	int	status;


fprintf(stderr,"bufhdr_init: Initializing buffer header %p size %d ...",bp,bh_size);
	bp->bh_next = 0;
	bp->bh_prev = 0;
	status = pthread_mutex_init(&bp->bh_mutex, 0);
	if (status) {
		perror("Error initializing buffer header mutex");
		return(-1);
	}
	bp->bh_startp = (unsigned char *)valloc(bh_size);
	if (bp->bh_startp == NULL) {
		perror("Error allocating buffer");
		return(-1);
	}
	bp->bh_size = bh_size;
	bp->bh_valid_startp = 0;	
	bp->bh_valid_size = 0;
	bp->bh_file_offset = 0;
	bp->bh_sequence = 0;
	bp->bh_from = 0;
	bp->bh_to = 0;	
	bp->bh_target = 0;
	bp->bh_time = 0;
	bp->bh_flags = BX_BUFHDR_INITIALIZED;
fprintf(stderr,"Done.\n");
	return(0);
}
//
// Buffers always go at the end of the linked list
int
bh_enqueue(struct bx_buffer_header *bp, struct bx_buffer_queue *bq) {
	struct	bx_buffer_header	*tmp;
	int		status;

	// Get the lock for this buffer queue
	pthread_mutex_lock(&bq->bq_mutex);

	if (bq->bq_first == 0) { // Nothing on this queue so just add it
		bq->bq_first = bp;
		bq->bq_last = bq->bq_first;
		bq->bq_counter++;
		bp->bh_prev = 0;
		bp->bh_next = 0;
	} else { 
		// Get the address of the last buffer on the list
		tmp = bq->bq_last; 
		// forward and backward link the two buffer headers
		tmp->bh_next = bp;
		bp->bh_prev = tmp;
		bp->bh_next = 0;

		// Update the queue bq_counter and *last* pointer
		bq->bq_counter++;
		bq->bq_last = bp;
	}
	// If something is waiting for something to appear in this queue then release it
	if (bq->bq_flags & BX_DEQUEUE_WAITING) {
		// Issue a broadcast to wake up any threads waiting on this queue
		status = pthread_cond_broadcast(&bq->bq_conditional);
		if ((status != 0) && (status != PTHREAD_BARRIER_SERIAL_THREAD)) {
			fprintf(stderr,"bx_wd_enqueue: ERROR: pthread_cond_broadcast failed: status is %d, errno is %d\n", status, errno);
			perror("Reason");
		}
	}
	bp->bh_time = 0xDEAD0000;
	// Release the lock for this buffer queue
	pthread_mutex_unlock(&bq->bq_mutex);
	return(0);
} // End of enqueue()

// Buffers always come from the beginning/top of the linked list
struct bx_buffer_header *
bh_dequeue(struct bx_buffer_queue *bq) {
	struct	bx_buffer_header	*bp;
	//struct	bx_buffer_header	*tmp;
	//int		status;

	// Get the lock for this item queue
	pthread_mutex_lock(&bq->bq_mutex);
	bq->bq_flags |= BX_DEQUEUE_WAITING;
	while (bq->bq_counter == 0) { // Nothing on this queue - wait for something to appear
		pthread_cond_wait(&bq->bq_conditional, &bq->bq_mutex);
	}
	bq->bq_flags &= ~BX_DEQUEUE_WAITING;

	// Take the first item header from this queue
	bp = bq->bq_first; 
	bq->bq_first = bp->bh_next;
	if (bq->bq_first == 0) { // This is the case where there are no more item on this queue
		bq->bq_last = 0;
	} else { // There is at least 1 item on this queue and it's "prev" pointer must now be null
		bq->bq_first->bh_prev = 0;
	}
	bq->bq_counter--;
	// Release the lock for this item queue
	pthread_mutex_unlock(&bq->bq_mutex);
	return(bp);
} // End of dequeue()
/**************************************************************************
*   Queue and buffer initialization
**************************************************************************/
// This subroutine is called by MAIN. It will initialize the Buffer Header
// queue *anchors* in the INPUT and OUTPUT Target Data Structure.
// Once the buffer queues are initialized, then each buffer header is 
// initialized and stuffed onto the INPUT target Buffer Header Queue.
//
int
qb_init() {
	struct	bx_td	*p;
	struct	bx_buffer_header	*bp;
	int 	i;
	int		first_target;
    int 	status;


	for (i = 0; i < MAX_TARGETS; i++) {
		p = &bx_td[i];
		if (p->bx_td_flags & BX_TD_TARGET_DEFINED) {
			// Initialize input queue mutex and barrier
			status = bufqueue_init(&p->bx_td_buffer_queue, "BQ");
			if (status) 
				return(-1);
		}
	}
	first_target = Sequence[0];

	// Allocate buffers and stuff them on the available queue
// TBD - calculate the number of buffers based on the number of targets and number of worker_threads...etc
	for (i = 0; i < Number_Of_Buffers; i++) {
		// Init the buffer header
		bp = &Buf_Hdrs[i];
// TBD - the "buffer size" needs to be calculated as the maximum transfer size of all targets
		status = bufhdr_init(bp,Buffer_Size);
		if (status < 0) {
			fprintf(stderr,"qb_init: buffer header initialization failed for buffer %d\n",i);
			return(-1);
		}
		// Put this buffer on the INPUT Target queue
		status = bh_enqueue(bp,&bx_td[first_target].bx_td_buffer_queue);
	}
	
	return(0);

} // End of qb_init()

