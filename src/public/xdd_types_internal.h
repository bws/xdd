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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
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

struct xdd_target {
    struct xdd_target_attributes attr;
    
};

struct xdd_plan_attributes {
    size_t block_size;
    size_t request_size;
    int retry_flag;
};

struct xdd_plan_pub {
    struct xdd_plan_attributes attr;
    size_t num_targets;
    struct xdd_target *targets;

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
