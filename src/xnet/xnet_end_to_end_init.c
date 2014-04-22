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
 * This file contains the subroutines necessary to perform initialization
 * for the end-to-end option.
 */
#include "xint.h"
#include "xni.h"


/*----------------------------------------------------------------------*/
/* xdd_e2e_target_init() - init socket library
 * This routine is called during target initialization to initialize the
 * socket library.
 *
 * Return values: 0 is good, -1 is bad
 *
 */
int32_t xint_e2e_xni_init(target_data_t *tdp) {
	int rc = 0;
	/* Create the XNI control block */
	size_t num_threads = tdp->td_planp->number_of_iothreads;
	if (xni_protocol_tcp == tdp->xni_pcl)
		rc = xni_allocate_tcp_control_block(num_threads,
											tdp->xni_tcp_congestion,
											&tdp->xni_cb);
#if HAVE_ENABLE_IB
	else if (xni_protocol_ib == tdp->xni_pcl)
		rc = xni_allocate_ib_control_block(tdp->xni_ibdevice,
										   num_threads,
										   &tdp->xni_cb);
#endif  /* HAVE_ENABLE_IB */
	else
		return -1;
	assert(0 == rc);
	
	/* Create the XNI context */
	rc = xni_context_create(tdp->xni_pcl, tdp->xni_cb, &tdp->xni_ctx);
	assert(0 == rc);
	return(0);
}

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
