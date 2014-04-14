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

/*----------------------------------------------------------------------------*/
/* xint_target_pretruncate() - attempt to preallocate space via ftruncate for 
 * the transfer.
 * If data already exists in the file, don't mess it up.
 * Upon sucessful preallocation this routine will return a 0.
 * Otherwise -1 is returned to indicate an error.
 */
int32_t
xint_target_pretruncate(target_data_t *tdp){
	int32_t		status;		// Status of the preallocate call


	// If this is a NULL target then don't bother with preallocation
	if (tdp->td_target_options & TO_NULL_TARGET)
		return(0);

	// Check to see if a preallocation amount was specified - if not, just return
	if (tdp->td_pretruncate <= 0) 
		return(0);

    /* A regression in XFS means that preallocate fixes the extents, but
       performance is still bad during file extend sequences.  Truncate
       the file to length to improve performance */
	status = ftruncate(tdp->td_file_desc, tdp->td_pretruncate);

	// Status should be 0 if everything worked otherwise there was a problem
	if (status) {
 		fprintf(xgp->errout,"%s: xdd_target_pretruncate: ERROR: Unable to ftruncate space for target %d name %s\n",
			xgp->progname,
			tdp->td_target_number,
			tdp->td_target_full_pathname);
		perror("Reason");
		fflush(xgp->errout);
		return(-1);
	}
	return(0);

} // End of xint_target_pretruncate()

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
