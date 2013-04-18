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
#ifndef XINT_LITE_H
#define XINT_LITE_H

#include <stdint.h>
#include <stdlib.h>
#include <xdd_plan.h>

#define XDD_LITE_DEFAULT_LISTEN_BACKLOG 10
#define XDD_LITE_DEFAULT_LISTEN_IFACE NULL
#define XDD_LITE_DEFAULT_LISTEN_PORT "40000"

typedef enum xdd_lite_op {NULL_OP_TYPE = 0, READ, WRITE} xint_op_t;

typedef struct xdd_lite_e2e {
    char* host;
    int port;
    int thread_count;
    int numa;
} xdd_lite_e2e_t;
    
typedef struct xdd_lite_options {
	int help_flag;
	int server_flag;
	int verbose_flag;

	/* Client/source specific options */
	struct client_source {
		xint_op_t ot;
		char* fname;
		size_t offset;
		size_t length;
		int dio_flag;
		char* out_filename;
		char* err_filename;
		size_t preallocate;
		char* restart_fname;
		size_t heartbeat_freq;
		int server_port;
			
		/* List of e2e connection information */
		int num_e2e_specs;
		xdd_lite_e2e_t* e2e_specs;
	} c;
	/* Server/destination specific options */
	struct server_destination {
		char* iface;
		char* port;
		int backlog;
	} s;

} xdd_lite_options_t;

int xdd_lite_options_init(xdd_lite_options_t* opts);

int xdd_lite_options_destroy(xdd_lite_options_t* opts);

int xdd_lite_options_parse(xdd_lite_options_t* opts, int argc, char** argv);

int xdd_lite_options_plan_create(xdd_lite_options_t* opts, xdd_plan_pub_t* plan);

int xdd_lite_print_usage();

int xdd_lite_start_source();

int xdd_lite_start_destination(int sd);

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
