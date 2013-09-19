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
#define  THIS_IS_A_SUBROUTINE 
#include "bx_data_structures.h"

//
// Show all the members of a item header
int
bx_wd_show(struct bx_wd *bx_wdp) {

	fprintf(stderr,"Worker Thread Data Structure: bx_wdp=%p, next=%p, prev=%p, flags=0x%016x\n", bx_wdp, bx_wdp->bx_wd_next, bx_wdp->bx_wd_prev, bx_wdp->bx_wd_flags); 
	fprintf(stderr,"Worker Thread Data Structure: bx_wdp=%p, my_worker_thread_number=%d, fd=%d\n", bx_wdp, bx_wdp->bx_wd_my_worker_thread_number, bx_wdp->bx_wd_fd); 
	fprintf(stderr,"Worker Thread Data Structure: bx_wdp=%p, bufhdrp=%p, next_buffer_queue=%d, my_queue=%p\n", bx_wdp, bx_wdp->bx_wd_bufhdrp, bx_wdp->bx_wd_next_buffer_queue, bx_wdp->bx_wd_my_queue); 
	return(0);
} // End of bx_wd_show()

//
// Show all the items on this  queue 
int
bx_wd_queue_show(struct bx_wd_queue *qp) {
	struct bx_wd	*bx_wdp;


	fprintf(stderr,"\n");
	fprintf(stderr,"Worker Thread Data Structure QUEUE: qp=%p, name='%s', counter=%d, flags=0x%08x\n", qp, qp->q_name, qp->q_counter, qp->q_flags); 
	fprintf(stderr,"Worker Thread Data Structure QUEUE: qp=%p, first=%p, last=%p\n\n", qp, qp->q_first, qp->q_last); 
	bx_wdp = qp->q_first;
	while (bx_wdp) {
		bx_wd_show(bx_wdp);
		bx_wdp = bx_wdp->bx_wd_next;
	}
	return(0);
} // End of bx_wd_queue_show()

//
// Initialize a bx_wd queue 
int
bx_wd_queue_init(struct bx_wd_queue *qp, char *q_name) {
	int	status;


	qp->q_name = q_name;
	qp->q_counter = 0;
	qp->q_flags = 0;
	qp->q_first = 0;
	qp->q_last = 0;
	status = pthread_mutex_init(&qp->q_mutex, 0);
	if (status) {
		perror("Error initializing Worker Thread Data Structure queue mutex");
		return(-1);
	}
	status = pthread_cond_init(&qp->q_conditional, 0);
	if (status) {
		perror("Error initializing Worker Thread Data Structure queue conditional variable");
		return(-1);
	}
	return(0);
} // End of bx_wd_queue_init()

// Items always go at the end of the linked list
int
bx_wd_enqueue(struct bx_wd *bx_wdp, struct bx_wd_queue *qp) {
	struct	bx_wd	*tmp;
	int		status;

	// Get the lock for this bx_wd queue
	pthread_mutex_lock(&qp->q_mutex);

	if (qp->q_first == 0) { // Nothing on this queue so just add it
		qp->q_first = bx_wdp;
		qp->q_last = qp->q_first;
		qp->q_counter++;
		bx_wdp->bx_wd_prev = 0;
		bx_wdp->bx_wd_next = 0;
	} else { 
		// Get the address of the last item on the list
		tmp = qp->q_last; 
		// forward and backward link the two item headers
		tmp->bx_wd_next = bx_wdp;
		bx_wdp->bx_wd_prev = tmp;
		bx_wdp->bx_wd_next = 0;

		// Update the queue q_counter and *last* pointer
		qp->q_counter++;
		qp->q_last = bx_wdp;
	}
	// If something is waiting for something to appear in this queue then release it
	if (qp->q_flags & BX_WD_WAITING) {
		// Enter barrier of waiting dequeue() operation
		status = pthread_cond_broadcast(&qp->q_conditional);
		if ((status != 0) && (status != PTHREAD_BARRIER_SERIAL_THREAD)) {
			fprintf(stderr,"bx_wd_enqueue: ERROR: pthread_cond_broadcast failed: status is %d, errno is %d\n", status, errno);
			perror("Reason");
		}
	}
	// Release the lock for this item queue
	pthread_mutex_unlock(&qp->q_mutex);
	return(0);
} // End of bx_wd_enqueue()

// Buffers always come from the beginning/top of the linked list
struct bx_wd *
bx_wd_dequeue(struct bx_wd_queue *qp) {
	struct	bx_wd	*bx_wdp;


	// Get the lock for this item queue
	pthread_mutex_lock(&qp->q_mutex);
	qp->q_flags |= BX_WD_WAITING;
	while (qp->q_counter == 0) { // Nothing on this queue - wait for something to appear
		pthread_cond_wait(&qp->q_conditional, &qp->q_mutex);
	}
	qp->q_flags &= ~BX_WD_WAITING;

	// Take the first item header from this queue
	bx_wdp = qp->q_first; 
	qp->q_first = bx_wdp->bx_wd_next;
	if (qp->q_first == 0) { // This is the case where there are no more item on this queue
		qp->q_last = 0;
	} else { // There is at least 1 item on this queue and it's "prev" pointer must now be null
		qp->q_first->bx_wd_prev = 0;
	}
	qp->q_counter--;
	// Release the lock for this item queue
	pthread_mutex_unlock(&qp->q_mutex);
	return(bx_wdp);
} // End of bx_wd_dequeue()
