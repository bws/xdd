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
 *       Bradly Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines that perform various initialization 
 * functions when xdd is started.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_io_buffers() - set up the I/O buffers
 * This routine will allocate the memory used as the I/O buffer for a target.
 * For some operating systems, you can use a shared memory segment instead of 
 * a normal malloc/valloc memory chunk. This is done using the "-sharedmemory"
 * command line option.
 */
unsigned char *
xdd_init_io_buffers(ptds_t *p) {
	unsigned char *rwbuf; /* the read/write buffer for this op */
	void *shmat_status;
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
#endif



	/* allocate slightly larger buffer for meta data for end-to-end ops */
	if ((p->target_options & TO_ENDTOEND)) 
		p->e2e_iosize = p->iosize + sizeof(p->e2e_msg);
	else	p->e2e_iosize = p->iosize;
	/* Check to see if we want to use a shared memory segment and allocate it using shmget() and shmat().
	 * NOTE: This is not supported by all operating systems. 
	 */
	if (p->target_options & TO_SHARED_MEMORY) {
#if (AIX || LINUX || SOLARIS || OSX || FREEBSD)
		/* In AIX we need to get memory in a shared memory segment to avoid
	     * the system continually trying to pin each page on every I/O operation */
#if (AIX)
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->e2e_iosize, IPC_CREAT | SHM_LGPAGE |SHM_PIN );
#else
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->e2e_iosize, IPC_CREAT );
#endif
		if (p->rwbuf_shmid < 0) {
			fprintf(xgp->errout,"%s: Cannot create shared memory segment\n", xgp->progname);
			perror("Reason");
			rwbuf = 0;
			p->rwbuf_shmid = -1;
		} else {
			shmat_status = (void *)shmat(p->rwbuf_shmid,NULL,0);
			if (shmat_status == (void *)-1) {
				fprintf(xgp->errout,"%s: Cannot attach to shared memory segment\n",xgp->progname);
				perror("Reason");
				rwbuf = 0;
				p->rwbuf_shmid = -1;
			}
			else rwbuf = (unsigned char *)shmat_status;
		}
		if (xgp->global_options & GO_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID allocated and attached, shmid=%d\n",p->rwbuf_shmid);
#elif (IRIX || WIN32 )
		fprintf(xgp->errout,"%s: Shared Memory not supported on this OS - using valloc\n",
			xgp->progname);
		p->target_options &= ~TO_SHARED_MEMORY;
#if (IRIX || SOLARIS || LINUX || AIX || OSX || FREEBSD)
		rwbuf = valloc(p->e2e_iosize);
#else
		rwbuf = malloc(p->e2e_iosize);
#endif
#endif 
	} else { /* Allocate memory the normal way */
#if (IRIX || SOLARIS || LINUX || AIX || OSX || FREEBSD)
		rwbuf = valloc(p->e2e_iosize);
#else
		rwbuf = malloc(p->e2e_iosize);
#endif
	}
	/* Check to see if we really allocated some memory */
	if (rwbuf == NULL) {
		fprintf(xgp->errout,"%s: cannot allocate %d bytes of memory for I/O buffer\n",
			xgp->progname,p->e2e_iosize);
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

	/* Lock all rwbuf pages in memory */
	xdd_lock_memory(rwbuf, p->e2e_iosize, "RW BUFFER");

	return(rwbuf);
} /* end of xdd_init_io_buffers() */

 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
