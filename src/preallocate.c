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
 * This file contains the subroutines that perform an "open" operation 
 * on the target device/file.
 */
#include "xdd.h"

#if  LINUX
/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - Preallocate routine for linux
 * This subroutine will check to see if the file system supports the Reserve Space
 * control call.
 * Return value of 0 is good.
 * Return value of 1 more more is bad.
 *
 */
int32_t
xdd_target_preallocate_for_os(ptds_t *p) {
	
#ifdef XFS_ENABLED
	int32_t 	status;		// Status of various system calls
	struct statfs 	sfs;		// File System Information struct
	xfs_flock64_t 	xfs_flock;	// Used to pass preallocation information to xfsctl()
	
	// Determine the file system type
	status = fstatfs(p->fd, &sfs);
	if (0 != status) {
		fprintf(xgp->errout, 
			"%s: xdd_target_preallocatefor_os<LINUX>: ERROR: Target %d name %s: cannot get file system information via statfs\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		perror("Reason");
		fflush(xgp->errout);
		return(1);
	}
	
	/* Support preallocate on XFS */ 
	if (XFS_SUPER_MAGIC == sfs.f_type) {
		int64_t pos,rem;
		const int64_t max_bytes = 1L<<32;

		/* Perform allocation in 4GB chunks to support 16KB alignment */
		pos = 0;
		rem = p->preallocate;
		while (rem > 0) {
			/* Always set whence to 0, for now */
			xfs_flock.l_whence = 0;

			/* Allocate the next 4GB */
			xfs_flock.l_start = pos; 
			xfs_flock.l_len = (rem <= max_bytes) ? rem : max_bytes;
			status = xfsctl(p->target_full_pathname,p->fd, XFS_IOC_RESVSP64, &xfs_flock);

			/* Check preallocate status */
			if (status) { 
				fprintf(xgp->errout, 
					"%s: xdd_target_preallocatefor_os<LINUX>: ERROR: Target %d name %s: xfsctl call for preallocation failed\n",
					xgp->progname,
					p->my_target_number,
					p->target_full_pathname);
				perror("Reason");
				fflush(xgp->errout);
				return(1);
			}
			pos += max_bytes;
			rem -= max_bytes;
		}
	}
		
	// Everything must have worked :)
	return(0); 
#else // XFS is not ENABLED
	fprintf(xgp->errout,
		"%s: xdd_target_preallocate_for_os<LINUX>: ERROR: This program was not compiled with XFS_ENABLED - no preallocation possible\n",
		xgp->progname);
	fflush(xgp->errout);
	return(-1);
#endif // XFS_ENABLED


} // End of Linux xdd_target_preallocate()

#elif AIX

/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - Preallocate routine for AIX
 */
int32_t
xdd_target_preallocate_for_os(ptds_t *p) {

	fprintf(xgp->errout,
		"%s: xdd_target_preallocate_for_os<AIX>: ERROR: Target %d name %s: Preallocation is not supported on AIX\n",
		xgp->progname,
		p->my_target_number,
		p->target_full_pathname);
	fflush(xgp->errout);
	return(-1);
	
} // End of AIX xdd_target_preallocate()

#else 

/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - The default Preallocate routine for all other OS
 */
int32_t
xdd_target_preallocate_for_os(ptds_t *p) {

	fprintf(xgp->errout,
		"%s: xdd_target_preallocate_for_os: ERROR: Target %d name %s: Preallocation is not supported on this OS\n",
		xgp->progname,
		p->my_target_number,
		p->target_full_pathname);
	fflush(xgp->errout);
	return(-1);
	
} // End of default xdd_target_preallocate()
 
#endif

/*----------------------------------------------------------------------------*/
// OS-independent code
/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - attempt to preallocate space for the transfer.
 * If data already exists in the file, don't mess it up.
 * Upon sucessful preallocation this routine will return a 0.
 * Otherwise -1 is returned to indicate an error.
 */
int32_t
xdd_target_preallocate(ptds_t *p){
	int32_t		status;		// Status of the preallocate call


	// Check to see if a preallocation amount was specified - if not, just return
	if (p->preallocate <= 0) 
		return(0);

	status = xdd_target_preallocate_for_os(p);

	// Check the status of the preallocate operation to see if it worked
	if (-1 == status)  // Preallocation not supported
		return(-1);

	// Status should be 0 if everything worked otherwise there was a problem
	if (status) {
 		fprintf(xgp->errout,"%s: xdd_target_preallocate: ERROR: Unable to preallocate space for target %d name %s\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		perror("Reason");
		fflush(xgp->errout);
		return(-1);
	}
	return(0);

} // End of xdd_target_preallocate()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
