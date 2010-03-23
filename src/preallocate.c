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
/* xdd_target_preallocate_for_linux() - Preallocate routine for linux
 */
int32_t
xdd_target_preallocate_for_linux(ptds_t *p, int fd) {

	int rc;
	struct statfs sfs = {0};
	int32_t status = 0;
	
	// Determine the file system type
	rc = fstatfs(fd, &sfs);
	if (0 != rc)
	{
		// FS Check failed
		status++;
		return status;
	}
	
	// Only support clean preallocate on XFS
	int xfs_super_magic = 0x58465342;
	if (xfs_super_magic == sfs.f_fsid.__val[1]) {
		struct flock64 flock = {0};
		flock.l_whence = SEEK_SET;
		flock.l_start = 0;
		flock.l_len = (uint64_t)(p->preallocate * p->block_size);
		rc = fcntl(fd, XFS_IOC_RESVSP64, &flock);
		if (0 != rc)
		{
			// Preallocate failed
			status++;
			return status;
		}
	}
	return status;

} // End of xdd_target_open_for_unix()
#endif 

/*----------------------------------------------------------------------------*/
/* xdd_target_preallocate() - attempt to preallocate space for the transfer.
 * If data already exists in the file, don't mess it up.
 */
int32_t
xdd_target_preallocate(ptds_t *p, int fd) {
	int32_t		status;		// Status of the preallocate call
#if LINUX 
	status = xdd_target_preallocate_for_linux(p, fd);
#else
	status = -1;
#endif

	// Check the status of the preallocate operation to see if it worked
	if (-1 == status) {
		fprintf(xgp->errout,
			"%s (%d): xdd_target_preallocate: Preallocate is only supported on Linux platforms %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}
	if (0 == status) {
            fprintf(xgp->errout,"%s (%d): xdd_target_preallocate: Unable to preallocate space: %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}
	return(0);

} // End of xdd_target_preallocate()

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
