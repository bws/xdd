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

// NOTE: Global variables are defined in bx_data_structures.h and live here
#undef THIS_IS_A_SUBROUTINE
#include "bx_data_structures.h"
int qb_init();

/**************************************************************************
*   MAIN
**************************************************************************/
// Start the program
int main(int argc, char *argv[]) {
	int i;
    int status;


	Buffer_Size = 0;
	Number_Of_Buffers = 0;
	// Initialize the BX_TD memory areas to zero
	for (i = 0; i < MAX_TARGETS; i++) {
		memset((void *)&bx_td[i], 0, sizeof(struct bx_td));
	}
	// Initialize the Sequence array
	for (i = 0; i < MAX_SEQUENCE_ENTRIES; i++) {
		Sequence[i] = -1;
	}
	
	// Process the Command Line
	status = ui(argc, argv);
	if (status < 0) 
		return(-1);
fprintf(stderr,"main: Buffer_Size=%d, number of targets=%d\n",Buffer_Size,Number_Of_Targets);
	// Initialize the main barrier where all targets meet before starting
	status = pthread_barrier_init(&Main_Barrier,0,Number_Of_Targets+1);
	if (status) {
		perror("Cannot init MAIN BARRIER ");
		return(errno);
	}
	status = pthread_barrier_init(&Target_Barrier,0,Number_Of_Targets);
	if (status) {
		perror("Cannot init TARGET BARRIER ");
		return(errno);
	}

	// Initialize the Worker Thread Data Structure and Buffer queues
	status = qb_init();
	if (status) {
		fprintf(stderr,"MAIN: Error initializing buffers - leaving now\n");
		return(1);
	}
	bufqueue_show(&bx_td[0].bx_td_buffer_queue);

	// Create the target_threads
	for (i = 0; i < MAX_TARGETS; i++) {
		if (bx_td[i].bx_td_flags & BX_TD_TARGET_DEFINED) {
			fprintf(stderr,"main: Creating target thread %d \n",i);
			status = pthread_create(&Targets[i], NULL, target_main, &bx_td[i]);
			if (status) {
				perror("Cannot create target thread");
				return(errno);
			}
		}
	}
	fprintf(stderr,"main: %d targets started\n",Number_Of_Targets);

	// Wait here for both target threads to complete then exit, stage left
	fprintf(stderr,"main: WATING FOR %d targets to complete\n",Number_Of_Targets);
	pthread_barrier_wait(&Main_Barrier);
	fprintf(stderr,"main: %d targets have completed! \n",Number_Of_Targets);

	// That's all folks

    return 0;
} // End of main()
