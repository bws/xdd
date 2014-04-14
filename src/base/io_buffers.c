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
 * This file contains the subroutines that allocate and initialize the io buffers
 * used by the Worker Threads.
 */
#include "xint.h"
#ifdef DARWIN
#include <sys/shm.h>
#endif

/*----------------------------------------------------------------------------*/
/* xdd_init_io_buffers() - set up the I/O buffers
 * This routine will allocate the memory used as the I/O buffer for a Worker
 * Thread. The pointer to the buffer (wd_bufp) and the size of the buffer 
 * (wd_buf_size) are set in the Worker Data Struct.
 *
 * This routine will return the pointer to the buffer upon success. If for
 * some reason the buffer cannot be allocated then NULL is returned. 
 *
 * For some operating systems, you can use a shared memory segment instead of 
 * a normal malloc/valloc memory chunk. This is done using the "-sharedmemory"
 * command line option.
 *
 * The size of the buffer depends on whether it is being used for network
 * I/O as in an End-to-end operation. For End-to-End operations, the size
 * of the buffer is 1 page larger than for non-End-to-End operations.
 *
 * For normal (non-E2E operations) the buffer pointers are as follows:
 *                   |<----------- wd_buf_size = N Pages ----------------->|
 *	                 +-----------------------------------------------------+
 *	                 |  data buffer                                        |
 *	                 |  transfer size (td_xfer_size) rounded up to N pages |
 *	                 |<-wd_bufp                                            |
 *	                 |<-task_datap                                         |
 *	                 +-----------------------------------------------------+
 *
 * For End-to-End operations, the buffer pointers are as follows:
 *  |<------------------- wd_buf_size = N+1 Pages ------------------------>|
 *	+----------------+-----------------------------------------------------+
 *	|<----1 page---->|  transfer size (td_xfer_size) rounded up to N pages |
 *	|<-wd_bufp       |<-task_datap                                         |
 *	|     |   E2E    |      E2E                                            |
 *	|     |<-Header->|   data buffer                                       |
 *	+-----*----------*-----------------------------------------------------+
 *	      ^          ^
 *	      ^          +-e2e_datap     
 *	      +-e2e_hdrp 
 */
unsigned char *
xdd_init_io_buffers(worker_data_t *wdp) {
	target_data_t		*tdp;			// Pointer to Target Data
	unsigned char 		*bufp;			// Generic Buffer Pointer
	void 				*shmat_status;	// Status of shmat()
	int 				buf_shmid;		// Shared Memory ID
	int					buffer_size;	// Size of buffer in bytes
	int					page_size;		// Size of a page of memory
	int					pages;			// Size of buffer in pages
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
#endif

	tdp = wdp->wd_tdp;
	wdp->wd_bufp = NULL;
	wdp->wd_buf_size = 0;

	// Calaculate the number of pages needed for a buffer
	page_size = getpagesize();
	pages = tdp->td_xfer_size / page_size;
	if (tdp->td_xfer_size % page_size)
		pages++; // Round up to page size
	if ((tdp->td_target_options & TO_ENDTOEND)) {
		// Add one page for the e2e header
		pages++; 

		// If its XNI, add another page for XNI, better would be for XNI to
		// pack all of the header data (and do the hton, ntoh calls)
		xdd_plan_t *planp = tdp->td_planp;
		if (PLAN_ENABLE_XNI & planp->plan_options) {
			pages++;
		}
	}

	
	// This is the actual size of the I/O buffer
	buffer_size = pages * page_size;

	/* Check to see if we want to use a shared memory segment and allocate it using shmget() and shmat().
	 * NOTE: This is not supported by all operating systems. 
	 */
	if (tdp->td_target_options & TO_SHARED_MEMORY) {
#if (AIX || LINUX || SOLARIS || DARWIN || FREEBSD)
	    /* In AIX we need to get memory in a shared memory segment to avoid
	     * the system continually trying to pin each page on every I/O operation */
#if (AIX)
		buf_shmid = shmget(IPC_PRIVATE, buffer_size, IPC_CREAT | SHM_LGPAGE |SHM_PIN );
#else
		buf_shmid = shmget(IPC_PRIVATE, buffer_size, IPC_CREAT );
#endif
		if (buf_shmid < 0) {
			fprintf(xgp->errout,"%s: Cannot create shared memory segment\n", xgp->progname);
			perror("Reason");
			bufp = 0;
			buf_shmid = -1;
		} else {
			shmat_status = (void *)shmat(buf_shmid,NULL,0);
			if (shmat_status == (void *)-1) {
				fprintf(xgp->errout,"%s: Cannot attach to shared memory segment\n",xgp->progname);
				perror("Reason");
				bufp = 0;
				buf_shmid = -1;
			}
			else bufp = (unsigned char *)shmat_status;
		}
		if (xgp->global_options & GO_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID allocated and attached, shmid=%d\n",buf_shmid);
#elif (IRIX || WIN32 )
		fprintf(xgp->errout,"%s: Shared Memory not supported on this OS - using valloc\n",
			xgp->progname);
		tdp->td_target_options &= ~TO_SHARED_MEMORY;
#if (IRIX || SOLARIS || LINUX || AIX || DARWIN || FREEBSD)
		bufp = valloc(buffer_size);
#else
		bufp = malloc(buffer_size);
#endif
#endif 
	} else { /* Allocate memory the normal way */
#if (AIX || LINUX)
		posix_memalign((void **)&bufp, sysconf(_SC_PAGESIZE), buffer_size);
#elif (IRIX || SOLARIS || LINUX || DARWIN || FREEBSD)
		bufp = valloc(buffer_size);
#else
		bufp = malloc(buffer_size);
#endif
	}
	/* Check to see if we really allocated some memory */
	if (bufp == NULL) {
		fprintf(xgp->errout,"%s: cannot allocate %d bytes of memory for I/O buffer\n",
			xgp->progname,buffer_size);
		fflush(xgp->errout);
#ifdef WIN32 
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"Reason:%s",lpMsgBuf);
		fflush(xgp->errout);
#else 
		perror("Reason");
#endif
		return(NULL);
	}
	/* Memory allocation must have succeeded */

	/* Lock all pages in memory */
	xdd_lock_memory(bufp, buffer_size, "RW BUFFER");

	wdp->wd_bufp = bufp;
	wdp->wd_buf_size = buffer_size;

	return(bufp);
} /* end of xdd_init_io_buffers() */

 
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
