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
#include <string.h>
#include "libxdd.h"
#include "xdd-lite.h"
#include "xdd-lite-forking-server.h"

/** Print out the CLI usage information */
int print_usage()
{
    xdd_lite_print_usage();
    return 0;
}

/* Start a forking server */
int start_server(char* iface, char* port, int backlog)
{
	int rc = 0;

	/* Start the forking server */
	rc = xdd_lite_start_forking_server(iface, port, backlog);
	return rc;
}

/* Start a client capable of interacting with the forking server */
int start_client(xdd_plan_pub_t* plan)
{
	int rc = 0;
	
	/* Execute the plan */
	rc += xdd_plan_start(plan);
	rc += xdd_plan_wait(plan);	
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
    } else {
		xdd_lite_options_destroy(&opts);
		return rc;
    }

	/* Print help */
    if (0 != rc || 1 == opts.help_flag) {
        print_usage();
		xdd_lite_options_destroy(&opts);
		return 1;
    }

	/* Start XDD-Lite in server mode or client mode */
	if (0 != opts.server_flag) {
		/* Extract the relevant server options */
		char iface[256];
		char port[256];
		int backlog = opts.s.backlog;
		strncpy(iface, opts.s.iface, 255);
		strncpy(port, opts.s.port, 255);
		iface[255] = port[255] = '\0';
		xdd_lite_options_destroy(&opts);

		/* Start the server */
		rc = start_server(iface, port, backlog);
	} else {
		/* Construct a plan from the specified options */
		xdd_plan_pub_t lite_plan;		
		rc += xdd_plan_init(&lite_plan);
		rc += xdd_lite_options_plan_create(&opts, &lite_plan);
		rc += xdd_lite_options_destroy(&opts);

		/* Start the client */
		rc = start_client(&lite_plan);
		xdd_plan_destroy(&lite_plan);
	}
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
