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
/*
 * This file contains the signal handler and the routine that initializes
 * the signal hanler call.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_signal_handler() - Routine that gets called when a signal gets caught. 
 * This will call the appropriate routines to shut down gracefully.
 */
void
xdd_signal_handler(int signum, siginfo_t *sip, void *ucp) {
	ucontext_t	*up;		// Pointer to the ucontext structure

	up = (ucontext_t *)ucp;
	
	fprintf(xgp->errout,"\n%s: xdd_signal_handler: Received signal %d: ", xgp->progname, signum);
	switch (signum) {
		case SIGINT:
			fprintf(xgp->errout,"SIGINT: Interrupt from keyboard\n");
			break;
		case SIGQUIT:
			fprintf(xgp->errout,"SIGQUIT: Quit from keyboard\n");
			break;
		case SIGABRT:
			fprintf(xgp->errout,"SIGABRT: Abort\n");
			break;
		case SIGTERM:
			fprintf(xgp->errout,"SIGTERm: Termination\n");
			break;
		default:
			fprintf(xgp->errout,"Unknown Signal - terminating\n");
			break;
	}
	fflush(xgp->errout);
	xgp->canceled += 1;
	if (xgp->canceled > 3) {
		fprintf(xgp->errout,"xdd_signal_handler: Starting Debugger\n");
		xdd_signal_start_debugger();
	}

} /* end of xdd_signal_handler() */

/*----------------------------------------------------------------------------*/
/* xdd_signal_init() - Initialize all the signal handlers
 */
int32_t
xdd_signal_init(void) {
	int		status;			// status of the sigaction() system call


	xgp->sa.sa_sigaction = xdd_signal_handler;			// Pointer to the signal handler
 	sigemptyset( &xgp->sa.sa_mask );				// The "empty set" - don't mask any signals
	xgp->sa.sa_flags = SA_SIGINFO; 					// This indicates that the signal handler will get a pointer to a siginfo structure indicating what happened
	status 	= sigaction(SIGINT,  &xgp->sa, NULL);	// Interrupt from keyboard - ctrl-c
	status += sigaction(SIGQUIT, &xgp->sa, NULL);	// Quit from keyboard 
	status += sigaction(SIGABRT, &xgp->sa, NULL);	// Abort signal from abort(3)
	status += sigaction(SIGTERM, &xgp->sa, NULL);	// Termination

	if (status) {
		fprintf(xgp->errout,"%s: xdd_signal_init: ERROR initializing signal handler(s)\n",xgp->progname);
		perror("Reason");
		return(-1);
	}
	return(0);

} /* end of xdd_signal_init() */

/*----------------------------------------------------------------------------*/
/* xdd_signal_start_debugger() - Will start the Interactive Debugger Thread
 */
void
xdd_signal_start_debugger() {
	int32_t		status;			// Status of a subroutine call


	status = pthread_create(&xgp->Interactive_Thread, NULL, xdd_interactive, (void *)(unsigned long)1);
	if (status) {
		fprintf(xgp->errout,"%s: xdd_signal_start_debugger: ERROR: Could not start debugger control processor\n", xgp->progname);
		fflush(xgp->errout);
		xdd_destroy_all_barriers();
		exit(1);
	}

} // End of xdd_signal_start_debugger()
