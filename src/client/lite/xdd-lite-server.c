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
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include "libxdd.h"

int xdd_lite_start_destination(int sd) {
	int rc = 0;
	union {
		struct { char buf[16]; } raw;
		struct { uint64_t magic, msize; } data;
	} header;
	size_t sz;
	char* mbuf;

	/* Peek the message header */
	sz = 0;
	while (sz < 16)
		sz = recv(sd, header.raw.buf + sz, 16 - sz, MSG_PEEK);

	/* Check the magic number and size */
	printf("Header magic: %lld", (long long) header.data.magic);
	printf("Header size: %lld", (long long) header.data.msize);
	
	/* Receive the full plan description */
	mbuf = malloc(header.data.msize);
	sz = 0;
	while (sz < header.data.msize)
		recv(sd, mbuf + sz, header.data.msize - sz, MSG_WAITALL);

	/* Setup the plan */

	/* Start the plan */
	
	/* Send the acknowledgement */

	
	close(sd);
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
