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
/* worker_thread.c
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
#define DEBUG 0


/**************************************************************************
*   The Worker Thread
**************************************************************************/
// There are one or more Worker Threads per target. 
// A Worker Thread will indicate that it is WAITING for something to do and
// place itself on the Worker Thread Data Structure  queue of its target. 
// Eventually the target will wake up a Worker Thread which is indicated 
// by a non-zero value in the bx_wd_released variable in the Worker Thread 
// Data Structure  for this Worker Thread. 
// Upon waking up, the Worker Thread will perform an INPUT or OUTPUT 
// operation depending on its target's designation. The Worker Thread 
// will have a buffer header structure that contains the location in the 
// file to perform the operation, the number of bytes to transfer, and the 
// I/O memory buffer. 
// Upon completing the requested I/O operation, the Worker Thread will 
// stuff its Buffer Header on to the "next buffer queue". In other words, 
// if this Worker Thread is an INPUT Worker Thread then it just read data 
// into its buffer.
// Therefore, upon completion of the read, this Worker Thread will stuff 
// its Buffer Header on the Buffer Header Queue of the OUTPUT target and
// wake up the OUTPUT target if it is waiting for a buffer.
// Likewise, if this Worker Thread is an OUTPUT Worker Thread then it just 
// wrote data from its buffer. Therefore, upon completion of the write, this
// Worker Thread will stuff its Buffer Header on the Buffer Header Queue 
// of the INPUT target and wake up the INPUT target if it is waiting 
// for a buffer.
//
// After putting its Buffer Header on the appropriate queue this Worker 
// Thread will stuff itself back onto its Worker Thread Data Structure
// queue and wait for something to do. 
//
// At some point the Worker Thread will wake up and find the "TERMINATE" 
// flag set in its Worker Thread Data Structure . At this point the Worker 
// Thread will break the loop and terminate. 
void *
worker_thread_main(void *pin) {   
    int 	status;
	struct bx_td 			*bx_tdp;
	struct bx_wd 			*bx_wdp;
	struct bx_buffer_queue 	*qp;
	struct bx_buffer_header	*bufhdrp;
	nclk_t					nclk;


	bx_wdp = (struct bx_wd *)pin;
	bx_tdp = bx_wdp->bx_wd_my_bx_tdp;
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu: worker_thread_main: ENTER: bx_wdp=%p, bx_tdp=%p\n", (unsigned long long int)nclk,bx_wdp, bx_tdp);
	status = 0;
	
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - ENTER %d\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number);

	while (1) {
		// Set flag to indicate that this worker_thread is WAITING
		bx_wdp->bx_wd_flags |= BX_WD_WAITING;
		// Enqueue this Worker Thread Data Structure  on the bx_wd_queue for this target
		bx_wd_enqueue(bx_wdp, bx_wdp->bx_wd_my_queue);
		// Wait for the target thread to release me
		pthread_mutex_lock(&bx_wdp->bx_wd_mutex);
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - got the bx_wd_mutex lock - waiting for something to do - time %d\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number);
		bx_wdp->bx_wd_flags |= BX_WD_WAITING;
		while (1 != bx_wdp->bx_wd_released) {
         	pthread_cond_wait(&bx_wdp->bx_wd_conditional, &bx_wdp->bx_wd_mutex);
		}
		bx_wdp->bx_wd_flags &= ~BX_WD_WAITING;
		bx_wdp->bx_wd_released = 0;
		if (bx_wdp->bx_wd_flags & BX_WD_TERMINATE)
			break;

nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - got the bx_wd_mutex lock - GOT something to do - time %d\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number);

bx_wd_show(bx_wdp);
		bufhdrp = bx_wdp->bx_wd_bufhdrp;
		if (bx_tdp->bx_td_flags & BX_TD_INPUT) { // Read the input file
			status = pread(bx_wdp->bx_wd_fd, bufhdrp->bh_startp, bufhdrp->bh_transfer_size, bufhdrp->bh_file_offset);
			if (status < 0) {
				perror("Read error");
				bufhdrp->bh_valid_size = 0;
			} else {
				bufhdrp->bh_valid_size = status;
			}
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - read %d of %d bytes starting at offset %d - time %zd\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number,status,bufhdrp->bh_transfer_size,bufhdrp->bh_file_offset);
			// Put this buffer on the output target queue
			qp = &bx_td[bx_wdp->bx_wd_next_buffer_queue].bx_td_buffer_queue;
			bh_enqueue(bx_wdp->bx_wd_bufhdrp, qp);
		} else { // Must be output
			status = pwrite(bx_wdp->bx_wd_fd, bufhdrp->bh_startp, bufhdrp->bh_transfer_size, bufhdrp->bh_file_offset);
			if (status < 0) {
				perror("Write error");
				bufhdrp->bh_valid_size = 0;
			} else {
				bufhdrp->bh_valid_size = status;
			}
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - wrote %d of %d bytes starting at offset %d - requeuing buffer %zd - time %p\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number,status,bufhdrp->bh_transfer_size,bufhdrp->bh_file_offset, bufhdrp);
			// Put this buffer on the input target queue
			qp = &bx_td[bx_wdp->bx_wd_next_buffer_queue].bx_td_buffer_queue;
			bh_enqueue(bufhdrp, qp);
			bufqueue_show(qp);
		}
nclk_now(&nclk);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - transferred %d bytes - time %d\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number,status);
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s - releasing the bx_wd_mutex lock %d\n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number);
		pthread_mutex_unlock(&bx_wdp->bx_wd_mutex);
	}
if (DEBUG) fprintf(stderr,"%llu worker_thread_main: my_worker_thread_number=%s %d - Exit \n", (unsigned long long int)nclk,(bx_tdp->bx_td_flags & BX_TD_INPUT)?"INPUT":"OUTPUT", bx_wdp->bx_wd_my_worker_thread_number);
   	return 0;
} // End of worker_thread_main()
