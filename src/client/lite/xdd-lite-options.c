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
#include "xdd-lite.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libxdd.h"

/* Forward declarations */
static int parse_block_size(xdd_lite_options_t* opts, char* val);
static int parse_request_size(xdd_lite_options_t* opts, char* val);
static int parse_itarget(xdd_lite_options_t* opts, char* val);
static int parse_itarget(xdd_lite_options_t* opts, char* val);
static int parse_mtarget(xdd_lite_options_t* opts, char* val);
static int parse_otarget(xdd_lite_options_t* opts, char* val);
static int parse_target_access(xdd_lite_options_t* opts, char* val);
static int parse_target_direct_io(xdd_lite_options_t* opts, char* val);
static int parse_target_length(xdd_lite_options_t* opts, char* val);
static int parse_target_num_threads(xdd_lite_options_t* opts, char* val);
static int parse_target_policy(xdd_lite_options_t* opts, char* val);
static int parse_target_start_offset(xdd_lite_options_t* opts, char* val);
static int parse_target_help(xdd_lite_options_t* opts, char* val);
static int parse_target_verbose(xdd_lite_options_t* opts, char* val);

/** Initialize a target options structure */
int xdd_lite_target_options_init(xdd_lite_target_options_t* topts) {
    assert(0 != topts);
    memset(topts, 0, sizeof(*topts));
	topts->num_threads = XDDLITE_DEFAULT_NUM_TARGET_THREADS;

	// TODO: Add this to the options
    return 0;
}

/** Deallocate any resources associated with a target options structure */
int xdd_lite_target_options_destroy(xdd_lite_target_options_t* topts) {
    assert(0 != topts);

	// Free each of the target option elements
	memset(topts, 0, sizeof(*topts));
    return 0;
}

/** Initialize the options structure */
int xdd_lite_options_init(xdd_lite_options_t* opts) {
    assert(0 != opts);
    memset(opts, 0, sizeof(xdd_lite_options_t));
	opts->block_size = XDDLITE_DEFAULT_BLOCK_SIZE;
	opts->request_size = XDDLITE_DEFAULT_REQUEST_SIZE;
    return 0;
}

/** Deallocate any resources associated with the options structure */
int xdd_lite_options_destroy(xdd_lite_options_t* opts) {
    assert(0 != opts);

	// Free each of the target option elements
	struct xdd_lite_target_options* p = opts->to_head;
	for (size_t i = 0; i < opts->num_targets; i++) {
		struct xdd_lite_target_options* t = p;
		p = p->next;
		free(t);
	}
    return 0;
}

/** Print xdd-lite option usage information */
int xdd_lite_options_print_usage() {
	printf("Usage: xdd-lite [Global options] [Target Spec [Target options]]+\n");
	printf("Global Options:\n\n");
	printf("  -A, --again               Enable transfer restart/resume.\n");
	printf("  -B, --block-size=BYTES    \n");
	printf("  -R, --request-size=BLOCKS \n");
	printf("  -H, --help\n");
	printf("  -V, --verbose             \n");
	
	printf("\nTarget Specs:\n\n");
	printf("  -i, --itarget=URI \n");
	printf("  -o, --otarget=URI \n");
	printf("  -m, --mtarget=??? \n");

	printf("\nTarget Options:\n\n");
	printf("  -a, --access=ORDER        \n");
	printf("  -d, --direct-io           \n");
	printf("  -l, --length=BYTES \n");
	printf("  -n, --num-threads=NUM     \n");
	printf("  -p, --policy=POLICY       \n");
	printf("  -s, --start-offset=BYTES  \n");
	return 0;
}

/** Parse a cli string into an options structure */
int xdd_lite_options_parse(xdd_lite_options_t* opts, int argc, char** argv) {
    int rc = 0;
    int option_idx = 0;

    static struct option long_options[] = {
        /* Global restart/resume flag */
        {"again", no_argument, 0, 'A'},
        /* Global block size */
        {"block-size", required_argument, 0, 'B'},
        /* Display usage */
        {"help", no_argument, 0, 'H'},
        /* Global length in Bytes */
        {"length", required_argument, 0, 'L'},
        /* Global request size */
        {"request-size", required_argument, 0, 'R'},
        /* Global verbosity */
        {"verbose", no_argument, 0, 'V'},
        /* In target */
        {"itarget", required_argument, 0, 'i'},
        /* Meta target */
        {"mtarget", required_argument, 0, 'm'},
        /* Out target */
        {"otarget", required_argument, 0, 'o'},
        /* Target access policy */
        {"access", required_argument, 0, 'a'},
        /* Target direct I/O flag */
        {"direct-io", no_argument, 0, 'd'},
        /* Target number of threads */
        {"num-threads", required_argument, 0, 'n'},
        /* Select ordering mode */
        {"policy", required_argument, 0, 'p'},
        /* Select start offset */
        {"start-offset", required_argument, 0, 's'},
        /* Target help */
        {"help-target", required_argument, 0, 'h'},
        /* Target verbosity */
        {"verbose-target", required_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

	size_t err_count = 0;
    int c = 0;
    while (0 == err_count) {
        c = getopt_long(argc, argv,
						"AB:HR:Vi:m:o:a:dl:n:p:s:hv",
						long_options, &option_idx);
		if (-1 == c) {
			break;
		}
        switch (c) {
            case 'A':
				opts->again_flag = 1;
				break;
            case 'B':
				err_count += parse_block_size(opts, optarg);
				break;
            case 'H':
				opts->help_flag = 1;
				break;
            case 'R':
				err_count += parse_request_size(opts, optarg);
				break;
            case 'V':
				opts->verbose_flag = 1;
				break;
            case 'i':
				err_count += parse_itarget(opts, optarg);
				break;
            case 'o':
				err_count += parse_otarget(opts, optarg);
				break;
            case 'm':
				err_count += parse_mtarget(opts, optarg);
				break;
            case 'a':
				err_count += parse_target_access(opts, optarg);
				break;
            case 'd':
				err_count += parse_target_direct_io(opts, optarg);
				break;
            case 'l':
				err_count += parse_target_length(opts, optarg);
				break;
            case 'n':
				err_count += parse_target_num_threads(opts, optarg);
				break;
            case 'p':
				err_count += parse_target_policy(opts, optarg);
				break;
            case 's':
				err_count += parse_target_start_offset(opts, optarg);
				break;
            case 'h':
				err_count += parse_target_help(opts, optarg);
				break;
            case 'v':
				err_count += parse_target_verbose(opts, optarg);
				break;
            default:
                printf("Error: Unknown optopt: %c optind: %d opterr: %d optarg: %s char: %c\n", optopt, optind, opterr, optarg, c);
                err_count++;
        }
    }
	// Convert non-zero error counts into a return code
	rc = (0 == err_count ? 0 : 1);
	
    return rc;
}

/** Convert the options into a valid plan */
int xdd_lite_options_plan_create(xdd_lite_options_t opts, xdd_planpub_t* plan) {
    int rc = 0;

	// Create the target attributes
	xdd_targetattr_t *tattrs = calloc(opts.num_targets, sizeof(*tattrs));
	struct xdd_lite_target_options *topt = opts.to_head;
	for (size_t i = 0; i < opts.num_targets; i++) {
		xdd_targetattr_t* ta = tattrs + i;
		xdd_targetattr_init(ta);
		if (XDDLITE_IN_TARGET_TYPE == topt->type) {
			xdd_targetattr_set_type(ta, XDD_IN_TARGET_TYPE);
		}
		else if (XDDLITE_OUT_TARGET_TYPE == topt->type) {
			xdd_targetattr_set_type(ta, XDD_OUT_TARGET_TYPE);
		}
		else {
			rc = 1;
			free(tattrs);
			return rc;
		}
		xdd_targetattr_set_uri(ta, topt->uri);
		xdd_targetattr_set_dio(ta, topt->dio_flag);
		xdd_targetattr_set_num_threads(ta, topt->num_threads);
		xdd_targetattr_set_start_offset(ta, topt->start_offset);

		/* Use the specified length if its there, otherwise use the default */
		if (0 != topt->length) {
			xdd_targetattr_set_length(ta, topt->length);
		} else {
			xdd_targetattr_set_length(ta, opts.default_target_length);
		}
		// Iterate to the target option struct
		topt = topt->next;
	}
	
	// Create the plan attributes
	xdd_planattr_t pattr;
	xdd_planattr_init(&pattr);
	xdd_planattr_set_block_size(&pattr, opts.block_size);
	xdd_planattr_set_request_size(&pattr, opts.request_size);

	// Create the plan
	rc = xdd_plan_init(plan, tattrs, opts.num_targets, pattr);

	// Clean up the memory
	for (size_t i = 0; i < opts.num_targets; i++)
		xdd_targetattr_destroy(tattrs + i);
	free(tattrs);
    return rc;
}

/** Parse the block size */
int parse_block_size(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
    char* p = 0;
    unsigned long long num;
	errno = 0;
	num = strtoull(val, &p, 10);

    if (ERANGE == errno) {
		fprintf(stderr, "Error: Block size too large: %s\n", val);
		rc = 1;
	}
	else if ('\0' != *p) {
		fprintf(stderr, "Error: Invalid block size: %s at %s\n", val, p);
		rc = 1;
    }
    else {
		opts->block_size = num;
    }
    return rc;
}

/** Parse the request size */
int parse_request_size(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
    char* p = 0;
    unsigned long long num;
	errno = 0;
	num = strtoull(val, &p, 10);

    if (ERANGE == errno) {
		fprintf(stderr, "Error: Request size too large: %s\n", val);
		rc = 1;
	}
	else if ('\0' != *p) {
		fprintf(stderr, "Error: Invalid request size: %s at %s\n", val, p);
		rc = 1;
    }
    else {
		opts->request_size = num;
    }
    return rc;
}

/** Parse the itarget */
int parse_itarget(xdd_lite_options_t* opts, char* val) {
    int rc = 0;

	struct xdd_lite_target_options* tmp = calloc(1, sizeof(*tmp));
	if (NULL == tmp) {
		fprintf(stderr, "Error: Insufficient resources for input target\n");
		rc = 1;
	}
	else {
		xdd_lite_target_options_init(tmp);
		tmp->type = XDDLITE_IN_TARGET_TYPE;
		strncpy(tmp->uri, val, 255);
		tmp->uri[255] = '\0';
		tmp->num_threads = 1;

		/* Add the new target options to the list */
		if (0 == opts->num_targets) {
			opts->to_head = tmp;
		}
		else {
			opts->to_tail->next = tmp;
		}
		opts->to_tail = tmp;
		opts->num_targets++;
	}
    return rc;
}

/** Parse the mtarget */
int parse_mtarget(xdd_lite_options_t* opts, char* val) {
    int rc = 1;
	fprintf(stderr, "Error: Meta-target support not available\n");
	return rc;
}

/** Parse the otarget */
int parse_otarget(xdd_lite_options_t* opts, char* val) {
    int rc = 0;

	struct xdd_lite_target_options* tmp = calloc(1, sizeof(*tmp));
	if (NULL == tmp) {
		fprintf(stderr, "Error: Insufficient resources for output target\n");
		rc = 1;
	}
	else {
		xdd_lite_target_options_init(tmp);
		tmp->type = XDDLITE_OUT_TARGET_TYPE;
		strncpy(tmp->uri, val, 255);
		tmp->uri[255] = '\0';
		tmp->num_threads = 1;
		/* Add the new target options to the list */
		if (0 == opts->num_targets) {
			opts->to_head = tmp;
		}
		else {
			opts->to_tail->next = tmp;
		}
		opts->to_tail = tmp;
		opts->num_targets++;
	}
    return rc;
}

/** Parse the target policy */
int parse_target_access(xdd_lite_options_t* opts, char* val) {
    int rc = 0;

	size_t len = strlen(val);
	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target access type specified without active target\n");
		rc = 1;
	}
    else if (0 == strncmp("loose", val, len)) {
		opts->to_tail->access = XDDLITE_LOOSE_ACCESS_TYPE;
    }
    else if (0 == strncmp("random", val, len)) {
		opts->to_tail->access = XDDLITE_RANDOM_ACCESS_TYPE;
    }
    else if (0 == strncmp("serial", val, len)) {
		opts->to_tail->access = XDDLITE_SERIAL_ACCESS_TYPE;
    }
    else if (0 == strncmp("unordered", val, len)) {
		opts->to_tail->access = XDDLITE_UNORDERED_ACCESS_TYPE;
    }
    else {
		fprintf(stderr, "Error: Unknown access type: %s\n", val);
		rc = 1;
    }
    return rc;
}

/** Parse the file length string into the option structure */
int parse_target_direct_io(xdd_lite_options_t* opts, char* val) {
    int rc = 0;

	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target direct I/O specified without active target\n");
		rc = 1;
	}
	else {
		opts->to_tail->dio_flag = 1;
    }
    return rc;
}

/** Parse the file length string into the option structure */
int parse_target_length(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
    char* p = 0;
    unsigned long long num;
	errno = 0;
	num = strtoull(val, &p, 10);

	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target length specified without active target\n");
		rc = 1;
	}
    else if (ERANGE == errno) {
		fprintf(stderr, "Error:  Target length too large: %s\n", val);
		rc = 1;
	}
	else if ('\0' != *p) {
		fprintf(stderr, "Error: Invalid target length: %s\n", val);
		rc = 1;
    }
    else {
		opts->to_tail->length = num;

		// Set the default target length so that the user doesn't have to
		// specify a length for all targets
		opts->default_target_length = num;
    }
    return rc;
}

/** Parse the num_threads string into the option structure */
int parse_target_num_threads(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
    char* p = 0;
    unsigned long num;
	errno = 0;
	num = strtoul(val, &p, 10);

	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target num threads specified without active target\n");
		rc = 1;
	}
	if (ERANGE == errno) {
		fprintf(stderr, "Error: Number of threads: %s\n", val);
		rc = 1;
	}
    else if ('\0' == *p) {
		fprintf(stderr, "Error: Invalid target number of threads: %s\n", val);
		rc = 1;
    }
    else {
		opts->to_tail->num_threads = num;
    }
    return rc;
}

/** Parse the target policy */
int parse_target_policy(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
	size_t len = strlen(val);
	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target policy type specified without active target\n");
		rc = 1;
	}
    else if (0 == strncmp("any", val, len)) {
		opts->to_tail->policy = XDDLITE_ANY_POLICY_TYPE;
    }
    else {
		fprintf(stderr, "Error: Unknown policy: %s\n", val);
		rc = 1;
    }
    return rc;
}

/** Parse the offset string into the option structure */
int parse_target_start_offset(xdd_lite_options_t* opts, char* val) {
    int rc = 0;
    char* p = 0;
    unsigned long num = strtoul(val, &p, 10);

	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target start offset specified without active target\n");
		rc = 1;
	}
    else if (ERANGE == errno) {
		fprintf(stderr, "Error: Start offset too large: %s\n", val);
		rc = 1;
	}
	else if ('\0' == *p) {
		fprintf(stderr, "Error: Invalid start offset: %s\n", val);
		rc = 1;
    }
    else {
		opts->to_tail->start_offset = num;
    }
    return rc;
}

/** Parse the verbosity string into the option structure */
int parse_target_help(xdd_lite_options_t* opts, char* val) {
	int rc = 0;
	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target help option specified without active target\n");
		rc = 1;
	}
    opts->help_flag = 1;
    return rc;
}

/** Parse the verbosity string into the option structure */
int parse_target_verbose(xdd_lite_options_t* opts, char* val) {
	int rc = 0;
 	if (NULL == opts->to_tail) {
		fprintf(stderr, "Error: Target verbose option specified without active target\n");
		rc = 1;
	}
    opts->verbose_flag = 1;
    return rc;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
