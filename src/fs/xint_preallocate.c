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
/*
 * This file contains the subroutines that perform an "open" operation 
 * on the target device/file.
 */
#include "xint.h"

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
xint_target_preallocate_for_os(target_data_t *tdp) {
	
#ifdef HAVE_ENABLE_XFS
	int32_t 	status;		// Status of various system calls
	struct statfs 	sfs;		// File System Information struct
	xfs_flock64_t 	xfs_flock;	// Used to pass preallocation information to xfsctl()
	
	// Determine the file system type
	status = fstatfs(tdp->td_file_desc, &sfs);
	if (0 != status) {
		fprintf(xgp->errout, 
				"%s: xdd_target_preallocatefor_os<LINUX>: ERROR: Target %d name %s: cannot get file system information via statfs\n",
				xgp->progname,
				tdp->td_target_number,
				tdp->td_target_full_pathname);
		perror("Reason");
		fflush(xgp->errout);
		return(1);
	}
	
	/* Support preallocate on XFS */ 
	if (XFS_SUPER_MAGIC == sfs.f_type) {
		/* On non 64-bit systems, preallocate may only work 4GB at a time */
		if (8 == sizeof(int*)) {
			/* Always set whence to 0, for now */
			xfs_flock.l_whence = 0;
				
			/* Allocate the entire file */
			xfs_flock.l_start = 0; 
			xfs_flock.l_len = tdp->td_preallocate;
			status = xfsctl(tdp->td_target_full_pathname,
							tdp->td_file_desc,
							XFS_IOC_RESVSP64, &xfs_flock);
		}
		else {
			/* Perform allocation in 4GB chunks to support 16KB alignment */
			const int64_t max_bytes = 1L<<32;
			int64_t pos = 0;
			int64_t rem = tdp->td_preallocate;
			while (rem > 0) {
				/* Always set whence to 0, for now */
				xfs_flock.l_whence = 0;
				
				/* Allocate the next 4GB */
				xfs_flock.l_start = pos; 
				xfs_flock.l_len = (rem <= max_bytes) ? rem : max_bytes;
				status = xfsctl(tdp->td_target_full_pathname,
								tdp->td_file_desc,
								XFS_IOC_RESVSP64, &xfs_flock);
			
				/* Check preallocate status */
				if (status) { 
					break;
				}
				pos += max_bytes;
				rem -= max_bytes;
			}
		}
		if (status) {
			fprintf(xgp->errout, 
					"%s: xdd_target_preallocatefor_os<LINUX>: ERROR: Target %d name %s: xfsctl call for preallocation failed\n",
					xgp->progname,
					tdp->td_target_number,
					tdp->td_target_full_pathname);
			perror("Reason");
			fflush(xgp->errout);
			return(1);
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


} // End of Linux xint_target_preallocate()

#elif AIX

/*----------------------------------------------------------------------------*/
/* xint_target_preallocate() - Preallocate routine for AIX
 */
int32_t
xint_target_preallocate_for_os(target_data_t *tdp) {

	fprintf(xgp->errout,
		"%s: xdd_target_preallocate_for_os<AIX>: ERROR: Target %d name %s: Preallocation is not supported on AIX\n",
		xgp->progname,
		tdp->td_target_number,
		tdp->td_target_full_pathname);
	fflush(xgp->errout);
	return(-1);
	
} // End of AIX xint_target_preallocate()

#else 

/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - The default Preallocate routine for all other OS
 */
int32_t
xint_target_preallocate_for_os(target_data_t *tdp) {

	fprintf(xgp->errout,
		"%s: xdd_target_preallocate_for_os: ERROR: Target %d name %s: Preallocation is not supported on this OS\n",
		xgp->progname,
		tdp->td_target_number,
		tdp->td_target_full_pathname);
	fflush(xgp->errout);
	return(-1);
	
} // End of default xint_target_preallocate()
 
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
xint_target_preallocate(target_data_t *tdp){
	int32_t		status;		// Status of the preallocate call


	// If this is a NULL target then don't bother with preallocation
	if (tdp->td_target_options & TO_NULL_TARGET)
		return(0);

	// Check to see if a preallocation amount was specified - if not, just return
	if (tdp->td_preallocate <= 0) 
		return(0);

	status = xint_target_preallocate_for_os(tdp);

	// Check the status of the preallocate operation to see if it worked
	if (-1 == status)  // Preallocation not supported
		return(-1);

	// Status should be 0 if everything worked otherwise there was a problem
	if (status) {
 		fprintf(xgp->errout,"%s: xdd_target_preallocate: ERROR: Unable to preallocate space for target %d name %s\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		perror("Reason");
		fflush(xgp->errout);
		return(-1);
	}
	return(0);

} // End of xint_target_preallocate()

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
