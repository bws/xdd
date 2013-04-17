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
#include "xint-lite.h"
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libxdd.h"

/* Forward declarations */
static int parse_heartbeat(xint_lite_options_t* opts, char* val);
static int parse_verbosity(xint_lite_options_t* opts, char* val);
static int parse_help(xint_lite_options_t* opts, char* val);
static int parse_file(xint_lite_options_t* opts, char* val);
static int parse_offset(xint_lite_options_t* opts, char* val);
static int parse_length(xint_lite_options_t* opts, char* val);
static int parse_e2e_spec(xint_lite_options_t* opts, char* val);

/** Initialize the options structure */
int xint_lite_options_init(xint_lite_options_t* opts) {
    assert(0 != opts);
    memset(opts, 0, sizeof(xint_lite_options_t));
    opts->stop_on_error_flag = 1;
    return 0;
}

/** Deallocate any resources associated with the options structure */
int xint_lite_options_destroy(xint_lite_options_t* opts) {
    assert(0 != opts);
    free(opts->e2e_specs);
    memset(opts, 0, sizeof(xint_lite_options_t));
    return 0;
}

/** Parse a cli string into an options structure */
int xint_lite_options_parse(xint_lite_options_t* opts, int argc, char** argv) {
    int rc = 0;
    int option_idx = 0;

    static struct option long_options[] = {
        /* Heartbeat */
        {"hb", no_argument, 0, 'b'},
        /* Enable direct I/O */
        {"dio", no_argument, 0, 'd'},
        /* Specify an e2e spec */
        {"e2e", required_argument, 0, 'e'},
        /* Display usage */
        {"help", no_argument, 0, 'h'},
        /* File offset */
        {"offset", required_argument, 0, 'o'},
        /* File size */
        {"length", required_argument, 0, 'l'},
        /* Select ordering mode */
        {"ordering", required_argument, 0, 'r'},
        /* Verbosity */
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int c = 0;
    while (0 == rc && -1 != c) {
        c = getopt_long(argc, argv, "bdeholrv", long_options, &option_idx);
        switch (c) {
            case 0:
                printf("0 encountered");
                rc = 1;
            case 'b':
		rc = parse_heartbeat(opts, argv[option_idx]);
		break;
            case 'd':
		opts->dio_flag = 1;
		break;
            case 'e':
		rc = parse_e2e_spec(opts, argv[option_idx]);
		break;
            case 'h':
		opts->help_flag = 1;
		break;
            case 'l':
		rc = parse_length(opts, argv[option_idx]);
		break;
            case 'o':
		rc = parse_offset(opts, argv[option_idx]);
		break;
            case 'r':
		rc = parse_e2e_spec(opts, argv[option_idx]);
		break;
            case 'v':
		rc = parse_verbosity(opts, argv[option_idx]);
		break;
            default:
                printf("Unexpected option %c", c);
                rc = 1;
        }
    }

    /* The only non-option argument is the file name */
    if ((option_idx + 1) == argc) {
	opts->fname = argv[option_idx];
    }
    else {
	rc = 1;
    }
    return rc;
}

/** Convert the options into a valid plan */
int xint_lite_plan_create(xint_lite_options_t* opts, xdd_plan_t* plan) {
    int rc;
    int i = 0;

    assert(0 != opts);
    for (i = 0; i < 1; i++) {
        rc = xdd_plan_create_e2e(plan);
        opts = 0;
        //rc = xdd_create_e2e_target(op_type, fname, offset, length, is_dio, qd, stdout, stderr, verbose, tracing, preallocate,
        //                           is_stop_error, restart_stuff, heartbeat_stuff,
        //                           e2e_host, e2e_port, e2e_qd, e2e_numa);
    }

    return rc;
}

/** Print usage information to stdout */
int xint_lite_print_usage() {
    printf("usage: xdd-lite [options] file\n");
    return 0;
}

/** Parse the heartbeat string into the option structure */
int parse_heartbeat(xint_lite_options_t* opts, char* val) {
    return 0;
}

/** Parse the offset string into the option structure */
int parse_offset(xint_lite_options_t* opts, char* val) {
    int rc = 0;
    char** p = 0;
    unsigned long num = strtoul(val, p, 10);
    if (NULL == p) {
	rc = 1;
    }
    else {
	opts->offset = num;
    }
    return rc;
}

/** Parse the file legth string into the option structure */
int parse_length(xint_lite_options_t* opts, char* val) {
    int rc = 0;
    char** p = 0;
    unsigned long num = strtoul(val, p, 10);
    if (NULL == p) {
	rc = 1;
    }
    else {
	opts->length = num;
    }
    return rc;
}

/** Parse the verbosity string into the option structure */
int parse_verbosity(xint_lite_options_t* opts, char* val) {
    opts->verbose_flag = 1;
    return 0;
}

/** Parse the e2e spec string into the option structure */
int parse_e2e_spec(xint_lite_options_t* opts, char* spec) {
    int rc = 0;
    int new_count = opts->num_e2e_specs + 1;
    xint_lite_e2e_t* new_e2e_list;

    new_e2e_list = realloc(opts->e2e_specs, new_count * sizeof(*new_e2e_list));
    if ( 0 != new_e2e_list ) {
	/* Parse string of the form host:port,count%numa */
	new_e2e_list[new_count - 1].host = "none";
	new_e2e_list[new_count - 1].port = 0;
	new_e2e_list[new_count - 1].thread_count = 0;
	new_e2e_list[new_count - 1].numa = -1;

	/* Swap the old and new values */
	free(opts->e2e_specs);
	opts->num_e2e_specs = new_count;
	opts->e2e_specs = new_e2e_list;
    }
    else {
	rc = 1;
    }
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
