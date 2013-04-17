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
 *       Steve Hodson, DoE/ORNL
 *       Steve Poole, DoE/ORNL
 *       Brad Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_target_init() - Initialize a Target Thread
 * This subroutine will open the target file and perform some initial sanity 
 * checks. 
 * 
 * It will then start all QThreads that are responsible for doing the actual I/O.
 *
 * It is important to note that upon entering this subroutine, the PTDS substructure
 * has already been built. This routine will simply create each QThread and 
 * pass it the address of the PTDS that unique to each QThread.
 * The PTDS that each QThread receives has its target number and QThread number.
 * 
 * The QThreadIssue threads is started after all QThreads
 * have been successfully initialized. As part of the QThread initialization
 * routine each QThread will place itself on the QThreadAvailable Queue.
 *
 * Return Values: 0 is good, -1 indicates an error.
 * 
 */
int32_t
xdd_target_init(ptds_t *p) {
	int32_t		status;			// Status of function calls
	nclk_t		CurrentLocalTime;	// Used the init the Global Clock
	nclk_t		TimeDelta;		// Used the init the Global Clock
	uint32_t	sleepseconds;
	

#if (AIX)
	p->my_thread_id = thread_self();
#elif (LINUX)
	p->my_thread_id = syscall(SYS_gettid);
#else
	p->my_thread_id = p->my_pid;
#endif

	// Set the pass number
	p->my_current_pass_number = 1;

	// Check to see that the target is valid and can be opened properly
	status = xdd_target_open(p);
	if (status) 
		return(-1);

	/* Perform preallocation if needed */
	xdd_target_preallocate(p);
        
	// The Seek List
	// There is one Seek List per target.
	// This list contains all the locations in a implied order that need to be accessed
	// during a single pass. 
	// It is this list that is used by xdd_issue() to assign I/O tasks to the QThreads.
	//
	p->seekhdr.seek_total_ops = p->target_ops;
	p->seekhdr.seeks = (seek_t *)calloc((int32_t)p->seekhdr.seek_total_ops,sizeof(seek_t));
	if (p->seekhdr.seeks == 0) {
		fprintf(xgp->errout,"%s: xdd_target_thread_init: ERROR: Cannot allocate memory for access list for Target %d name '%s' - terminating\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		xgp->abort = 1;
		return(-1);
	}

	xdd_init_seek_list(p);

	// Set up the timestamp table - Note: This must be done *after* the seek list is initialized
	xdd_ts_setup(p); 

	// Set up for the big loop 
	if (xgp->max_errors == 0) 
		xgp->max_errors = p->target_ops;

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
	status = xdd_lockstep_init(p); 
	if (status != XDD_RC_GOOD)
		return(status);
	
	// Initialize the barriers and mutex
	status = xdd_target_init_barriers(p);
	if (status) 
	    return(-1);
	
	// Initialize the Target Offset Table
	status = tot_init(&(p->totp), p->queue_depth, p->target_ops);
	if (status) {
	    return(-1);
	}
	

	// Special setup for an End-to-End operation
	if (p->target_options & TO_ENDTOEND) {
		status = xdd_e2e_target_init(p);
		if (status)
			return(-1);
	}

	// Start the QThreads
	status = xdd_target_init_start_qthreads(p);
	if (status) 
		return(-1);

	// Display the information for this target
	xdd_target_info(xgp->output, p);
	if (xgp->csvoutput)
		xdd_target_info(xgp->csvoutput, p);

	return(0);
} // End of xdd_target_init()

/*----------------------------------------------------------------------------*/
/* xdd_target_init_barriers() - Initialize the barriers and mutex
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
xdd_target_init_barriers(ptds_t *p) {
    int32_t		status;								// Status of subroutine calls
    char		tmpname[XDD_BARRIER_NAME_LENGTH];	// Used to create unique names for the barriers


	// The following initialization calls are all done with accumulated status for ease of coding.
	// Normal/good status from xdd_init_barrier() is zero so if everything initializes properly then
	// the accumulated status should be zero. Any barrier that is not properly initialized is cause
	// for exiting XDD altogether. 
	status = 0;

	// The Target_QThread Initialization barrier
	sprintf(tmpname,"T%04d:target_qthread_init_barrier",p->my_target_number);
	status += xdd_init_barrier(p->my_planp, &p->target_qthread_init_barrier,2, tmpname);

	// The Target Pass barrier
	sprintf(tmpname,"T%04d>targetpass_qthread_passcomplete_barrier",p->my_target_number);
	status += xdd_init_barrier(p->my_planp, &p->targetpass_qthread_passcomplete_barrier,p->queue_depth+1,tmpname);

	// The Target Pass E2E EOF Complete barrier - only initialized when an End-to-End operation is running
	if (p->target_options & TO_ENDTOEND) {
		sprintf(tmpname,"T%04d>targetpass_qthread_eofcomplete_barrier",p->my_target_number);
		status += xdd_init_barrier(p->my_planp, &p->targetpass_qthread_eofcomplete_barrier,2,tmpname);
	}

	// The Target Start Trigger barrier 
	if (p->target_options & TO_WAITFORSTART) { // If we are expecting a Start Trigger then we need to init the starttrigger barrier
		sprintf(tmpname,"T%04d>target_target_starttrigger_barrier",p->my_target_number);
		status += xdd_init_barrier(p->my_planp, &p->trigp->target_target_starttrigger_barrier,2,tmpname);
	}

	// The "counter_mutex" is used by the QThreads when updating the counter information in the Target Thread PTDS
	status += pthread_mutex_init(&p->counter_mutex, 0);

	if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_barriers: ERROR: Cannot create barriers/mutex for target number %d name '%s'\n",
			xgp->progname, 
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		return(-1);
	}

	// Initialize the semaphores used to control QThread selection
	p->any_qthread_available = 0;
	status = pthread_mutex_init(&p->any_qthread_available_mutex, NULL);
	status = pthread_cond_init(&p->any_qthread_available_condition, NULL);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_barriers: Target %d: ERROR: Cannot initialize any_qthread_available condvar.\n",
			xgp->progname, 
			p->my_target_number);
		fflush(xgp->errout);
		return(-1);
	}

	return(0);
} // End of xdd_target_init_barriers()

/*----------------------------------------------------------------------------*/
/* xdd_target_thread_init_start_qthreads() - Start up all QThreads 
 *
 * The actual QThread routine called xdd_qthread() is located in qthread.c 
 *
 * Return value is 0 if everything succeeded or -1 if there was a failure.
 */
int32_t
xdd_target_init_start_qthreads(ptds_t *p) {
	ptds_t		*qp;					// Pointer to the QThread PTDS
	int32_t		q;						// QThread Number
	int32_t		status;					// Status of subtroutine calls
	int32_t		e2e_addr_index;			// index into the e2e address table
	int32_t		e2e_addr_port;			// Port number in the e2e address table

	
	// Now let's start up all the QThreads for this target
	qp = p->next_qp; // This is the first QThread
	e2e_addr_index = 0;
	e2e_addr_port = 0;
	for (q = 0; q < p->queue_depth; q++ ) {
	    pthread_attr_t qthread_attr;

	    // Initialize the attributes
	    pthread_attr_init(&qthread_attr);
	    
	    // Start a QThread and wait for it to initialize
	    qp->my_target_number = p->my_target_number;
	    qp->my_qthread_number = q;
	    if (p->target_options & TO_ENDTOEND) {

		// Find an e2e entry that has a valid port count
		while (0 == p->e2e_address_table[e2e_addr_index].port_count) {
		    e2e_addr_index++;
		}
		//assert(e2e_addr_index < p->e2e_address_table->number_of_entries);
		
		qp->e2e_dest_hostname = p->e2e_address_table[e2e_addr_index].hostname;
		qp->e2e_dest_port = p->e2e_address_table[e2e_addr_index].base_port + e2e_addr_port;
		
		// Set the QThread Numa node if possible
#if defined(HAVE_CPU_SET_T) && defined(HAVE_PTHREAD_ATTR_SETAFFINITY_NP)
		status = pthread_attr_setaffinity_np(&qthread_attr,
						     sizeof(p->e2e_address_table[e2e_addr_index].cpu_set),
						     &p->e2e_address_table[e2e_addr_index].cpu_set);
#endif
		// Roll over to the begining of the list if that is required
		e2e_addr_port++;
		if (e2e_addr_port == p->e2e_address_table[e2e_addr_index].port_count) {
		    e2e_addr_index++;
		    e2e_addr_port = 0;
		}
		if (xgp->global_options & GO_REALLYVERBOSE)
		    fprintf(stderr,"Target Init: Target %d: assigning hostname %s port %d to qthread %d\n",
			    p->my_target_number, qp->e2e_dest_hostname,
			    qp->e2e_dest_port, qp->my_qthread_number);
	    }

	    
	    status = pthread_create(&qp->qthread, &qthread_attr, xdd_qthread, qp);
	    if (status) {
		fprintf(xgp->errout,"%s: xdd_target_init_start_qthreads: ERROR: Cannot create qthread %d for target number %d name '%s' - Error number %d\n",
			xgp->progname, 
			q,
			p->my_target_number,
			p->target_full_pathname,
			status);
		fflush(xgp->errout);
		// Set errno to the correct error number so that perror() can display the error message
		errno = status;
		perror("Reason");
		return(-1);
	    }
	    /* Wait for the previous thread to initialize before creating the next one */
	    xdd_barrier(&p->target_qthread_init_barrier,&p->occupant,1);
	    if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(xgp->errout,"\r%s: xdd_target_init_start_qthreads: Target %d QThread %d of %d started\n",
			xgp->progname,
			p->my_target_number,
			q,
			p->queue_depth);
	    }

	    // If the QThread initialization fails, then everything needs to stop right now.
	    if (xgp->abort == 1) { 
		fprintf(xgp->errout,"%s: xdd_target_init_start_qthreads: ERROR: Target thread aborting due to previous initialization failure for target number %d name '%s'\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		return(-1);
	    }
	    // Get next QThread pointer
	    qp = qp->next_qp;
	} // End of FOR loop that starts all qthreads for this target

	if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(xgp->errout,"\n%s: xdd_target_init_start_qthreads: Target %d ALL %d QThreads started\n",
			xgp->progname,
			p->my_target_number,
			p->queue_depth);
	}

	return(0);

} // End of xdd_target_init_start_qthreads()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
