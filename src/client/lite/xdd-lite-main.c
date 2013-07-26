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
#include <stdio.h>
#include <string.h>
#include "libxdd.h"
#include "xdd-lite.h"
#include "xdd-lite-forking-server.h"

/** Print out the CLI usage information */
int print_usage()
{
    xdd_lite_options_print_usage();
    return 0;
}

/** Ensure the options are specified correctly */
int validate_options(xdd_lite_options_t* opts)
{
	int rc = 0;
	size_t active_target_length = 0;
	int has_itarget = 0;
	int has_otarget = 0;
	
	// Ensure there is at least one target
	if (0 == opts->num_targets)
		rc++;
	else {
		// Check that target options make sense together
		xdd_lite_target_options_t *top = opts->to_head;
		while (NULL != top) {
			// Log the target types
			if (XDDLITE_IN_TARGET_TYPE == top->type)
				has_itarget = 1;
			else if (XDDLITE_OUT_TARGET_TYPE == top->type)
				has_otarget = 1;
			
			// If the uri is xni, dio and start_offset don't make sense
			int is_xni_target = (0 == strncmp("xni://", top->uri, 6));
			if (is_xni_target && 0 != top->start_offset) {
				fprintf(stderr,
						"Error: XNI targets do not support start offsets\n");
				rc++;
			}
			else if (is_xni_target && 0 != top->dio_flag) {
				fprintf(stderr,
						"Error: XNI targets do not support Direct I/O\n");
				rc++;
			}

			/* Save the first non-zero length */
			if (0 == active_target_length && 0 != top->length) {
				active_target_length = top->length;
			}
			
			// Check the lengths
			if (0 != top->length && top->length != active_target_length) {
				fprintf(stderr, "Error: Target lengths don't match\n");
				rc++;
			}

			// Iterate
			top = top->next;
		}

		// Must have both an in and out target type
		if (!(has_itarget && has_otarget)) {
			fprintf(stderr, "Error: Must include both an itarget and otarget\n");
			rc++;
		}
		
		// At least one of the targets had to specify a length
		if (0 >= active_target_length) {
			fprintf(stderr, "Error: A target length must be specified\n");
			rc++;
		}
	}
	return rc;		 
}

/* Start a forking server */
int start_server(char* iface, char* port, int backlog)
{
	int rc = 0;

	/* Start the forking server */
	//rc = xdd_lite_start_forking_server(iface, port, backlog);
	return rc;
}

/* Start a client capable of interacting with the forking server */
int start_client(xdd_planpub_t* plan)
{
	int rc = 0;
	
	/* Execute the plan */
	//rc += xdd_plan_start(plan);
	//rc += xdd_plan_wait(plan);	
	return rc;
}

/** Main */
int main(int argc, char** argv)
{
    int rc;
    xdd_lite_options_t opts;

    /* Initialize and parse options */
    rc = xdd_lite_options_init(&opts);    
    if (0 == rc) {
        rc += xdd_lite_options_parse(&opts, argc, argv);
    }

	/* Print help */
    if (0 != rc) {
        print_usage();
		xdd_lite_options_destroy(&opts);
		return 1;
    }
	else if (1 == opts.help_flag) {
        print_usage();
		xdd_lite_options_destroy(&opts);
		return 0;
	}

	/* Validate options */
	if (0 != validate_options(&opts)) {
		xdd_lite_options_destroy(&opts);
		return 1;
	}
	
	/* Convert the options into a plan */
	xdd_planpub_t lite_plan;
	rc = xdd_lite_options_plan_create(opts, &lite_plan);
	if (0 != rc) {
		fprintf(stderr, "Error: Invalid plan\n");
		return 1;
	}

	/* Cleanup the options */
	rc = xdd_lite_options_destroy(&opts);
	if (0 != rc) {
		fprintf(stderr, "Error: Unable to free options\n");
		return 1;
	}
	
	/* Start the plan */
	rc = xdd_plan_start(&lite_plan);
	if (0 != rc) {
		fprintf(stderr, "Error: Unable to start plan execution\n");
		return 2;
	}

	/* Wait for completion */
	rc = xdd_plan_wait(&lite_plan);
	if (0 != rc) {
		fprintf(stderr, "Error: Plan execution failed\n");
		return 2;
	}

	/* TODO: Output the plan results here */
	
	/* Cleanup the plan */
	rc = xdd_plan_destroy(&lite_plan);
    return rc;
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
