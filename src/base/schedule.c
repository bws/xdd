/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines that implement process scheduling
 * functions that can raise or lower execution priority on a system.
 */
#include "xint.h"
/*----------------------------------------------------------------------------*/
/* xdd_schedule_options() - do the appropriate scheduling operations to 
 *   maximize performance of this program.
 */
void
xdd_schedule_options(void) {
#ifdef HAVE_SCHEDSCHEDULER
    int32_t status;  /* status of a system call */
    struct sched_param param; /* for the scheduler */

    if (xgp->global_options & GO_NOPROCLOCK) 
	return;
    if (getuid() != 0)
	fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to lock processes\n",xgp->progname);
    /* lock ourselves into memory for the duration */
    status = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (status < 0) {
	fprintf(xgp->errout,"%s: xdd_schedule_options: cannot lock process into memory\n",xgp->progname);
	perror("Reason");
    }
    if (xgp->global_options & GO_MAXPRI) {
	if (getuid() != 0) 
	    fprintf(xgp->errout,"%s: xdd_schedule_options: You must be super user to max priority\n",xgp->progname);

	/* reset the priority to max max max */
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	status = sched_setscheduler(0,SCHED_FIFO,&param);
	if (status == -1) {
	    fprintf(xgp->errout,"%s: xdd_schedule_options: cannot reschedule priority\n",xgp->progname);
	    perror("Reason");
	}
    }
#endif
} /* end of xdd_schedule_options() */
 
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
