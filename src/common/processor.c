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
 * This file contains the subroutines that figure out how many CPUs are on a
 * system and how to assign those processors to specific Target or QThreads.
 */
#include "xdd.h"
/*----------------------------------------------------------------------------*/
/* xdd_processor() - assign this xdd thread to a specific processor 
 * This works on most operating systems except LINUX at the moment. 
 * The ability to assign a *process* to a specific processor is new to Linux as
 * of the 2.6.8 Kernel. However, the ability to assign a *thread* to a specific
 * processor still does not exist in Linux as of 2.6.8. Therefore, all xdd threads
 * will run on all processors using a "faith-based" approach - in other words you
 * need to place your faith in the Linux kernel thread scheduler that the xdd
 * threads get scheduled on the appropriate processors and evenly spread out
 * accross all processors as efficiently as possible. Scarey huh? If/when the
 * Linux developers provide the ability for individual threads to be assigned
 * to specific processors for Linux that code will be incorporated here.
 */
void
xdd_processor(ptds_t *p) {
#if (SOLARIS)
	processorid_t i;
	int32_t  status;
	int32_t  n,cpus;
	n = sysconf(_SC_NPROCESSORS_ONLN);
	cpus = 0;
	for (i = 0; n > 0; i++) {
		status = p_online(i, P_STATUS);
		if (status == -1 )
			continue;
		/* processor present */
		if (cpus == p->processor) 
			break;
		cpus++;
		n--;
	}
	/* At this point "i" contains the processor number to bind this process to */
	status = processor_bind(P_LWPID, P_MYID, i, NULL);
	if (status < 0) {
		fprintf(xgp->errout,"%s: Processor assignment failed for target %d\n",xgp->progname,p->my_target_number);
		perror("Reason");
	}
	return;

#elif (LINUX)
	size_t 		cpumask_size; 	/* size of the CPU mask in bytes */
	cpu_set_t 	cpumask; 	/* mask of the CPUs configured on this system */
	int			status; 	/* System call status */
	int32_t 	n; 		/* the number of CPUs configured on this system */
	int32_t 	cpus; 		/* the number of CPUs configured on this system */
	int32_t 	i; 

	cpumask_size = (unsigned int)sizeof(cpumask);
	status = sched_getaffinity(syscall(SYS_gettid), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error getting the CPU mask when trying to schedule processor affinity\n",xgp->progname);
		return;
	}
	n = xdd_linux_cpu_count();
	cpus = 0;

	for (i = 0; n > 0; i++) {
		if (CPU_ISSET(i, &cpumask)) {
			/* processor present */
			if (cpus == p->processor) 
				break;
			cpus++;
			n--;
		}
	}
	/* at this point i contains the proper CPU number to use in the mask setting */
	cpumask_size = (unsigned int)sizeof(cpumask);
	CPU_ZERO(&cpumask);
	CPU_SET(i, &cpumask);
	status = sched_setaffinity(syscall(SYS_gettid), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error setting the CPU mask when trying to schedule processor affinity\n",xgp->progname);
		perror("Reason");
	}
	if (xgp->global_options&GO_REALLYVERBOSE)
		fprintf(xgp->output,"%s: INFORMATION: Assigned processor %d to pid %d threadid %d \n",
			xgp->progname,
			p->processor,
			p->my_pid,
			p->my_thread_id);
	return;
#elif (AIX)
	int32_t status;
	if (xgp->global_options & GO_REALLYVERBOSE)
		fprintf(xgp->output, "Binding process/thread %d/%d to processor %d\n",p->my_pid, p->my_thread_id, p->processor);
	status = bindprocessor( BINDTHREAD, p->my_thread_id, p->processor );
	if (status) {
		fprintf(xgp->errout,"%s: Processor assignment failed for target %d to processor %d, thread ID %d, process ID %d\n",
			xgp->progname, p->my_target_number, p->processor, p->my_thread_id, p->my_pid);
		perror("Reason");
	}
	return;
#elif (IRIX || WIN32)
	int32_t  i;
	i = sysmp (MP_MUSTRUN,p->processor);
	if (i < 0) {
		fprintf(xgp->errout,"%s: **WARNING** Error assigning target %d to processor %d\n",
			xgp->progname, p->my_target_number, p->processor);
		perror("Reason");
	}
	return;
#endif
} /* end of xdd_processor() */
/*----------------------------------------------------------------------------*/
/* xdd_get_processor() - Get the processor number that this is current running on.
 * This routine exists because there is no commonly defined routine that does
 * this. It is supposed to be sched_getcpu() but that is not implemented by
 * all OS versions.
 * This subroutine will return an integer from 0 to N-1 where N is the number
 * of processors on this system. If there is no subroutine for getting this
 * information then a -1 will be returned.
 */
int
xdd_get_processor() {

#ifdef HAVE_UTMPX_H
		return( sched_getcpu() );
#else
		return(-1);
#endif
} // End of xdd_get_processor()
