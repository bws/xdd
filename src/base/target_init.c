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
/*
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xint_target_init() - Initialize a Target Thread
 * This subroutine will open the target file and perform some initial sanity 
 * checks. 
 * 
 * It will then start all WorkerThreads that are responsible for doing the actual I/O.
 *
 * It is important to note that upon entering this subroutine, the Worker Data substructure
 * has already been built. This routine will simply create each WorkerThread and 
 * pass it the address of the Worker Data that is unique to each WorkerThread.
 * The Worker Data Struct that each WorkerThread receives has its WorkerThread number.
 * 
 * Return Values: 0 is good, -1 indicates an error.
 * 
 */
int32_t
xint_target_init(target_data_t *tdp) {
	int32_t		status;			// Status of function calls
//	nclk_t		CurrentLocalTime;	// Used the init the Global Clock
//	nclk_t		TimeDelta;		// Used the init the Global Clock
//	uint32_t	sleepseconds;
	

#if (AIX)
	tdp->td_thread_id = thread_self();
#elif (LINUX)
	tdp->td_thread_id = syscall(SYS_gettid);
#else
	tdp->td_thread_id = tdp->td_pid;
#endif

	// Set the pass number
	tdp->td_counters.tc_pass_number = 1;

	// Check to see that the target is valid and can be opened properly
	status = xdd_target_open(tdp);
	if (status) 
		return(-1);

	/* Perform preallocation if needed */
	xint_target_preallocate(tdp);
        
	/* Perform pretruncation if needed */
	xint_target_pretruncate(tdp);

	// The Seek List
	// There is one Seek List per target.
	// This list contains all the locations in a implied order that need to be accessed
	// during a single pass. 
	// It is this list that is used by xdd_issue() to assign I/O tasks to the WorkerThreads.
	//
	tdp->td_seekhdr.seek_total_ops = tdp->td_target_ops;
	tdp->td_seekhdr.seeks = (seek_t *)calloc((int32_t)tdp->td_seekhdr.seek_total_ops,sizeof(seek_t));
	if (tdp->td_seekhdr.seeks == 0) {
		fprintf(xgp->errout,"%s: xdd_target_thread_init: ERROR: Cannot allocate memory for access list for Target %d name '%s' - terminating\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		fflush(xgp->errout);
		xgp->abort = 1;
		return(-1);
	}

	xdd_init_seek_list(tdp);

	// Set up the timestamp table - Note: This must be done *after* the seek list is initialized
	xdd_ts_setup(tdp); 

	// Set up for the big loop 
	if (xgp->max_errors == 0) 
		xgp->max_errors = tdp->td_target_ops;

	/* If we are synchronizing to a Global Clock, let's synchronize
	 * here so that we all start at *roughly* the same time
	 */
	// FIXME - TOM review to see if this can go in plan_init
//	if (xgp->gts_addr) {
//		nclk_now(&CurrentLocalTime);
//		while (CurrentLocalTime < xgp->ActualLocalStartTime) {
//		    TimeDelta = ((xgp->ActualLocalStartTime - CurrentLocalTime)/BILLION);
//	    	    if (TimeDelta > 2) {
//			sleepseconds = TimeDelta - 2;
//			sleep(sleepseconds);
//		    }
//		    nclk_now(&CurrentLocalTime);
//		}
//	}
	if (xgp->global_options & GO_TIMER_INFO) {
		fprintf(xgp->errout,"Starting now...\n");
		fflush(xgp->errout);
	}

	/* This section will initialize the slave side of the lock step mutex and barriers */
	status = xdd_lockstep_init(tdp); 
	if (status != XDD_RC_GOOD)
		return(status);
	
	// Initialize the barriers and mutex
	status = xint_target_init_barriers(tdp);
	if (status) 
	    return(-1);
	
	// Initialize the Target Offset Table
	tdp->td_totp = 0;
	status = tot_init(&(tdp->td_totp), tdp->td_queue_depth, tdp->td_target_ops);
	if (status) {
	    return(-1);
	}
	
	// Special setup for an End-to-End operation
	if (tdp->td_target_options & TO_ENDTOEND) {
		status = xdd_e2e_target_init(tdp);
		if (status)
			return(-1);
	}

	// Start the WorkerThreads
	status = xint_target_init_start_worker_threads(tdp);
	if (status) 
		return(-1);

	// If this is XNI, perform the connection here
	xdd_plan_t *planp = tdp->td_planp;
	if (PLAN_ENABLE_XNI & planp->plan_options) {
		/* Perform the XNI accept/connect */
		if (tdp->td_target_options & TO_E2E_DESTINATION) { 
			status = xint_e2e_dest_connect(tdp);
		} else {
			status = xint_e2e_src_connect(tdp);
		}
		if (0 != status) {
			fprintf(xgp->errout, "Failure during XNI connection.\n");
			return -1;
		}
	}

	// Display the information for this target
	xdd_target_info(xgp->output, tdp);
	if (xgp->csvoutput)
		xdd_target_info(xgp->csvoutput, tdp);

	return(0);
} // End of xdd_target_init()

/*----------------------------------------------------------------------------*/
/* xint_target_init_barriers() - Initialize the barriers and mutex
 * managed by the Target Thread.
 * Barrier Naming Convention: 
 *    (1) The first part of the name is the barrier "owner" or thread that initializes the barrier
 *    (2) The second part of the name is the other [dependant] thread that will use this barrier
 *    (3) The third part of the name is a descriptive word about the function of this barrier
 *    (4) The fourth part of the name is the word "barrier" to indicate what it is
 *
 * It is important to use the xdd_init_barrier() subroutine to initialize all barriers within XDD
 * because it keeps track of all initialized barriers so that they can be properly shut down
 * upon any exit from XDD. This prevents an unwanted accumulation of zombie semaphores lying about.
 *
 * Return value is 0 if everything succeeded or -1 if there was a failure.
 */
int32_t
xint_target_init_barriers(target_data_t *tdp) {
    int32_t		status;								// Status of subroutine calls
    char		tmpname[XDD_BARRIER_MAX_NAME_LENGTH];	// Used to create unique names for the barriers


	// The following initialization calls are all done with accumulated status for ease of coding.
	// Normal/good status from xdd_init_barrier() is zero so if everything initializes properly then
	// the accumulated status should be zero. Any barrier that is not properly initialized is cause
	// for exiting XDD altogether. 
	status = 0;

	// The Target_WorkerThread Initialization barrier
	sprintf(tmpname,"T%04d:target_worker_thread_init_barrier",tdp->td_target_number);
	status += xdd_init_barrier(tdp->td_planp, &tdp->td_target_worker_thread_init_barrier,2, tmpname);

	// The Target Pass barrier
	sprintf(tmpname,"T%04d>targetpass_worker_thread_passcomplete_barrier",tdp->td_target_number);
	status += xdd_init_barrier(tdp->td_planp, &tdp->td_targetpass_worker_thread_passcomplete_barrier,tdp->td_queue_depth+1,tmpname);

	// The Target Pass E2E EOF Complete barrier - only initialized when an End-to-End operation is running
	if (tdp->td_target_options & TO_ENDTOEND) {
		sprintf(tmpname,"T%04d>targetpass_worker_thread_eofcomplete_barrier",tdp->td_target_number);
		status += xdd_init_barrier(tdp->td_planp, &tdp->td_targetpass_worker_thread_eofcomplete_barrier,2,tmpname);
	}

	// The Target Start Trigger barrier 
	if (tdp->td_target_options & TO_WAITFORSTART) { // If we are expecting a Start Trigger then we need to init the starttrigger barrier
		sprintf(tmpname,"T%04d>target_target_starttrigger_barrier",tdp->td_target_number);
		status += xdd_init_barrier(tdp->td_planp, &tdp->td_trigp->target_target_starttrigger_barrier,2,tmpname);
	}

	// The "td_counters_mutex" is used by the WorkerThreads when updating the counter information in the Target Thread Data
	status += pthread_mutex_init(&tdp->td_counters_mutex, 0);

	if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_barriers: ERROR: Cannot create td_counters_mutex for target number %d name '%s'\n",
			xgp->progname, 
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		fflush(xgp->errout);
		return(-1);
	}

	// Initialize the semaphores used to control WorkerThread selection
	tdp->td_any_worker_thread_available = 0;
	status = pthread_mutex_init(&tdp->td_any_worker_thread_available_mutex, NULL);
	status = pthread_cond_init(&tdp->td_any_worker_thread_available_condition, NULL);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_barriers: Target %d: ERROR: Cannot initialize any_worker_thread_available condvar.\n",
			xgp->progname, 
			tdp->td_target_number);
		fflush(xgp->errout);
		return(-1);
	}

	return(0);
} // End of xdd_target_init_barriers()

/*----------------------------------------------------------------------------*/
/* xint_target_thread_init_start_worker_threads() - Start up all WorkerThreads 
 *
 * The actual WorkerThread routine called xdd_worker_thread() is located in worker_thread.c 
 *
 * Return value is 0 if everything succeeded or -1 if there was a failure.
 */
int32_t
xint_target_init_start_worker_threads(target_data_t *tdp) {
	worker_data_t	*wdp;					// Pointer to the WorkerThread Data Struct
	int32_t			q;						// WorkerThread Number
	int32_t			status;					// Status of subtroutine calls
	int32_t			e2e_addr_index;			// index into the e2e address table
	int32_t			e2e_addr_port;			// Port number in the e2e address table

	
	// Now let's start up all the WorkerThreads for this target
	wdp = tdp->td_next_wdp; // This is the first WorkerThread
	e2e_addr_index = 0;
	e2e_addr_port = 0;
	for (q = 0; q < tdp->td_queue_depth; q++ ) {
	    pthread_attr_t worker_thread_attr;

	    // Initialize the attributes
	    pthread_attr_init(&worker_thread_attr);
	    
	    // Start a WorkerThread and wait for it to initialize
	    wdp->wd_worker_number = q;
	    if (tdp->td_target_options & TO_ENDTOEND) {

		// Find an e2e entry that has a valid port count
		while (0 == tdp->td_e2ep->e2e_address_table[e2e_addr_index].port_count) {
		    e2e_addr_index++;
		}
		//assert(e2e_addr_index < p->e2ep->e2e_address_table->number_of_entries);
		
		wdp->wd_e2ep->e2e_dest_hostname = tdp->td_e2ep->e2e_address_table[e2e_addr_index].hostname;
		wdp->wd_e2ep->e2e_dest_port = tdp->td_e2ep->e2e_address_table[e2e_addr_index].base_port + e2e_addr_port;
		
		// Set the WorkerThread Numa node if possible
#if defined(HAVE_CPU_SET_T) && defined(HAVE_PTHREAD_ATTR_SETAFFINITY_NP)
		status = pthread_attr_setaffinity_np(&worker_thread_attr,
						     sizeof(tdp->td_e2ep->e2e_address_table[e2e_addr_index].cpu_set),
						     &tdp->td_e2ep->e2e_address_table[e2e_addr_index].cpu_set);
#endif
		// Roll over to the begining of the list if that is required
		e2e_addr_port++;
		if (e2e_addr_port == tdp->td_e2ep->e2e_address_table[e2e_addr_index].port_count) {
		    e2e_addr_index++;
		    e2e_addr_port = 0;
		}
		if (xgp->global_options & GO_REALLYVERBOSE)
		    fprintf(stderr,"Target Init: Target %d: assigning hostname %s port %d to worker_thread %d\n",
			    tdp->td_target_number, wdp->wd_e2ep->e2e_dest_hostname,
			    wdp->wd_e2ep->e2e_dest_port, wdp->wd_worker_number);
	    }

	    
	    status = pthread_create(&wdp->wd_thread, &worker_thread_attr, xdd_worker_thread, wdp);
	    if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_start_worker_threads: ERROR: Cannot create worker_thread %d for target number %d name '%s' - Error number %d\n",
			xgp->progname, 
			q,
			tdp->td_target_number,
			tdp->td_target_full_pathname,
			status);
		fflush(xgp->errout);
		// Set errno to the correct error number so that perror() can display the error message
		errno = status;
		perror("Reason");
		return(-1);
	    }
	    /* Wait for the previous thread to initialize before creating the next one */
	    xdd_barrier(&tdp->td_target_worker_thread_init_barrier,&tdp->td_occupant,1);
	    if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(xgp->errout,"\r%s: xdd_target_init_start_worker_threads: Target %d WorkerThread %d of %d started\n",
			xgp->progname,
			tdp->td_target_number,
			q,
			tdp->td_queue_depth);
	    }

	    // If the WorkerThread initialization fails, then everything needs to stop right now.
	    if (xgp->abort == 1) { 
		fprintf(xgp->errout,"%s: xdd_target_init_start_worker_threads: ERROR: Target thread aborting due to previous initialization failure for target number %d name '%s'\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		fflush(xgp->errout);
		return(-1);
	    }
	    // Get next WorkerThread pointer
	    wdp = wdp->wd_next_wdp;
	} // End of FOR loop that starts all worker_threads for this target

	if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(xgp->errout,"\n%s: xdd_target_init_start_worker_threads: Target %d ALL %d WorkerThreads started\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_queue_depth);
	}

	return(0);

} // End of xdd_target_init_start_worker_threads()

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
