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
 *       Brad Settlemyer, DoE/ORNL
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

/*
 * Boolean flag indicating whether the user wishes to enter the debugger on
 * SIGINT (ctrl-c)
 */
static int enter_debugger_on_sigint = 0;

/*----------------------------------------------------------------------------*/
/* xdd_signal_handler() - Routine that gets called when a signal gets caught. 
 * This will call the appropriate routines to shut down gracefully.
 */
void
xdd_signal_handler(int signum, siginfo_t *sip, void *ucp) {
	int enter_debugger = 0;
        
	fprintf(xgp->errout,"\n%s: xdd_signal_handler: Received signal %d: ", xgp->progname, signum);
	switch (signum) {
		case SIGINT:
			fprintf(xgp->errout,"SIGINT: Interrupt from keyboard\n");
                        if (enter_debugger_on_sigint)
                            enter_debugger = 1;
			break;
		case SIGQUIT:
			fprintf(xgp->errout,"SIGQUIT: Quit from keyboard\n");
			break;
		case SIGABRT:
			fprintf(xgp->errout,"SIGABRT: Abort\n");
			break;
		case SIGTERM:
			fprintf(xgp->errout,"SIGTERM: Termination\n");
			break;
		case SIGUSR1:
			fprintf(xgp->errout,"SIGUSR1: User requests debug mode\n");
                        enter_debugger = 1;
			break;
		default:
			fprintf(xgp->errout,"Unknown Signal - terminating\n");
			break;
	}
	fflush(xgp->errout);

        if (enter_debugger) {
            fprintf(xgp->errout,"xdd_signal_handler: Starting Debugger\n");
            xdd_signal_start_debugger();
        }
        else {
            exit(signum);
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
	status += sigaction(SIGUSR1, &xgp->sa, NULL);	// Termination

	if (status) {
		fprintf(xgp->errout,"%s: xdd_signal_init: ERROR initializing signal handler(s)\n",xgp->progname);
		perror("Reason");
		return(-1);
	}

        /* If XDD_DEBUG_ENABLE is set at startup, ctrl-c will enter the debugger
           rather than cancel the task */
        if (0 != getenv("XDD_DEBUG_ENABLE"))
            enter_debugger_on_sigint = 1;
        
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

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
