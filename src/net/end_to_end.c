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
 * This file contains the subroutines necessary the end-to-end
 * Send and Receive functions. 
 */
#include "xint.h"

/*----------------------------------------------------------------------*/
/* xdd_e2e_eof_source_side() - End-Of-File processing for Source 
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_e2e_eof_source_side(worker_data_t *wdp) {
	target_data_t		*tdp;
	xint_e2e_t			*e2ep;			// Pointer to the E2E struct for this worker

	tdp = wdp->wd_tdp;
	e2ep = wdp->wd_e2ep;

	e2ep->e2e_header_size = 0;
	e2ep->e2e_xfer_size = e2ep->e2e_header_size;

if (xgp->global_options & GO_DEBUG_E2E) fprintf(stderr,"DEBUG_E2E: %lld: xdd_e2e_eof_source_side: Target %d Worker: %d: ENTER: \n", (long long int)pclk_now(), tdp->td_target_number, wdp->wd_worker_number);

    /* If this is XNI, just short circuit */
    if (PLAN_ENABLE_XNI & tdp->td_planp->plan_options) {
		e2ep->e2e_send_status = 0;
		e2ep->e2e_sr_time = 0;
		return 0;
	}

	// we only support XNI now
	return -1;
} /* end of xdd_e2e_eof_source_side() */

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
