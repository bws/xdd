/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *  I/O Perofrmance, Inc.
 */
/*
   xdd - a basic i/o performance test
*/
#define XDDMAIN /* This is needed to make the xdd_globals pointer a static variable here and an extern everywhere else as defined in xdd.h */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* This is the main entry point for xdd. This is where it all begins and ends.
 */
int32_t
main(int32_t argc,char *argv[]) {
	int32_t i,j,q;
	int32_t status;
	pclk_t base_time; /* This is time t=0 */
	pclk_t tt; 
	pthread_t heartbeat_thread;
	pthread_t results_thread;
	ptds_t *p;
	char *c;
#if (LINUX)
	struct rlimit rlim;
#endif


	// Initialize the Global Data Structure
	xdd_init_globals(argv[0]);

	// Parse the input arguments 
	xgp->argc = argc; // remember the original arg count
	xgp->argv = argv; // remember the original argv 
	xdd_parse(argc,argv);

	// Init output format header
	if (xgp->global_options & GO_ENDTOEND) 
		xdd_results_format_id_add("+E2ESRTIME+E2EIOTIME+E2EPERCENTSRTIME ");

	// Optimize runtime priorities and all that 
	xdd_schedule_options();

	// initialize the signal handlers 
	xdd_init_signals();

	// Init all the necessary barriers 
	xdd_init_all_barriers();
#ifdef WIN32
	/* Init the ts serializer mutex to compensate for a Windows bug */
	xgp->ts_serializer_mutex_name = "ts_serializer_mutex";
	ts_serializer_init(&xgp->ts_serializer_mutex, xgp->ts_serializer_mutex_name);
#endif
#if ( IRIX )
	/* test if allowed this many processes */
	if (xgp->number_of_iothreads > prctl(PR_MAXPROCS)) {
			fprintf(xgp->errout, "Can only utilize %d processes per user\n",prctl(PR_MAXPROCS));
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(1);
	}
#endif 
	/* initialize the clocks */
	pclk_initialize(&tt);
	if (tt == PCLK_BAD) {
			fprintf(xgp->errout, "%s: Cannot initialize the picosecond clock\n",xgp->progname);
			fflush(xgp->errout);
			xdd_destroy_all_barriers();
			exit(1);
	}
	pclk_now(&base_time);
	// Init the Global Clock 
	xdd_init_global_clock(&xgp->ActualLocalStartTime);
	// display configuration information about this run 
	xdd_config_info();
#if (LINUX)
	// Get the current resource usage and set the limit to a constant unless the -rlimit option is specified as something other than 0
	status = getrlimit(RLIMIT_STACK, &rlim);
	if (status) {
		fprintf(xgp->errout, "%s: WARNING: Cannot get resource limit stack size\n",xgp->progname);
		perror("Reason");
		fflush(xgp->errout);
	} else { // Try to set the rlimits if requested
		if (xgp->rlimit == 0) { // Use the Default RLIMIT
			rlim.rlim_cur = DEFAULT_XDD_RLIMIT_STACK_SIZE;
			rlim.rlim_max = DEFAULT_XDD_RLIMIT_STACK_SIZE;
		} else if (xgp->rlimit > 0) {
			rlim.rlim_cur = xgp->rlimit;
			rlim.rlim_max = xgp->rlimit;
		} 
		status = setrlimit(RLIMIT_STACK, &rlim);
		if (status) {
			fprintf(xgp->errout, "%s: WARNING: Cannot set resource limit stack size to %lld\n",xgp->progname, rlim.rlim_cur);
			perror("Reason");
			fflush(xgp->errout);
		}
	}
#endif
	// Start up all the threads 
	/* The basic idea here is that there are N targets and each target can have X instances ( or queue threads as they are referred to)
	 * The "ptds" array contains pointers to the main ptds's for the primary targets.
	 * Each target then has a pointer to a linked-list of ptds's that constitute the targets that are used to implement the queue depth.
	 * The thread creation process is serialized such that when a thread is created, it will perform its initialization tasks and then
	 * enter the "serialization" barrier. Upon entering this barrier, the *while* loop that creates these threads will be released
	 * to create the next thread. In the meantime, the thread that just finished its initialization will enter the main thread barrier 
	 * waiting for all threads to become initialized before they are all released. Make sense? I didn't think so.
	 */
	for (j=0; j<xgp->number_of_targets; j++) {
		p = xgp->ptdsp[j]; /* Get the ptds for this target */
		q = 0;
		while (p) { /* create a thread for each ptds for this target - the queue depth */
			/* Create the new thread */
			p->run_start_time = base_time;
			status = pthread_create(&p->thread, NULL, xdd_io_thread, p);
			if (status) {
				fprintf(xgp->errout,"%s: Error creating thread %d",xgp->progname, j);
				fflush(xgp->errout);
				perror("Reason");
				xdd_destroy_all_barriers();
				exit(1);
			}
			p->my_target_number = j;
			p->my_qthread_number = q;
			p->total_threads = xgp->number_of_iothreads;
			/* Wait for the previous thread to complete initialization and then create the next one */
			xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
            		// If the previous target *open* fails, then everything needs to stop right now.
            		if (xgp->abort_io == 1) { 
			        fprintf(xgp->errout,"%s: xdd_main: xdd thread %d aborting due to previous initialization failure\n",xgp->progname,p->my_target_number);
		        	fflush(xgp->errout);
				xdd_destroy_all_barriers();
				exit(1);
	    		}
			q++;
			p = p->nextp; /* get next ptds in the queue for this target */
		}
	} /* end of FOR loop that starts all procs */
	/* Start the Results Manager */
	status = pthread_create(&results_thread, NULL, xdd_results_manager, (void *)(unsigned long)0);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_main: Error creating Results Manager thread", xgp->progname);
		fflush(xgp->errout);
	}
	/* start a heartbeat monitor if necessary */
	if (xgp->heartbeat) {
		status = pthread_create(&heartbeat_thread, NULL, xdd_heartbeat, (void *)(unsigned long)j);
		if (status) {
			fprintf(xgp->errout,"%s: xdd_main: Error creating heartbeat monitor thread", xgp->progname);
			fflush(xgp->errout);
		}
	}

	/* Wait for all children to finish */
	xdd_barrier(&xgp->final_barrier);

	// Display the Ending Time for this run
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(xgp->output,"Ending time for this run, %s\n",c);
	if (xgp->csvoutput)
		fprintf(xgp->csvoutput,"Ending time for this run, %s\n",c);

	/* Cleanup the semaphores and barriers */
	xdd_destroy_all_barriers();

	/* Time to leave... sigh */
	return(0);
} /* end of main() */
 
