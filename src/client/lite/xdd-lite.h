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
#ifndef XDD_LITE_H
#define XDD_LITE_H

#include <stdint.h>
#include <stdlib.h>
#include <xdd_types.h>

#define XDDLITE_DEFAULT_LISTEN_BACKLOG 10
#define XDDLITE_DEFAULT_LISTEN_IFACE NULL
#define XDDLITE_DEFAULT_LISTEN_PORT "40000"
#define XDDLITE_DEFAULT_BLOCK_SIZE 4096
#define XDDLITE_DEFAULT_REQUEST_SIZE 1
#define XDDLITE_DEFAULT_NUM_TARGET_THREADS 1

enum xdd_lite_target_type {XDDLITE_NULL_TARGET_TYPE = 0,
						   XDDLITE_IN_TARGET_TYPE,
						   XDDLITE_META_TARGET_TYPE,
						   XDDLITE_OUT_TARGET_TYPE };
typedef enum xdd_lite_target_type xdd_lite_target_t;

enum xdd_lite_access_type {XDDLITE_NULL_ACCESS_TYPE = 0,
						   XDDLITE_LOOSE_ACCESS_TYPE,
						   XDDLITE_RANDOM_ACCESS_TYPE,
						   XDDLITE_SERIAL_ACCESS_TYPE,
						   XDDLITE_UNORDERED_ACCESS_TYPE };
typedef enum xdd_lite_access_type xdd_lite_access_t;

enum xdd_lite_policy_type {XDDLITE_NULL_POLICY_TYPE = 0,
						   XDDLITE_ANY_POLICY_TYPE };
typedef enum xdd_lite_policy_type xdd_lite_policy_t;

struct xdd_lite_target_options {
	xdd_lite_target_t type;
	char  uri[256];
	xdd_lite_access_t access;
	int dio_flag;
	size_t length;
	size_t num_threads;
	xdd_lite_policy_t policy;
	size_t start_offset;

	/* Use a linked-list pattern to chain all the target options */
	struct xdd_lite_target_options *next;
};
typedef struct xdd_lite_target_options xdd_lite_target_options_t;

struct xdd_lite_options {
	int again_flag;
	size_t block_size;
	int help_flag;
	size_t request_size;
	int verbose_flag;
	size_t num_targets;
	size_t default_target_length;

	/* List of target options */
	xdd_lite_target_options_t *to_head;
	xdd_lite_target_options_t *to_tail;
};
typedef struct xdd_lite_options xdd_lite_options_t;

int xdd_lite_target_options_init(xdd_lite_target_options_t* topts);

int xdd_lite_target_options_destroy(xdd_lite_target_options_t* topts);

int xdd_lite_options_init(xdd_lite_options_t* opts);

int xdd_lite_options_destroy(xdd_lite_options_t* opts);

int xdd_lite_options_parse(xdd_lite_options_t* opts, int argc, char** argv);

int xdd_lite_options_plan_create(xdd_lite_options_t opts, xdd_planpub_t* plan);

int xdd_lite_options_print_usage();

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
