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
/* target.c
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


/**************************************************************************
*   Target Initialization
**************************************************************************/
// This subroutine is called by the Target Thread when it starts.
// Target Initialization consists of:
//     Openning the target file for either input or output
//     Create all the Worker Threads for this target
// When a Worker Thread is started, it will put itself on the Worker 
// Thread Data Structure  Queue for its target. THe Worker Thread Data 
// Structure Queue anchor is located in the Target Data Structure of 
// the target.
//
// If all goes well then 0 is returned to the caller.
//
int
target_init(struct bx_td *p) {
    int 			status;
	int				i;
	unsigned int	qflags;
	struct bx_wd		*bx_wdp;

fprintf(stderr,"\n %s Target Thread %d: Initializing\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);
	qflags = 0;
	// Open the file
	if (p->bx_td_flags & BX_TD_INPUT) {
		p->bx_td_fd = open(p->bx_td_file_name, O_RDONLY);
		if (p->bx_td_fd < 0) {
			perror("target_init: Cannot open input file");
			return(-1);
		}
	} else	{ 
		p->bx_td_fd = creat(p->bx_td_file_name, S_IRWXU|S_IRWXG|S_IRWXO);
		if (p->bx_td_fd < 0) {
			perror("target_init: Cannot open/create output file");
			return(-1);
		}
	}

	// Create the Worker Threads
	for (i = 0; i < p->bx_td_number_of_worker_threads; i++) {
		bx_wdp = &p->bx_td_bx_wd[i];
		fprintf(stderr,"target_init: %s target %d creating Worker Threads %d of %d, fd=%d, bx_wdp=%p...\n",
				(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
				p->bx_td_my_target_number, 
				i+1, 
				p->bx_td_number_of_worker_threads,
				p->bx_td_fd,
				bx_wdp);
		bx_wdp->bx_wd_my_worker_thread_number = i;
		bx_wdp->bx_wd_flags = qflags;
		bx_wdp->bx_wd_fd = p->bx_td_fd;
		bx_wdp->bx_wd_my_queue = &p->bx_td_bx_wd_queue;
		bx_wdp->bx_wd_my_bx_tdp = p;
		status = pthread_create(&p->bx_td_worker_threads[i], NULL, worker_thread_main, bx_wdp);
		if (status) {
			perror("target_init: Cannot create Worker Threads");
			return(-1);
		}
	}
fprintf(stderr,"\n %s Target Thread %d: Initialization Complete\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);

	return(0);

} // End of target_init()

/**************************************************************************
*   The Input Target Thread
**************************************************************************/
// This thread will wait on its Worker Thread Data Structure  queue for a 
// Worker Threads to become available.
// Once it gets a Worker Threads then it will wait for a buffer to become 
// available. 
// After it gets a buffer it will assign the buffer to the Worker Threads 
// and release the Worker Threads to perform the operation.
// Since the Worker Threads run asynchronously, then after releasing the 
// Worker Threads this thread will repeat the process of waiting for a 
// Worker Thread Data Structure and Worker Threads, buffer, and releasing 
// the Worker Threads until all bytes from the input file have been 
// scheduled for transfer. 
//
// After all the operations have been scheduled, this thread will wait for
// each of its Worker Threads to become available at which time this 
// thread will set the "TERMINATE" flag in the Worker Thread Data Structure 
// for each Worker Threads and release the Worker Threads so that it can 
// terminate gracefully.
//
// Once all the input Worker Threads have terminated, it is safe to assume that
// all of the data from the input file has been given to the OUTPUT target.
// Therefore, this thread can 
int
target_input(struct bx_td *p) {
	long long int	bytes_remaining;
	struct bx_wd		*bx_wdp = 0;
	struct bx_buffer_header	*bufhdrp = 0;
	struct bx_buffer_queue *qp;
	int				i;
	int				transfer_size;
	off_t			current_file_offset;


fprintf(stderr,"target_input: - ENTER\n");
	// Now copy the data from the input file to the output file 
	bytes_remaining = p->bx_td_file_size;
	current_file_offset = 0;
	while(bytes_remaining > 0) {
fprintf(stderr,"target_input: - bytes_remaining=%lld\n",(long long int)bytes_remaining);
		// Get the next available input qrhread
		bx_wdp = bx_wd_dequeue(&p->bx_td_bx_wd_queue);
fprintf(stderr,"target_input: - got a Worker Thread Data Structure: %p\n",bx_wdp);
bufqueue_show(&p->bx_td_buffer_queue);	
		// Get the next available buffer from the available buffer queue
		bufhdrp = bh_dequeue(&p->bx_td_buffer_queue);
	
fprintf(stderr,"target_input: - got a BUFFER: %p\n",bufhdrp);
		if (bytes_remaining < p->bx_td_transfer_size)
			transfer_size = bytes_remaining;
		else transfer_size = p->bx_td_transfer_size;
		bufhdrp->bh_transfer_size = transfer_size;
		bufhdrp->bh_file_offset = current_file_offset;
		// Point this buffer to the Worker Thread Data Structure/Worker Threads 
		// and release the Worker Threads.
		bx_wdp->bx_wd_bufhdrp = bufhdrp;
		bx_wdp->bx_wd_released = 1;
		bx_wdp->bx_wd_next_buffer_queue = p->bx_td_next_buffer_queue;

		pthread_cond_broadcast(&bx_wdp->bx_wd_conditional);
		bytes_remaining -= transfer_size;
		current_file_offset += transfer_size;
fprintf(stderr,"target_input: - WORKER THREAD RELEASED\n");
	}

	// At this point all the input operations have been scheduled and 
	// this target needs to wait for the last Worker Threads to transfer its
	// data and complete. 
	// This target thread will wait on the Worker Threads until all the
	// Worker Threads have checked in as complete. After that, this subroutine
	// thread will exit.
	
	for (i = 0; i < p->bx_td_number_of_worker_threads; i++) {
		// Get the next available input qrhread
fprintf(stderr,"target_input: - Waiting for WORKER THREAD %d of %d\n",i+1,p->bx_td_number_of_worker_threads);
		bx_wdp = bx_wd_dequeue(&p->bx_td_bx_wd_queue);
		bx_wdp->bx_wd_flags |= BX_WD_TERMINATE;
		pthread_cond_broadcast(&bx_wdp->bx_wd_conditional);
fprintf(stderr,"target_input: - WORKER THREAD %d of %d has completed\n",i+1,p->bx_td_number_of_worker_threads);
	}

	// Now we wait for all the buffers to come back from the OUTPUT target
	// at which point we know that all the data has been written to the
	// output device. 
	for (i = 0; i < Number_Of_Buffers; i++) {
fprintf(stderr,"target_input: - Waiting for BUFFER %d of %d\n",i+1,Number_Of_Buffers);
		// Get a buffer from the buffer queue
		bufhdrp = bh_dequeue(&p->bx_td_buffer_queue);
fprintf(stderr,"target_input: - BUFFER %d of %d completed\n",i+1,Number_Of_Buffers);
	}
	
	// Now that all the buffers have returned from the OUTPUT target,
	// we take the last buffer header, set the "END_OF_FILE" flag,
	// and wake up the OUTPUT target who is waiting for a buffer to appear
	// on its buffer queue. When it wakes up and looks at this particular
	// buffer header, it will then know to break out of its loop and terminate.
	//
	// Set the End Of File flag in the buf header and wake up the output target
	bufhdrp->bh_flags |= BX_BUFHDR_END_OF_FILE;
	qp = &bx_td[bx_wdp->bx_wd_next_buffer_queue].bx_td_buffer_queue;
	bh_enqueue(bx_wdp->bx_wd_bufhdrp, qp);

fprintf(stderr,"\n Target Input  %d: DONE\n",p->bx_td_my_target_number);
    return 0;
} // End of target_input()

/**************************************************************************
*   The Output Target Thread
**************************************************************************/
// Just like the INPUT target thread only backwards.
int
target_output(struct bx_td *p) {
	struct bx_wd	*bx_wdp = 0;
	struct bx_buffer_header	*bufhdrp = 0;
	int				buffers_written;
	int				i;


	buffers_written = 0;
fprintf(stderr,"target_output: - ENTER\n");
	// Now copy the data from the input file to the output file 
	while(1) {
		// Get the next available buffer from the available buffer queue
		bufhdrp = bh_dequeue(&p->bx_td_buffer_queue);
fprintf(stderr,"target_output: - got a BUFHDR: %p\n",bx_wdp);
		if (bufhdrp->bh_flags & BX_BUFHDR_END_OF_FILE) {
fprintf(stderr,"target_output: - got an EOF: %p\n",bx_wdp);
			break;
		}
	
		// Get the next available output qrhread
		bx_wdp = bx_wd_dequeue(&p->bx_td_bx_wd_queue);
fprintf(stderr,"target_output: - got a Worker Thread Data Structure: %p\n",bx_wdp);
	
		// Point the Worker Threads to the buffer and release the Worker Threads
		bx_wdp->bx_wd_bufhdrp = bufhdrp;
		bx_wdp->bx_wd_released = 1;
		bx_wdp->bx_wd_next_buffer_queue = p->bx_td_next_buffer_queue;

		pthread_cond_broadcast(&bx_wdp->bx_wd_conditional);
		buffers_written++;
fprintf(stderr,"target_output: - WORKER THREAD RELEASED\n");
	}

	// Terminate all Worker Threads
	for (i = 0; i < p->bx_td_number_of_worker_threads; i++) {
		// Get the next available output qrhread
fprintf(stderr,"target_output: - Waiting for WORKER THREAD %d of %d\n",i+1,p->bx_td_number_of_worker_threads);
		bx_wdp = bx_wd_dequeue(&p->bx_td_bx_wd_queue);
		bx_wdp->bx_wd_flags |= BX_WD_TERMINATE;
		pthread_cond_broadcast(&bx_wdp->bx_wd_conditional);
fprintf(stderr,"target_output: - WORKER THREAD %d of %d has completed\n",i+1,p->bx_td_number_of_worker_threads);
	}
	fprintf(stderr,"\n Target Output  %d: %d buffers written - DONE\n",p->bx_td_my_target_number, buffers_written);

    return 0;
} // End of target_output()

/**************************************************************************
*   The Target Thread
**************************************************************************/
// When this thread is started it will call target_init() to perform its 
// initialization and then call target_input or target_output depending 
// on which target it is. Up returning from target_input/target_output
// the target thread will enter the "main" barrier where "main" is 
// waiting for all targets to complete. 
void *
target_main(void *pin) {
    int status;
	struct bx_td *p;


	p = (struct bx_td *)pin;

fprintf(stderr,"\n %s Target Thread %d: Starting\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);
	status = target_init(p);
	if (status != 0) {
		fprintf(stderr,"Target Thread %d: Error during initialization - exiting\n",p->bx_td_my_target_number);
		return NULL;
	}

	// Wait here for all other Targets to initialize
	pthread_barrier_wait(&Target_Barrier);
	
	// At this point all Targets have initialized 
	if (p->bx_td_flags & BX_TD_INPUT)
		status = target_input(p);
	else status = target_output(p);

fprintf(stderr,"\n %s Target Thread %d: COMPLETED - status=%d\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number,
			status);
	if (status != 0) {
		fprintf(stderr,"%s Target %d: failed\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);
	}

	fprintf(stderr,"\n %s Target Thread %d: DONE\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);

	// At this point this target is done so it will enter the "main barrier"
	// where "main" is waiting. When all targets arrive then the barrier
	// collapses and this target thread will exit.
fprintf(stderr,"\n %s Target Thread %d: Entering MAIN_BARRIER\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);
	pthread_barrier_wait(&Main_Barrier);
fprintf(stderr,"\n %s Target Thread %d: Leaving\n",
			(p->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", 
			p->bx_td_my_target_number);

    return NULL;
} // End of target_main()
