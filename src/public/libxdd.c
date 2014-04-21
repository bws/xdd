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
#include "libxdd.h"
#include <assert.h>
#include <string.h>
#define XDDMAIN
#include "xdd_types_internal.h"
#include "xint.h"

/* Forward declarations */
static int add_targets_to_plan(xdd_plan_t *planp,
							   struct xdd_target_attributes **tattrs,
							   size_t ntattr,
							   struct xdd_plan_attributes *pattr);
static int local_target_init(target_data_t *tdp,
							 size_t target_idx,
							 struct xdd_target_attributes *tattr,
							 struct xdd_plan_attributes *pattr);

int xdd_targetattr_init(xdd_targetattr_t *attr) {
    struct xdd_target_attributes *p = calloc(1, sizeof(*p));
	if (NULL == p)
		return 1;
    (*attr) = p;
    return 0;
}

int xdd_targetattr_destroy(xdd_targetattr_t *attr) {
    free (*attr);
    return 0;
}

size_t xdd_targetattr_get_length(xdd_targetattr_t attr) {
    return attr->length;
}

const char* xdd_targetattr_get_uri(xdd_targetattr_t attr) {
    return attr->uri;
}

int xdd_targetattr_set_type(xdd_targetattr_t *attr, xdd_target_type_t xtt) {
    (*attr)->target_type = xtt;
    return 0;
}

int xdd_targetattr_set_uri(xdd_targetattr_t *attr, char* uri) {
    strncpy((*attr)->uri, uri, 2047);
    (*attr)->uri[2047] = '\0';
    return 0;
}

int xdd_targetattr_set_dio(xdd_targetattr_t *attr, int dio_flag) {
    (*attr)->u.in.dio_flag = dio_flag;
    return 0;
}

int xdd_targetattr_set_length(xdd_targetattr_t *attr, size_t length) {
    (*attr)->length = length;
    return 0;
}

int xdd_targetattr_set_num_threads(xdd_targetattr_t *attr, size_t nthreads) {
    (*attr)->num_threads = nthreads;
    return 0;
}

int xdd_targetattr_set_start_offset(xdd_targetattr_t *attr, off_t off) {
    (*attr)->u.in.start_offset = off;
    return 0;
}

int xdd_planattr_init(xdd_planattr_t* pattr) {
    struct xdd_plan_attributes* xpa = calloc(1, sizeof(*xpa));
	if (NULL == xpa)
		return 1;
    (*pattr) = xpa;
    return 0;
}

int xdd_planattr_destroy(xdd_planattr_t* pattr) {
    free(*pattr);
    return 0;
}

int xdd_planattr_set_block_size(xdd_planattr_t* pattr, size_t block_size) {
    (*pattr)->block_size = block_size;
    return 0;
}

int xdd_planattr_set_request_size(xdd_planattr_t* pattr, size_t request_size) {
    (*pattr)->request_size = request_size;
    return 1;
}

int xdd_planattr_set_retry_flag(xdd_planattr_t* pattr, int retry_flag) {
    (*pattr)->retry_flag = retry_flag;
    return 1;
}

int xdd_plan_init(xdd_planpub_t* plan, xdd_targetattr_t* tattrs, size_t ntattr, xdd_planattr_t pattr) {
    int rc = 0;
    struct xint_plan *private_planp;
    xdd_occupant_t barrier_occupant;

    // Initialize the global data
    xint_global_data_initialization("libxdd");
    if (0 == xgp) {
        return 1;
    }

    // Initialize the internal plan type
    private_planp = xint_plan_data_initialization();
    if (0 == private_planp) {
        return 1;
    }

	// Initialize the plan's barrier chain
	xdd_init_barrier_chain(private_planp);

	// Initialize the barrier occupant
	memset(&barrier_occupant, 1, sizeof(barrier_occupant));

	// Add the targets to the plan
	rc = add_targets_to_plan(private_planp, tattrs, ntattr, pattr);
	if (0 != rc) {
		xdd_destroy_all_barriers(private_planp);
		return 1;
	}
	
	// Allocate the plan
	struct xdd_plan_pub *tmp = calloc(1, sizeof(*tmp));
	if (0 == tmp) {
		xdd_destroy_all_barriers(private_planp);
		return 1;
	}
	
    // Copy the plan data into the opaque public plan
	tmp->data = private_planp;
	tmp->occupant = barrier_occupant;
    (*plan) = tmp;
    return rc;
}

int xdd_plan_destroy(xdd_planpub_t* plan) {
    xdd_destroy_all_barriers((*plan)->data);
    free(*plan);
    return 0;
}

int xdd_plan_start(const xdd_planpub_t* plan) {
    // Start all of the threads
    int rc = xint_plan_start((*plan)->data, &(*plan)->occupant);
    if (0 != rc) {
        xdd_destroy_all_barriers((*plan)->data);
        return 1;
    }
    return rc;
}

int xdd_plan_wait(const xdd_planpub_t* plan) {
    int rc = 0;
    xdd_plan_t* planp = (*plan)->data;

    // Wait for the results manager, which signals completion
    xdd_barrier(&planp->main_results_final_barrier,
                &(*plan)->occupant, 1);
    return rc;
}

int add_targets_to_plan(xdd_plan_t *planp,
						struct xdd_target_attributes **tattrs,
						size_t ntattr,
						struct xdd_plan_attributes *pattr) {
	int rc = 0;
	size_t num_iothreads = 0;
	
	// Create all the targets in an array
	target_data_t* target_array = 0;
	if (ntattr > 0) {
		target_array = calloc(ntattr, sizeof(*target_array));
	}
	
	// Process the targets in order
	size_t max_buffers_req = 0;
	for (size_t i = 0; i < ntattr; i++) {
		struct xdd_target_attributes* tap = *(tattrs + i);
		target_data_t *tdp = target_array + i;

		// Initialize/set the target data
		rc = local_target_init(tdp, i, tap, pattr);
		if (0 != rc) {
			free(target_array);
			return 1;
		}
		
		// Determine if this target needs more buffers
		if (max_buffers_req < tap->num_threads)
			max_buffers_req = tap->num_threads;

		// Accumulate the total number of iothreads
		num_iothreads += tap->num_threads;
	}

	// Add the fully constructed targets to the plan
	planp->number_of_iothreads = num_iothreads;
	planp->number_of_targets = ntattr;
	for (size_t i = 0; i < (size_t)planp->number_of_targets; i++) {
		planp->target_datap[i] = target_array + i;

		// Allocate space for the results
		planp->target_average_resultsp[i] = malloc(sizeof(results_t));
	}
	return rc;
}

int local_target_init(target_data_t *tdp,
					  size_t target_idx,
					  struct xdd_target_attributes* tattr,
					  struct xdd_plan_attributes* pattr) {
	worker_data_t *prev_wdp = NULL;

	// Zero out the target data
	memset(tdp, 0, sizeof(*tdp));

	// Allocate space to hold the target name
	size_t fname_len = strlen(tattr->uri) + 1;
	char *fname = malloc(fname_len);
	strncpy(fname, tattr->uri, fname_len);
	
	// Setup a data pattern and e2e buffer before initialization
	tdp->td_dpp = malloc(sizeof(*tdp->td_dpp));
	xdd_data_pattern_init(tdp->td_dpp);
	tdp->td_e2ep = xdd_get_e2ep();
	
	// Now initialize the target data
	xdd_init_new_target_data(tdp, target_idx);
	
	// Modify the target data according to user attributes
	if (XDD_IN_TARGET_TYPE == tattr->target_type)
		tdp->td_rwratio = 1.0;
	else if (XDD_OUT_TARGET_TYPE == tattr->target_type)
		tdp->td_rwratio = 0.0;
	else
		tdp->td_rwratio = -1.0;
	
	tdp->td_target_basename = fname;
	tdp->td_queue_depth = tattr->num_threads;
	tdp->td_block_size = pattr->block_size;
	tdp->td_reqsize = pattr->request_size;
	tdp->td_bytes = tattr->length;
	tdp->td_start_offset = tattr->u.in.start_offset;
	if (tattr->u.in.dio_flag)
		tdp->td_target_options |= TO_DIO;
	
	// TODO: Modify the e2e fields
		
	// Allow the target data to calculate some internal fields
	xdd_calculate_xfer_info(tdp);
		
	// Add worker data structs to the target
	for (size_t i = 0; i < (size_t)tdp->td_queue_depth; i++) {
		worker_data_t *wdp = xdd_create_worker_data(tdp, i);
		if (0 == i) {
			tdp->td_next_wdp = wdp;
		} else {
			prev_wdp->wd_next_wdp = wdp;
		}
		prev_wdp = wdp;
	}
	return 0;
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
