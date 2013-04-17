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
#include "libxdd.h"
#include "xdd-lite.h"

int print_usage() {
    xint_lite_print_usage();
}

/** Main */
int main(int argc, char** argv) {

    int rc;
    size_t num_targets;
    xdd_lite_options_t opts;
    xdd_plan_t lite_plan;

    /* Initialize and parse options */
    rc = xint_lite_options_init(&opts);    
    if (0 == rc) {
        rc += xint_lite_options_parse(&opts, argc, argv);
    } else {
        goto cleanup_options;
    }
    if (0 != rc || 1 == opts->help_flag) {
        print_usage();
        goto cleanup_options;
    }

    /* Construct a plan from the specified options */
    rc += xdd_plan_init(&lite_plan);
    rc += xint_lite_options_plan_create(&opts, &lite_plan);

    /* Execute the plan */
    rc += xdd_plan_start(&lite_plan);
    rc += xdd_plan_wait(&lite_plan);

    /* Perform cleanup */
  cleanup_plan:
    xdd_plan_destroy(&lite_plan);
  cleanup_options:
    xint_light_options_destroy(&opts);
    
    return rc;
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
