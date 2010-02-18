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
 * This file contains the subroutines that implement process scheduling
 * functions that can raise or lower execution priority on a system.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_schedule_options() - do the appropriate scheduling operations to 
 *   maximize performance of this program.
 */
void
xdd_schedule_options(void) {
	int32_t status;  /* status of a system call */
	struct sched_param param; /* for the scheduler */

	if (xgp->global_options & GO_NOPROCLOCK) 
                return;
#if !(OSX)
#if (IRIX || SOLARIS || AIX || LINUX || FREEBSD)
	if (getuid() != 0)
		fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to lock processes\n",xgp->progname);
#endif 
	/* lock ourselves into memory for the duration */
	status = mlockall(MCL_CURRENT | MCL_FUTURE);
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_schedule_options: cannot lock process into memory\n",xgp->progname);
		perror("Reason");
	}
	if (xgp->global_options & GO_MAXPRI) {
#if (IRIX || SOLARIS || AIX || LINUX || FREEBSD)
		if (getuid() != 0) 
			fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to max priority\n",xgp->progname);
#endif
		/* reset the priority to max max max */
		param.sched_priority = sched_get_priority_max(SCHED_FIFO);
		status = sched_setscheduler(0,SCHED_FIFO,&param);
		if (status == -1) {
			fprintf(xgp->errout,"%s: xdd_schedule_options: cannot reschedule priority\n",xgp->progname);
			perror("Reason");
		}
	}
#endif // if not OSX
} /* end of xdd_schedule_options() */
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
