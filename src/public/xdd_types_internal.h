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
#ifndef XDD_TYPES_INTERNAL_H
#define XDD_TYPES_INTERNAL_H

#include "xint.h"

struct xdd_target_attributes {
    xdd_target_type_t target_type;
    char uri[2048];
    size_t num_threads;
	size_t length;
    union {
        struct {
            int dio_flag;
			off_t start_offset;
        } in;
        struct {
            int dio_flag;
			off_t start_offset;
        } out;
        struct {
            int data_pattern;
        } meta;
    } u;
};

struct xdd_plan_attributes {
    size_t block_size;
    size_t request_size;
    int retry_flag;
};

struct xdd_plan_pub {
	// XDD internal data structures
	xdd_plan_t* data;
	xdd_occupant_t occupant;
	
};

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
