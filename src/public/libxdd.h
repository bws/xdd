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
#ifndef LIBXDD_H
#define LIBXDD_H

#include <sys/types.h>
#include "xdd_types.h"
/**
 * A quick overview of how to use this interface.  More complicated things are possible,
 * but this is the most basic interaction:
 *   1.  Create at least 2 targetattrs, and set the attributes to create an IN and OUT
 *       target
 *   2.  Create plan attributes to set the access size
 *   3.  Create the plan from the array of target attributes and the plan attributes
 *   4.  Start the plan
 *   5.  Wait for plan completion
 *
 * Todo:  Retrieve results output from the plan
 */

int xdd_targetattr_init(xdd_targetattr_t *attr);

int xdd_targetattr_destroy(xdd_targetattr_t *attr);

size_t xdd_targetattr_get_length(xdd_targetattr_t attr);

const char* xdd_targetattr_get_uri(xdd_targetattr_t attr);

int xdd_targetattr_set_type(xdd_targetattr_t *attr, xdd_target_type_t);

int xdd_targetattr_set_uri(xdd_targetattr_t *attr, char* uri);

int xdd_targetattr_set_dio(xdd_targetattr_t *attr, int dio_flag);

int xdd_targetattr_set_start_offset(xdd_targetattr_t *attr, off_t off);

int xdd_targetattr_set_length(xdd_targetattr_t *attr, size_t length);

int xdd_targetattr_set_num_threads(xdd_targetattr_t *attr, size_t nthreads);

int xdd_planattr_init(xdd_planattr_t* pattr);

int xdd_planattr_destroy(xdd_planattr_t* pattr);

int xdd_planattr_set_block_size(xdd_planattr_t* pattr, size_t block_size);

int xdd_planattr_set_request_size(xdd_planattr_t* pattr, size_t request_size);

int xdd_planattr_set_retry_flag(xdd_planattr_t* pattr, int retry_flag);

int xdd_plan_init(xdd_planpub_t* plan, xdd_targetattr_t* tattrs, size_t ntattrs, xdd_planattr_t pattr);

int xdd_plan_destroy(xdd_planpub_t* plan);

int xdd_plan_start(const xdd_planpub_t* plan);

int xdd_plan_wait(const xdd_planpub_t* plan);

#endif
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
