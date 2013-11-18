/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines that support the Target threads.
 */
#include "xint.h"
#include "parse.h"
#include "interactive.h"
extern xdd_interactive_func_t xdd_interactive_func[];

/*----------------------------------------------------------------------------*/
/* xdd_interactive_exit()
 * Tell a all targets to abort and exit
 */
int
xdd_interactive_exit(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	xdd_occupant_t	barrier_occupant;	// Used by the xdd_barrier() function to track who is inside a barrier

	xdd_init_barrier_occupant(&barrier_occupant, "INTERACTIVE_EXIT", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);
	xgp->global_options |= GO_INTERACTIVE_EXIT;

	// Enter the FINAL barrier to tell XDD MAIN that everything is complete
	xdd_barrier(&planp->main_results_final_barrier,&barrier_occupant,0);

	pthread_exit(0);

} // End of xdd_interactive_goto()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_usage() - Display usage information
 */
void
xdd_interactive_usage(int32_t fullhelp) {
    int i,j;

    fprintf(stderr,"Available interactive commands\n");
    i = 0;
    while(xdd_interactive_func[i].func_name) {
		if (xdd_interactive_func[i].flags & XDD_INTERACTIVE_FUNC_INVISIBLE) { // This function is not visible
			i++;
			continue;
		}
        fprintf(stderr,"%s",xdd_interactive_func[i].help);
        if (fullhelp) {
            j=0;
            while (xdd_interactive_func[i].ext_help[j] && (j < XDD_INTERACTIVE_EXTRA_HELP_LINES)) {
                fprintf(stderr,"%s",xdd_interactive_func[i].ext_help[j]);
                j++;
            }
            fprintf(stderr,"\n");
        }
        i++;
    }
} // End of xdd_interactive_usage()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_help()
 * Display a list of valid commands 
 *
 */
int
xdd_interactive_help(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
    int i,j;
	int	found;


	if (tokens <= 1) {
		xdd_interactive_usage(0);
		return(0);
	}
	fprintf(stderr,"Help for the '%s' command: \n",cmdline);
    i = 0;
	found=0;
    while(xdd_interactive_func[i].func_name) {
		if ((strcmp(xdd_interactive_func[i].func_name, cmdline) == 0) ||
			(strcmp(xdd_interactive_func[i].func_alt, cmdline) == 0)) { 
			found=1;
            fprintf(stderr,"Option name is %s or abbreviated as %s",
				xdd_interactive_func[i].func_name,
				xdd_interactive_func[i].func_alt);
            fprintf(stderr,"%s",xdd_interactive_func[i].help);
            j=0;
            while (xdd_interactive_func[i].ext_help[j] && (j < XDD_INTERACTIVE_EXTRA_HELP_LINES)) {
                fprintf(stderr,"%s",xdd_interactive_func[i].ext_help[j]);
                j++;
            }
            fprintf(stderr,"\n");
        }
        i++;
    }
	if (found == 0) {
		fprintf(stderr,"xdd_help: cannot find specified option '%s' to provide help on\n", cmdline);
	    xdd_interactive_usage(0);
	}

	return(0);

} // End of xdd_interactive_help()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show()
 * Display any one of a number of things including:
 *  Data Struct for a specific Target
 *  Data Struct for a specific Worker Thread on a specific Target
 *  The trace table
 *  XDD Global Data 
 *  Data in the buffer of specific Worker Thread
 *  Worker Thread Available Queue
 *  Barrier Chain
 *
 */
int
xdd_interactive_show(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	char	*cp;

	cp = cmdline;
	while ((*cp != TAB) && (*cp != SPACE) && (*cp != '\0')) cp++;
	while ((*cp == TAB) || (*cp == SPACE) || (*cp == '\0')) cp++;
	if (strcmp(cp, "barrier") == 0) 
		xdd_interactive_show_barrier(tokens, cp, flags, planp);
	else if (strcmp(cp, "target_data") == 0) 
		xdd_interactive_show_target_data(tokens, cp, flags, planp);
	else if (strcmp(cp, "worker_data") == 0) 
		xdd_interactive_show_worker_data(tokens, cp, flags, planp);
	else if (strcmp(cp, "worker_state") == 0) 
		xdd_interactive_show_worker_state(tokens, cp, flags, planp);
	else if (strcmp(cp, "results") == 0) 
            xdd_interactive_show_results(tokens, cp, flags, planp);
	else if (strcmp(cp, "tot") == 0) 
		xdd_interactive_show_tot(tokens, cp, flags, planp);
	else if (strcmp(cp, "printtot") == 0) 
		xdd_interactive_show_print_tot(tokens, cp, flags, planp);
	else if (strcmp(cp, "trace") == 0) 
		xdd_interactive_show_trace(tokens, cp, flags, planp);
	else if (strcmp(cp, "global") == 0) 
		xdd_interactive_show_global_data(tokens, cp, flags, planp);
	else if (strcmp(cp, "data") == 0) 
		xdd_interactive_show_rwbuf(tokens, cp, flags, planp);
	else { // Show a list of what one can show
		fprintf(xgp->output,"show: please specify: barrier, target_data, worker_data, worker_state, trace, global, or data\n");
	}

	return(0);

} // End of xdd_interactive_show()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_run()
 * Tell XDD to start up the targets and run to completion
 *
 */
int
xdd_interactive_run(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

	// Signal the target threads to start running
	xdd_barrier(&planp->interactive_barrier,&planp->interactive_occupant,1);
	return(0);

} // End of xdd_interactive_run()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_step()
 * Tell XDD to run one single OP for each Target and then stop
 *
 */
int
xdd_interactive_step(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	return(0);

} // End of xdd_interactive_step()
/*----------------------------------------------------------------------------*/
/* xdd_interactive_stop()
 * Cause all Target Worker Threads to stop issuing I/O tasks and wait for either
 * a "run" or "step" command
 *
 */
int
xdd_interactive_stop(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

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
xdd_interactive_goto(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

	// Set the current operation number in each target to the specified value

	return(0);

} // End of xdd_interactive_goto()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_rwbuf()
 * Display the data in the rw buffer of a specific Worker Thread
 */
void
xdd_interactive_show_rwbuf(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

	// Display the offset and data bytes in hex and text for the data buffer of each Worker Thread

} // End of xdd_interactive_show_rwbuf()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_rwbuf()
 * Display the global_data for this run
 */
void
xdd_interactive_show_global_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

	xdd_show_global_data();

} // End of xdd_interactive_show_global_data()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_target_data()
 * Display the Data Struct of a specific Worker Thread or Target Thread
 */
void
xdd_interactive_show_target_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;

	/* Target Specific variables */
	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		if (planp->target_datap[target_number]) {
			xdd_show_target_data(planp->target_datap[target_number]);
		} else {
				fprintf(xgp->output,"ERROR: Target %d does not seem to have a TARGE_DATA structure\n", target_number);
		}
	}

} // End of xdd_interactive_show_target_data()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_worker_state()
 * Display the state of each Worker Thread
 */
void
xdd_interactive_show_worker_state(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;
	target_data_t	*tdp;
	worker_data_t	*wdp;


	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number];
		if (tdp) {
			fprintf(xgp->output,"Target %d\n",tdp->td_target_number);
			wdp = tdp->td_next_wdp;
			while (wdp) {
				fprintf(xgp->output,"Target %d Worker thread %d\n",tdp->td_target_number, wdp->wd_worker_number);
				xdd_interactive_display_state_info(wdp);
				wdp = wdp->wd_next_wdp;
			}
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a Data Struct\n", target_number);
		}
	}
} // End of xdd_interactive_show_worker_state()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_results()
 * Display the current results data
 */
void
xdd_interactive_show_results(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int				target_number;
	results_t		*rp;


	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		rp = planp->target_average_resultsp[target_number];
		if (rp) {
			xdd_show_results_data(rp,0,planp);
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a Results Data Struct\n", target_number);
		}
	}
} // End of xdd_interactive_show_results()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_display_state_info()
 * Display the state of each Worker Thread
 */
void
xdd_interactive_display_state_info(worker_data_t *wdp) {
	int64_t			tmp;
	nclk_t			now;		// Current time
	int32_t			tot_offset; // Offset into TOT
	tot_entry_t		*tep;
	target_data_t	*tdp;	// Pointer to the parent Target Thread's Data Struct


	tdp = wdp->wd_tdp;
	nclk_now(&now);		// Current time
	fprintf(xgp->output,"    Target %d Worker %d Current State is 0x%08x\n",tdp->td_target_number,wdp->wd_worker_number,wdp->wd_current_state);
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_INIT)
		fprintf(xgp->output,"    Initializing\n");
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_IO) {
		tmp = (long long int)(now - wdp->wd_counters.tc_current_op_start_time)/BILLION;
		fprintf(xgp->output,"    Doing I/O - op %lld, location %lld, started at %lld or %lld milliseconds ago, end time is %lldn",
			(long long int)wdp->wd_counters.tc_current_op_number, 
			(long long int)wdp->wd_counters.tc_current_byte_offset,
			(long long int)wdp->wd_counters.tc_current_op_start_time,
			(long long int)tmp,
			(long long int)wdp->wd_counters.tc_current_op_end_time);
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_DEST_RECEIVE) {
		fprintf(xgp->output,"    Destination Side of an E2E - waiting to receive data from Source, target op number %lld, location %lld, length %lld, recvfrom status is %d\n", 
			(long long int)wdp->wd_counters.tc_current_op_number, 
			(long long int)wdp->wd_e2ep->e2e_hdrp->e2eh_byte_offset, 
			(long long int)wdp->wd_e2ep->e2e_hdrp->e2eh_data_length, 
			wdp->wd_e2ep->e2e_recv_status);
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_SRC_SEND) {
		fprintf(xgp->output,"    Source Side of an E2E - waiting to send data to Destination, target op number %lld, location %lld, length %lld, sendto status is %d\n", 
			(long long int)wdp->wd_counters.tc_current_op_number, 
			(long long int)wdp->wd_e2ep->e2e_hdrp->e2eh_byte_offset, 
			(long long int)wdp->wd_e2ep->e2e_hdrp->e2eh_data_length, 
			wdp->wd_e2ep->e2e_send_status);
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_BARRIER) {
		fprintf(xgp->output,"    Inside barrier '%s'\n",wdp->wd_occupant_name);
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_UPDATE) {
		tot_offset = ((wdp->wd_counters.tc_current_byte_offset/tdp->td_xfer_size) % tdp->td_totp->tot_entries);
		tep = &tdp->td_totp->tot_entry[tot_offset];
		fprintf(xgp->output,"    Waiting for TOT lock to update TOT Offset %d: \n",tot_offset);
		fprintf(xgp->output,"    TOT Offset, %d, tot_op_number, %lld, tot_byte_offset, %lld, block, %lld, delta, %lld, ops/blocks\n",
			tot_offset,
			(long long int)tep->tot_op_number,
			(long long int)tep->tot_byte_offset,
			(long long int)(tep->tot_byte_offset / tdp->td_xfer_size),
			(long long int)((long long int)(wdp->wd_counters.tc_current_byte_offset - tep->tot_byte_offset) / tdp->td_xfer_size));
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_RELEASE) {
		tot_offset = ((wdp->wd_counters.tc_current_byte_offset/tdp->td_xfer_size) % tdp->td_totp->tot_entries);
		tep = &tdp->td_totp->tot_entry[tot_offset];
		fprintf(xgp->output,"    Waiting for TOT lock to release TOT Offset %d: \n",tot_offset);
		fprintf(xgp->output,"    TOT Offset, %d, tot_op_number, %lld, tot_byte_offset, %lld, block, %lld, delta, %lld, ops/blocks\n",
			tot_offset,
			(long long int)tep->tot_op_number,
			(long long int)tep->tot_byte_offset,
			(long long int)(tep->tot_byte_offset / tdp->td_xfer_size),
			(long long int)((long long int)(wdp->wd_counters.tc_current_byte_offset - tep->tot_byte_offset) / tdp->td_xfer_size));
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_TOT_LOCK_TS) {
		tot_offset = ((wdp->wd_counters.tc_current_byte_offset/tdp->td_xfer_size) % tdp->td_totp->tot_entries) - 1;
		if (tot_offset < 0) 
			tot_offset = tdp->td_totp->tot_entries - 1; // The last TOT_ENTRY
		tep = &tdp->td_totp->tot_entry[tot_offset];
		fprintf(xgp->output,"    Waiting for TOT lock to time stamp TOT Offset %d before waiting\n", tot_offset);
		fprintf(xgp->output,"    TOT Offset, %d, tot_op_number, %lld, tot_byte_offset, %lld, block, %lld, delta, %lld, ops/blocks\n",
			tot_offset,
			(long long int)tep->tot_op_number,
			(long long int)tep->tot_byte_offset,
			(long long int)(tep->tot_byte_offset / tdp->td_xfer_size),
			(long long int)((long long int)(wdp->wd_counters.tc_current_byte_offset - tep->tot_byte_offset) / tdp->td_xfer_size));
	}
	if (wdp->wd_current_state & WORKER_CURRENT_STATE_WT_WAITING_FOR_PREVIOUS_IO) {
		tot_offset = ((wdp->wd_counters.tc_current_byte_offset/tdp->td_xfer_size) % tdp->td_totp->tot_entries) - 1;
		if (tot_offset < 0) 
			tot_offset = tdp->td_totp->tot_entries - 1; // The last TOT_ENTRY
		tep = &tdp->td_totp->tot_entry[tot_offset];
		fprintf(xgp->output,"    Waiting for previous I/O at TOT Offset, %d, my current op number, %lld, my current byte offset, %lld, my current block offset, %lld, waiting for block, %lld\n",
			tot_offset,
			(long long int)wdp->wd_counters.tc_current_op_number,
			(long long int)wdp->wd_counters.tc_current_byte_offset,
			(long long int)(wdp->wd_counters.tc_current_byte_offset / tdp->td_xfer_size),
			(long long int)((long long int)(wdp->wd_counters.tc_current_byte_offset - tdp->td_xfer_size) / tdp->td_xfer_size));
		fprintf(xgp->output,"    TOT Offset, %d, tot_op_number, %lld, tot_byte_offset, %lld, block, %lld, delta, %lld, ops/blocks\n",
			tot_offset,
			(long long int)tep->tot_op_number,
			(long long int)tep->tot_byte_offset,
			(long long int)(tep->tot_byte_offset / tdp->td_xfer_size),
			(long long int)((long long int)(wdp->wd_counters.tc_current_byte_offset - tep->tot_byte_offset) / tdp->td_xfer_size));
	}
	fprintf(xgp->output,"wd_worker_thread_target_sync:");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_AVAILABLE)
		fprintf(xgp->output," AVAILABLE");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_BUSY)
		fprintf(xgp->output," BUSY");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_TARGET_WAITING)
		fprintf(xgp->output," TARGET_WAITING");
	if (wdp->wd_worker_thread_target_sync & WTSYNC_EOF_RECEIVED)
		fprintf(xgp->output," EOF_RECEIVED");
	fprintf(xgp->output,"\n");

} // End of xdd_interactive_display_state_info()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_worker_data()
 * Display the Worker Thread Data
 */
void
xdd_interactive_show_worker_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;
	target_data_t	*tdp;
	worker_data_t	*wdp;

	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number];
		if (tdp) {
			wdp = tdp->td_next_wdp;
			while (wdp) {
				fprintf(xgp->output,"Target %d Worker thread %d\n",tdp->td_target_number, wdp->wd_worker_number);
				xdd_show_worker_data(wdp);
				wdp = wdp->wd_next_wdp;
			}
		} else {
		fprintf(xgp->output,"ERROR: Target %d does not seem to have a Worker Data Struct\n", target_number);
		}
	}

} // End of xdd_interactive_show_worker_data()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_trace()
 * Display the data in the trace buffer of a specific Worker Thread or a
 * 'composite' view of the trace buffers from all Worker Threads for a specific Target
 */
void
xdd_interactive_show_trace(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {

} // End of xdd_interactive_show_trace()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_tot()
 * Display the data in the target_offset_table for a specific target
 */
void
xdd_interactive_show_tot(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;
	target_data_t	*tdp;

	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number];
		if (tdp)  {
			xdd_interactive_show_tot_display_fields(tdp,xgp->output);
		} else {
			fprintf(xgp->output,"ERROR: Target %d does not seem to have a Data Struct\n", target_number);
		}
	}
} // End of  xdd_interactive_show_tot()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_print_tot()
 * Display the data in the target_offset_table for a specific target
 */
void
xdd_interactive_show_print_tot(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;
	target_data_t	*tdp;

	for (target_number = 0; target_number < planp->number_of_targets; target_number++) {
		tdp = planp->target_datap[target_number];
		if (tdp)  {
			if (xgp->csvoutput == 0) {
				fprintf(xgp->errout,"No CSV file specified to write the TOT information to, sending to standard out\n");
				xdd_interactive_show_tot_display_fields(tdp,xgp->output);
			} else { 
				xdd_interactive_show_tot_display_fields(tdp,xgp->csvoutput);
			}
		} else {
			fprintf(xgp->output,"ERROR: Target %d does not seem to have a Data Struct\n", target_number);
		}
	}

} // End of  xdd_interactive_show_print_tot()

/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_tot_display_fields()
 * Display the data in the target_offset_table for a specific target
 */
void
xdd_interactive_show_tot_display_fields(target_data_t *tdp, FILE *fp) {

	int			i;
	char		*tot_mutex_state;
	int32_t		tot_offset; // Offset into TOT
	tot_entry_t	*tep;		// Pointer to a TOT Entry
	int			sem_val = 0;
	int			status;
	int			save_errno;
	int64_t		tot_block;
	tot_wait_t	*totwp;


	fprintf(fp,"Target %d has %d TOT Entries, queue depth of %d\n",
		tdp->td_target_number, 
		tdp->td_totp->tot_entries, 
		tdp->td_queue_depth);
	fprintf(fp,"TOT Offset,Waitp,WAIT TS,POST TS,W/P Delta,Update TS,Byte Location,Block Location,I/O Size,WaitWorkerThread,PostWorkerThread,UpdateWorkerThread,SemVal,Mutex State\n");
	for (tot_offset = 0; tot_offset < tdp->td_totp->tot_entries; tot_offset++) {
		tep = &tdp->td_totp->tot_entry[tot_offset];
		status = pthread_mutex_trylock(&tep->tot_mutex);
		if (status == 0) {
			tot_mutex_state = "unlocked";
			pthread_mutex_unlock(&tep->tot_mutex);
		} else { // Must have been locked...
			if (errno == EBUSY) 
				tot_mutex_state = "LOCKED";
			else {
				save_errno = errno;
				fprintf(fp,"Error on pthread_mutex_trylock at offset %d in the TOT\n",tot_offset);
				errno = save_errno;
				perror("Reason");
				tot_mutex_state = "UNKNOWN";
			}
		}
			
		if (tep->tot_io_size) 
			tot_block = (long long int)((long long int)tep->tot_byte_offset / tep->tot_io_size);
		else  tot_block = -1;
		fprintf(fp,"%5d,%p,%lld,%lld,%lld,%lld,%lld,%lld,%d,%d,%d,%d,%d,%s\n",
			tot_offset,
			tep->tot_waitp,
			(long long int)tep->tot_wait_ts,
			(long long int)tep->tot_post_ts,
			(long long int)(tep->tot_post_ts - (long long int)tep->tot_wait_ts),
			(long long int)tep->tot_update_ts,
			(long long int)tep->tot_byte_offset,
			(long long int)tot_block,
			tep->tot_io_size,
			tep->tot_wait_worker_thread_number,
			tep->tot_post_worker_thread_number,
			tep->tot_update_worker_thread_number,
			sem_val,
			tot_mutex_state);
			if (tep->tot_waitp) {
				totwp = tep->tot_waitp;
				i=0;
 				fprintf(fp,"TOT Offset,TOTW,TOTW#,tot_wait ptr,next tot_wait ptr,Worker Data ptr,condition,Is Released\n");
				while (totwp) {
   					fprintf(fp,"%5d,TOTW,%d,%p,%p,%p,-,%d\n",tot_offset,i,totwp,totwp->totw_nextp,totwp->totw_wdp,totwp->totw_is_released);
					totwp = totwp->totw_nextp;
					i++;
				sleep(1);
				}
			}
	} // End of FOR loop that displays all the TOT entries
} // End of xdd_interactive_show_tot_display_fields()


/*----------------------------------------------------------------------------*/
/* xdd_interactive_show_barrier()
 * Display the Data Struct of a specific Worker Thread or Target Thread
 */
void
xdd_interactive_show_barrier(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	xdd_barrier_t	*bp;			// Pointer to a barrier
	xdd_occupant_t	*occupantp;		// Pointer to an occupant 
	int				i,j;			// counter


	fprintf(xgp->output, "\n--------------- Begin Barrier Information --------------\n");
	fprintf(xgp->output, "Barrier count is %d\n",planp->barrier_count);
	fprintf(xgp->output, "Barrier_chain_first is %p\n",planp->barrier_chain_first);
	fprintf(xgp->output, "Barrier_chain_last is %p\n",planp->barrier_chain_last);
	bp = planp->barrier_chain_first;
	
	fprintf(xgp->output, "Barrier Pointer, Previous, Next, Name, Counter, Threads, First Occupant, Last Occupant\n");
	for (i = 0; i < planp->barrier_count; i++) { // Go through and display information about each barrier
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
						fprintf(xgp->output,"\tOccupant %d <%p> is %s, type %lld, target/worker data pointer %p, entry %lld, prev=%p, next=%p\n", 
							j+1, 
							occupantp, 
							occupantp->occupant_name,
							(long long int)occupantp->occupant_type,
							(void *)occupantp->occupant_data,
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
xdd_interactive_ts_report(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp) {
	int		target_number;
	struct	xint_target_data	*tdp;

	// Process TimeStamp reports for the -ts option
	for (target_number=0; target_number<planp->number_of_targets; target_number++) { 
		tdp = planp->target_datap[target_number]; /* Get the Target Data Pointer for this target */
		/* Display and write the time stamping information if requested */
		if (tdp->td_ts_table.ts_options & (TS_ON | TS_TRIGGERED)) {
			if (tdp->td_ts_table.ts_current_entry > tdp->td_ts_table.ts_size) 
				tdp->td_ts_table.ts_hdrp->tsh_numents = tdp->td_ts_table.ts_size;
			else tdp->td_ts_table.ts_hdrp->tsh_numents = tdp->td_ts_table.ts_current_entry;
			xdd_ts_reports(tdp);  /* generate reports if requested */
		}
	} // End of processing TimeStamp reports
	return(0);
} // End of xdd_interactive_timestamp_report()
