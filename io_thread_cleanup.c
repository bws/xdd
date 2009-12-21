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
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_io_thread() - control of the I/O for the specified reqsize      
 * Notes: Two thread_barriers are used due to an observed race-condition
 * in the xdd_barrier() code, most likely in the semaphore op,
 * that causes some threads to be woken up prematurely and getting the
 * intended synchronization out of sync hanging subsequent threads.
 */
int32_t
xdd_io_thread_cleanup(ptds_t *p) {
	int32_t  	i,j;  	/* k? random variables */
	ptds_t  	*qp;   	/* Pointer to a qthread ptds */
	ptds_t  	*tp;   	/* Pointer to a target ptds */
	char errmsg[256];
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
	HANDLE tmphandle;
#endif


	if (xgp->global_options & GO_DEBUG) {
		fprintf(xgp->errout,"io_thread_cleanup[mythreadnum %d]:Starting cleanup\n",p->mythreadnum);
	}

	// Close target file and delete if requested
	if ((p->target_options & TO_DELETEFILE) && (
		 p->target_options & TO_REGULARFILE)) { // Close the file and delete it
#ifdef WIN32
		CloseHandle(p->fd); // Need to close the file first
		DeleteFile(p->target);
#else
		unlink(p->target);
#endif
		fprintf(xgp->errout,"%s: File %s removed\n",xgp->progname,p->target);
		fflush(xgp->errout);
	}
	else { // Just close the file
#if (WIN32)
	CloseHandle(p->fd);
#else
	close(p->fd);
#endif
	} // Done closing files

	// Close any network sockets 
	if ((p->rawp) && (p->target_options & TO_READAFTERWRITE)) {
		if (xgp->global_options & GO_REALLYVERBOSE)
			fprintf(stderr,"closing socket %d, recvd msgs = %d sent msgs = %d\n",p->rawp->raw_sd, p->rawp->raw_msg_recv, p->rawp->raw_msg_sent);
		close(p->rawp->raw_sd);
	}

	// Now let's get rid of the memory buffers and skidaddle on out of here
	for (i = 0; i < p->queue_depth; i++) {
		xdd_unlock_memory(p->rwbuf_save, p->iosize, "IOBuffer");
	}
#if (AIX)
	if (p->target_options & TO_SHARED_MEMORY) {
		if (shmctl(p->rwbuf_shmid, IPC_RMID, NULL)  == -1)
			fprintf(xgp->errout,"%s: Cannot Remove SHMID for Target %d Q %d\n",xgp->progname, p->my_target_number, p->my_qthread_number);
		else if (xgp->global_options & GO_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID removed, shmid=%d\n",p->rwbuf_shmid);
	}
#endif
	// xdd_system_info(stderr);	

	// Wait for all other threads to finish cleaning up before leaving
	xdd_barrier(&xgp->cleanup_barrier);

    return(0);
} /* end of xdd_io_thread() */
