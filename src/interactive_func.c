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
 * This file contains the subroutines that support the Target threads.
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_interactive_exit()
 * Tell a all targets to abort and exit
 */
int
xdd_interactive_exit(int32_t tokens, char *cmdline, uint32_t flags) {
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "INTERACTIVE_EXIT", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);
	xgp->global_options |= GO_INTERACTIVE_EXIT;

	// Enter the FINAL barrier to tell XDD MAIN that everything is complete
	xdd_barrier(&xgp->main_results_final_barrier,&barrier_occupant,0);

	pthread_exit(0);

} // End of xdd_interactive_goto()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_help()
 * Display a list of valid commands 
 *
 */
int
xdd_interactive_help(int32_t tokens, char *cmdline, uint32_t flags) {

	return(0);

} // End of xdd_interactive_help()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_show()
 * Display any one of a number of things including:
 *  PTDS for a specific Target
 *  PTDS for a specific QThread on a specific Target
 *  The trace table
 *  XDD Global Data 
 *  Data in the buffer of specific QThread
 *  QThread Available Queue
 *  Barrier Chain
 *
 */
int
xdd_interactive_show(int32_t tokens, char *cmdline, uint32_t flags) {
	char	*cp;

	fprintf(xgp->output,"show: tokens=%d, cmdline=%s, flags=0x%08x\n",tokens,cmdline,flags);
	cp = cmdline;
	while ((*cp != TAB) && (*cp != SPACE) && (*cp != '\0')) cp++;
	while ((*cp == TAB) || (*cp == SPACE) || (*cp == '\0')) cp++;
	fprintf(xgp->output,"show: cp=%s\n",cp);
	if (strcmp(cp, "barrier") == 0) 
		xdd_interactive_show_barrier(tokens, cp, flags);
	else if (strcmp(cp, "ptds") == 0) 
		xdd_interactive_show_ptds(tokens, cp, flags);
	else if (strcmp(cp, "qtptds") == 0) 
		xdd_interactive_show_qtptds(tokens, cp, flags);
	else if (strcmp(cp, "qtsem") == 0) 
		xdd_interactive_show_qtsem(tokens, cp, flags);
	else if (strcmp(cp, "qtstate") == 0) 
		xdd_interactive_show_qtstate(tokens, cp, flags);
	else if (strcmp(cp, "trace") == 0) 
		xdd_interactive_show_trace(tokens, cp, flags);
	else if (strcmp(cp, "global") == 0) 
		xdd_interactive_show_global_data(tokens, cp, flags);
	else if (strcmp(cp, "data") == 0) 
		xdd_interactive_show_rwbuf(tokens, cp, flags);
	else { // Show a list of what one can show
		fprintf(xgp->output,"show: please specify: barrier, ptds, qtptds, qtsem, qtstate, trace, global, or data\n");
	}

	return(0);

} // End of xdd_interactive_show()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_run()
 * Tell XDD to start up the targets and run to completion
 *
 */
int
xdd_interactive_run(int32_t tokens, char *cmdline, uint32_t flags) {


	// Signal the target threads to start running
	xdd_barrier(&xgp->interactive_barrier,&xgp->interactive_occupant,1);

	return(0);

} // End of xdd_interactive_run()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_step()
 * Tell XDD to run one single OP for each Target and then stop
 *
 */
int
xdd_interactive_step(int32_t tokens, char *cmdline, uint32_t flags) {
	return(0);

} // End of xdd_interactive_step()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_stop()
 * Cause all Target QThreads to stop issuing I/O tasks and wait for either
 * a "run" or "step" command
 *
 */
int
xdd_interactive_stop(int32_t tokens, char *cmdline, uint32_t flags) {

	xgp->global_options |= GO_INTERACTIVE_STOP;

	return(0);

} // End of xdd_interactive_stop()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_goto()
 * Tell a specific Target or all Targets to go to a specific Operation number
 * and resume from that point
 *
 */
int
xdd_interactive_goto(int32_t tokens, char *cmdline, uint32_t flags) {

	// Set the current operation number in each target to the specified value

	return(0);

} // End of xdd_interactive_goto()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_rwbuf()
 * Display the data in the rw buffer of a specific QThread
 */
void
xdd_interactive_show_rwbuf(int32_t tokens, char *cmdline, uint32_t flags) {

	// Display the offset and data bytes in hex and text for the data buffer of each qthread

} // End of xdd_interactive_show_rwbuf()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_rwbuf()
 * Display the global_data for this run
 */
void
xdd_interactive_show_global_data(int32_t tokens, char *cmdline, uint32_t flags) {

	xdd_show_global_data();

} // End of xdd_interactive_show_global_data()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_ptds()
 * Display the PTDS of a specific QThread or Target Thread
 */
void
xdd_interactive_show_ptds(int32_t tokens, char *cmdline, uint32_t flags) {
	int		target_number;

	/* Target Specific variables */
	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		if (xgp->ptdsp[target_number]) {
			xdd_show_ptds(xgp->ptdsp[target_number]);
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a PTDS\n", target_number);
		}
	}

} // End of xdd_interactive_show_ptds()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_qtsem()
 * Display the state of the QThread semaphores
 */
void
xdd_interactive_show_qtsem(int32_t tokens, char *cmdline, uint32_t flags) {
	int		target_number;
	ptds_t	*p;
	ptds_t	*qp;
	int		sem_val;
	int		status;


	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number];
		if (p) {
			sem_val = 0;
			status = sem_getvalue(&p->sem_any_qthread_available, &sem_val);
			fprintf(xgp->output,"Target %d sem_any_qthread_available: status of sem_getvalue is %d, sem_val is %d",p->my_target_number, status, sem_val);
			qp = p->next_qp;
			while (qp) {
				sem_val = 0;
				status = sem_getvalue(&qp->sem_this_qthread_is_available, &sem_val);
				fprintf(xgp->output,"Target %d Qthread %d sem_this_qthread_is_available: status of sem_getvalue is %d, sem_val is %d",qp->my_target_number, qp->my_qthread_number, status, sem_val);
				sem_val = 0;
				if ((qp->target_options & TO_SERIAL_ORDERING) || (qp->target_options & TO_LOOSE_ORDERING))  {
					status = sem_getvalue(&qp->sem_qthread_ordering, &sem_val);
					fprintf(xgp->output,"Target %d Qthread %d sem_qthread_ordering: status of sem_getvalue is %d, sem_val is %d",qp->my_target_number, qp->my_qthread_number, status, sem_val);
				}
				qp = qp->next_qp;
			}
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a PTDS\n", target_number);
		}
	}
}

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_qtstate()
 * Display the state of each QThread
 */
void
xdd_interactive_show_qtstate(int32_t tokens, char *cmdline, uint32_t flags) {
	int		target_number;
	ptds_t	*p;
	ptds_t	*qp;


	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number];
		if (p) {
			fprintf(xgp->output,"Target %d\n",p->my_target_number);
			xdd_interactive_display_state_info(p);
			qp = p->next_qp;
			while (qp) {
				fprintf(xgp->output,"Target %d Qthread %d\n",qp->my_target_number, qp->my_qthread_number);
				xdd_interactive_display_state_info(qp);
				qp = qp->next_qp;
			}
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a PTDS\n", target_number);
		}
	}
} // End of xdd_interactive_show_qtstate()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_display_state_info()
 * Display the state of each QThread
 */
void
xdd_interactive_display_state_info(ptds_t *qp) {
	int64_t		tmp;
	pclk_t		now;		// Current time


	pclk_now(&now);		// Current time
	fprintf(xgp->output,"    Current State is 0x%08x\n",qp->my_current_state);
	if (qp->my_current_state & CURRENT_STATE_INIT)
		fprintf(xgp->output,"    Initializing\n");
	if (qp->my_current_state & CURRENT_STATE_IO) {
		tmp = (long long int)(now - qp->my_current_op_start_time)/BILLION;
		fprintf(xgp->output,"    Doing I/O - op %lld, location %lld, started at %lld or %lld milliseconds ago, end time is %lldn",
			(long long int)qp->target_op_number, 
			(long long int)qp->my_current_byte_location,
			(long long int)qp->my_current_op_start_time,
			(long long int)tmp,
			(long long int)qp->my_current_op_end_time);
	}
	if (qp->my_current_state & CURRENT_STATE_DEST_RECEIVE) 
		fprintf(xgp->output,"    Destination Side of an E2E - waiting to receive data from Source, target op number %lld, location %lld, length %lld, recvfrom status is %d\n", 
			(long long int)qp->target_op_number, 
			(long long int)qp->e2e_header.location, 
			(long long int)qp->e2e_header.length, 
			qp->e2e_recv_status);
	if (qp->my_current_state & CURRENT_STATE_SRC_SEND) 
		fprintf(xgp->output,"    Source Side of an E2E - waiting to send data to Destination, target op number %lld, location %lld, length %lld, sendto status is %d\n", 
			(long long int)qp->target_op_number, 
			(long long int)qp->e2e_header.location, 
			(long long int)qp->e2e_header.length, 
			qp->e2e_send_status);
	if (qp->my_current_state & CURRENT_STATE_BARRIER)
		fprintf(xgp->output,"    Inside barrier '%s'\n",qp->current_barrier->name);
	if (qp->my_current_state & CURRENT_STATE_WAITING_ANY_QTHREAD_AVAILABLE)
		fprintf(xgp->output,"    Waiting on the any_qthread_available semaphore\n");
	if (qp->my_current_state & CURRENT_STATE_WAITING_THIS_QTHREAD_AVAILABLE)
		fprintf(xgp->output,"    Waiting on the sem_this_qthread_is_available semaphore\n");
	if (qp->my_current_state & CURRENT_STATE_WAITING_FOR_PREVIOUS_QTHREAD)
		fprintf(xgp->output,"    Waiting for previous QThread '%d' task complete\n",(qp->qthread_to_wait_for)?qp->qthread_to_wait_for->my_qthread_number:-1);
	fprintf(xgp->output,"    pass_complete is '%d'\n",qp->pass_complete);
	if (qp->this_qthread_is_available)
		fprintf(xgp->output,"    This QThread is Available\n");
	if (qp->target_options & TO_ENDTOEND) { // Check to see if this is in an E2E operation and if EOF has been sent or received
		if (qp->target_options & TO_E2E_SOURCE) { // This is the Source Side
			if (qp->pass_complete) // Pass Complete means that the EOF has been sent
				fprintf(xgp->output,"    Source QThread %d has sent EOF to Deestination\n",qp->my_qthread_number);
		} else { // Must be the Destination Side
			if (qp->pass_complete) // Pass Complete means that the EOF has been received
				fprintf(xgp->output,"    Destination QThread %d has received EOF from Source\n",qp->my_qthread_number);
		}
	}
} // End of xdd_interactive_display_state_info()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_qptds()
 * Display the PTDS of the QThreads 
 */
void
xdd_interactive_show_qtptds(int32_t tokens, char *cmdline, uint32_t flags) {
	int		target_number;
	ptds_t	*p;
	ptds_t	*qp;

	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number];
		if (p) {
			qp = p->next_qp;
			while (qp) {
				fprintf(xgp->output,"Target %d Qthread %d current barrier is %p",qp->my_target_number, qp->my_qthread_number, qp->current_barrier);
				if (qp->current_barrier) 
					fprintf(xgp->output," name is '%s'",qp->current_barrier->name);
				fprintf(xgp->output,"\n");
				qp = qp->next_qp;
			}
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a PTDS\n", target_number);
		}
	}

} // End of xdd_interactive_show_qtptds()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_trace()
 * Display the data in the trace buffer of a specific QThread or a
 * 'composite' view of the trace buffers from all QThreads for a specific Target
 */
void
xdd_interactive_show_trace(int32_t tokens, char *cmdline, uint32_t flags) {

} // End of xdd_interactive_show_trace()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_barrier()
 * Display the PTDS of a specific QThread or Target Thread
 */
void
xdd_interactive_show_barrier(int32_t tokens, char *cmdline, uint32_t flags) {
	xdd_barrier_t	*bp;			// Pointer to a barrier
	xdd_occupant_t	*occupantp;		// Pointer to an occupant 
	int				i,j;			// counter


	fprintf(xgp->output, "\n--------------- Begin Barrier Information --------------\n");
	fprintf(xgp->output, "Barrier count is %d\n",xgp->barrier_count);
	fprintf(xgp->output, "Barrier_chain_first is %p\n",xgp->barrier_chain_first);
	fprintf(xgp->output, "Barrier_chain_last is %p\n",xgp->barrier_chain_last);
	bp = xgp->barrier_chain_first;
	
	fprintf(xgp->output, "Barrier Pointer, Previous, Next, Name, Counter, Threads, First Occupant, Last Occupant\n");
	for (i = 0; i < xgp->barrier_count; i++) { // Go through and display information about each barrier
		if (bp == NULL) { // Yikes! This should be a valid pointer....
			fprintf(xgp->output,"YIKES! Barrier %d has no pointer even though the count is non-zero\n",i);
			break;
		} else { // Must be a real barrier pointer - I hope...
			fprintf(xgp->output, "%16p, %16p, %16p, %64s, %04d, %04d, %16p, %16p\n",
				bp, 
				bp->prev_barrier, 
				bp->next_barrier, 
				bp->name, 
				bp->counter, 
				bp->threads, 
				bp->first_occupant, 
				bp->last_occupant);
			if (bp->counter) { // This means this barrier must be occupied....
				// Display the occupants
				occupantp = bp->first_occupant;
				for (j = 0; j < bp->counter; j++) {
					if (occupantp) {
						fprintf(xgp->output,"\tOccupant %d <%p> is %s, type %lld, ptds %p, entry %lld, prev=%p, next=%p\n", 
							j+1, 
							occupantp, 
							occupantp->occupant_name,
							(long long int)occupantp->occupant_type,
							occupantp->occupant_ptds,
							(long long int)occupantp->entry_time,
							occupantp->prev_occupant,
							occupantp->next_occupant);
						// Move along to the next occupant in the chain
						occupantp = occupantp->next_occupant; 
					} else { // Hmmmm.... no occupant pointer even though the occupant count is non-zero....
						fprintf(xgp->output,"\tYIKES! Occupant %d has no occupant pointer even though the occupant count is non-zero\n",j);
						break;
					}
				} // End of FOR loop that display all the occupants
			}
		} // End of displaying info about a specific barrier
		// Move along to the next barrier
		bp = bp->next_barrier;
	} // End of FOR loop that displays information about ALL the barriers
	fprintf(xgp->output, "\n--------------- End of Barrier Information --------------\n");

} // End of xdd_interactive_show_barrier()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_timestamp_report()
 * Generate the time stamp report 
 */
int
xdd_interactive_ts_report(int32_t tokens, char *cmdline, uint32_t flags) {
	int		target_number;
	ptds_t	*p;

	// Process TimeStamp reports for the -ts option
	for (target_number=0; target_number<xgp->number_of_targets; target_number++) { 
		p = xgp->ptdsp[target_number]; /* Get the ptds for this target */
		/* Display and write the time stamping information if requested */
		if (p->ts_options & (TS_ON | TS_TRIGGERED)) {
			if (p->ts_current_entry > p->ttp->tt_size) 
				p->ttp->numents = p->ttp->tt_size;
			else p->ttp->numents = p->ts_current_entry;
			xdd_ts_reports(p);  /* generate reports if requested */
		}
	} // End of processing TimeStamp reports
	return(0);
} // End of xdd_interactive_timestamp_report()
