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
#include "xdd-lite-forking-server.h"
#include <assert.h>
#include <stdio.h>
#include <string.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include "xdd-lite.h"

/** SIGCHLD handler */
static void wait_for_child(int sig) {
	assert(SIGCHLD == sig);
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Setup a signal handler for SIGCHLD */
static int setup_signal_handler() {
	int rc = 0;
	struct sigaction sa;
	
	sa.sa_handler = wait_for_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
	rc = sigaction(SIGCHLD, &sa, NULL);
    if (0 != rc) {
		/* Print out the errno code */
        perror("ERROR: Unable to set a signal handler.");
        rc = 1;
    }
	return rc;
}

static int start_child(int sd) {
	printf("Child work goes here: %d.\n", sd);
	return 0;	
}

/** A basic forking server */
int xdd_lite_start_forking_server(char* iface, char* port, int backlog) {
	int rc = 0;
	int parent_sock;
	struct addrinfo hints, *res;
	int reuseaddr = 1;

	/* Setup a signal handler for SIGCHLD */
	rc = setup_signal_handler();
	if (0 != rc) {
		return rc;
	}

	/* Get the address info */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	rc = getaddrinfo(iface, port, &hints, &res);
	if (0 != rc ) {
		fprintf(stderr, "Unable to resolve host %s:%s.\n", iface, port);
		return rc;
	}

	/* Setup the parent socket */
	parent_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	rc = setsockopt(parent_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	rc = bind(parent_sock, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	/* Listen */
	rc = listen(parent_sock, backlog);

	/* Main loop */
	while (1) {
        int pid;
		struct sockaddr_in their_addr;
        socklen_t size = sizeof(struct sockaddr_in);
        int child_sock = accept(parent_sock, (struct sockaddr*)&their_addr, &size);

		printf("Got a connection");
		pid = fork();
		if (0 == pid) {
			/* Child process */
			close(parent_sock);
			rc = start_child(child_sock);
			return rc;

		} else {
			close(child_sock);
			/* Parent process */
			if (-1 == pid) {
				/* Print message in errno */
				perror("ERROR: Unable to handle incoming connection");
			}
		}
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
