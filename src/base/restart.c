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
 *//*
 * This file contains the subroutines necessary to manage everything to
 * do with restarting an xddcp operation.
 */
#include "xint.h"

// Prototypes
int xdd_restart_create_restart_file(xint_restart_t *rp);
int xdd_restart_write_restart_file(xint_restart_t *rp);

/*----------------------------------------------------------------------------*/
// This routine is called to create a new restart file when a new copy 
// operation is started for the first time.
// The filename of the restart file can be specified by the "-restart file xxx"
// option where xxx will be the file name of the restart file. 
// If the restart file name is not specified then the default location and 
// name will be used. Currently, the default location is the current working
// directory where xddcp is being executed and the file name will be 
//     xdd.$src.$src_basename.$dest.$dest_basename.$gmt_timestamp-GMT.$ext
// where $src is the host name of the source machine
//       $src_basename is the base name of the source file
//       $dest is the host name of the destination machine
//       $dest_basename is the base name of the destination file
//       $gmt_timestamp is the time at which this restart file was created in
//         the form YYYY-MM-DD-hhmm-GMT or year-month-day-hourminutes in GMT
//         and the "-GMT" is appended to the timestamp to indicate this timezone
//       $ext is the file extension which is ".rst" for this type of file
//
int 
xdd_restart_create_restart_file(xint_restart_t *rp) {

	time_t	t;				// Time structure
	struct 	tm	*tm;		// Pointer to the broken-down time struct that lives in the restart struct
	
 
	// Check to see if the file name was provided or not. If not, the create a file name.
	if (rp->restart_filename == NULL) { // Need to create the file name here
		rp->restart_filename = malloc(MAX_TARGET_NAME_LENGTH);
		if (rp->restart_filename == NULL) {
			fprintf(xgp->errout,"%s: RESTART_MONITOR: ALERT: Cannot allocate %d bytes of memory for the restart file name\n",
				xgp->progname,
				MAX_TARGET_NAME_LENGTH);
			perror("Reason");
			rp->fp = xgp->errout;
			return(0);
		}
		// Get the current time in a appropriate format for a file name
		time(&t);
		tm = gmtime_r(&t, &rp->tm);
		sprintf(rp->restart_filename,"xdd.%s.%s.%s.%s.%4d-%02d-%02d-%02d%02d-GMT.rst",
			(rp->source_host==NULL)?"NA":rp->source_host,
			(rp->source_filename==NULL)?"NA":basename(rp->source_filename),
			(rp->destination_host==NULL)?"NA":rp->destination_host,
			(rp->destination_filename==NULL)?"NA":basename(rp->destination_filename),
			tm->tm_year+1900, // number of years since 1900
			tm->tm_mon+1,  // month since January - range 0 to 11 
			tm->tm_mday,   // day of the month range 1 to 31
			tm->tm_hour,
			tm->tm_min);
	}

	// Now that we have a file name lets try to open it in purely write mode.
	rp->fp = fopen(rp->restart_filename,"w");
	if (rp->fp == NULL) {
		fprintf(xgp->errout,"%s: RESTART_MONITOR: ALERT: Cannot create restart file %s!\n",
			xgp->progname,
			rp->restart_filename);
		perror("Reason");
		fprintf(xgp->errout,"%s: RESTART_MONITOR: ALERT: Defaulting to error out for restart file\n",
			xgp->progname);
		rp->fp = xgp->errout;
		rp->restart_filename = NULL;
		free(rp->restart_filename);
		return(-1);
	}

	// And write the initial restart value
	if (0 != xdd_restart_write_restart_file(rp)) {
		fprintf(xgp->errout,"%s: Restart file corrupted: %s  Previous restart offset was %lld\n",
				xgp->progname,
				rp->restart_filename,
				(long long int)rp->initial_restart_offset);
	}
	
	// Success - everything must have worked and we have a restart file
	fprintf(xgp->output,"%s: RESTART_MONITOR: INFO: Successfully created restart file %s\n",
		xgp->progname,
		rp->restart_filename);
	return(0);
} // End of xdd_restart_create_restart_file()

/*----------------------------------------------------------------------------*/
// xdd_restart_write_restart_file() - Update the restart file with current info
// 
// This routine is called to write new information to an existing restart file 
// during a copy operation - this is also referred to as a "checkpoint"
// operation. 
// Each time the write is performed to this file it is flushed to the storage device. 
// 
// 
int
xdd_restart_write_restart_file(xint_restart_t *rp) {
	int		status;
	long long int restart_offset;

	// Determine the new offset
	if (rp->byte_offset > rp->initial_restart_offset) {
		restart_offset = rp->byte_offset;
	}
	else {
		restart_offset = rp->initial_restart_offset;
	}
	
	// Seek to the beginning of the file 
	status = fseek(rp->fp, 0L, SEEK_SET);
	if (status < 0) {
		fprintf(xgp->errout,"%s: RESTART_MONITOR: WARNING: Seek to beginning of restart file %s failed\n",
			xgp->progname,
			rp->restart_filename);
		perror("Reason");
	}
	
	// Put the ASCII text offset information into the restart file
	fprintf(rp->fp,"-restart offset %lld\n", (long long int)restart_offset);

	// Flush the file for safe keeping
	fflush(rp->fp);

	return(0);
} // End of xdd_restart_write_restart_file()

/*----------------------------------------------------------------------------*/
// This routine is created when xdd starts a copy operation (aka xddcp).
// This routine will run in the background and waits for various xdd I/O
// Worker Threads to update their respective counters and enter the barrier to 
// notify this routine that a block has been read/written.
// This monitor runs on both the source and target machines during a copy
// operation as is indicated in the name of the restart file. The information
// contained in the restart file has different meanings depending on which side
// of the copy operation the restart file represents. 
// On the source side, the restart file contains information regarding the lowest 
// and highest byte offsets into the source file that have been successfully 
// "read" and "sent" to the destination machine. This does not mean that the 
// destination machine has actually received the data but it is an indicator of 
// what the source side thinks has happened.
// On the destination side the restart file contains information regarding the
// lowest and highest byte offsets and lengths that have been received and 
// written (committed) to stable media. This is the file that should be used
// for an actual copy restart opertation as it contains the most accurate state
// of the destination file.
//
// This routine will also display information about the copy operation before,
// during, and after xddcp is complete. 
// 
void *
xdd_restart_monitor(void *data) {
	int				target_number;			// Used as a counter
	target_data_t			*current_tdp;	// The current Target Thread Data Struct
	int64_t 		check_counter;			// The current number of times that we have checked on the progress of a copy
	xdd_occupant_t	barrier_occupant;		// Used by the xdd_barrier() function to track who is inside a barrier
	xint_restart_t		*rp;
	int				status;					// Status of mutex init/lock/unlock calls
	//tot_entry_t		*tep;					// Pointer to an entry in the Target Offset Table
	//int				te;						// TOT Entry number
	//int64_t			lowest_offset;			// Lowest Offset in bytes 
	xdd_plan_t* planp = (xdd_plan_t*)data;



	// Initialize stuff
	if (xgp->global_options & GO_REALLYVERBOSE)
		fprintf(xgp->output,"%s: xdd_restart_monitor: Initializing...\n", xgp->progname);

	for (target_number=0; target_number < planp->number_of_targets; target_number++) {
		current_tdp = planp->target_datap[target_number];
		rp = current_tdp->td_restartp;
		status = pthread_mutex_init(&rp->restart_lock, 0);
		if (status) {
			fprintf(stderr,"%s: xdd_restart_monitor: ERROR initializing restart_lock for target %d, status=%d, errno=%d", 
				xgp->progname, 
				target_number, 
				status, 
				errno);
			perror("Reason");
			return(0);
		}
		if (current_tdp->td_target_options & TO_E2E_DESTINATION) {
			xdd_restart_create_restart_file(rp);
		} else {
			fprintf(xgp->output,"%s: xdd_restart_monitor: INFO: No restart file being created for target %d [ %s ] because this is not the destination side of an E2E operation.\n", 
				xgp->progname,
				current_tdp->td_target_number,
				current_tdp->td_target_full_pathname);
		}
	}
	if (xgp->global_options & GO_REALLYVERBOSE)
		fprintf(xgp->output,"%s: xdd_restart_monitor: Initialization complete.\n", xgp->progname);

	// Enter this barrier to release main indicating that restart has initialized
	xdd_init_barrier_occupant(&barrier_occupant, "RESTART_MONITOR", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);
	xdd_barrier(&planp->main_general_init_barrier,&barrier_occupant,0);

	check_counter = 0;
	// This is the loop that periodically checks all the targets/Worker Threads 
	for (;;) {
		// Sleep for the specified period of time
		sleep(planp->restart_frequency);

		// If it is time to leave then leave - the Worker Thread cleanup will take care of closing the restart files
		if (xgp->abort | xgp->canceled) 
			break;

		check_counter++;
		// Check all targets
		for (target_number=0; target_number < planp->number_of_targets; target_number++) {
			current_tdp = planp->target_datap[target_number];
			// If this target does not require restart monitoring then continue
			if ( !(current_tdp->td_target_options & TO_RESTART_ENABLE) ) {
                // if restart is NOT enabled for this target then continue
				continue;
			}
			rp = current_tdp->td_restartp;
			if (!rp) {
                // Hmmm... no restart pointer..
				continue;
			}
			pthread_mutex_lock(&rp->restart_lock);

			if (rp->flags & RESTART_FLAG_SUCCESSFUL_COMPLETION) {
				pthread_mutex_unlock(&rp->restart_lock);
				continue;
			} else {
				// Put the "Last Committed Block" information in the restart structure...
				// In the case of restart processing it is NOT ok to use the target
				// offset table to determine the restart number.  Rather, we have to
				// determine how many worker threads exist, and use the smallest offset
				// in the worker tasks as the restart offset
				struct xint_worker_data *cur_wdp, *restart_wdp;
				cur_wdp = restart_wdp = current_tdp->td_next_wdp;
				while ((cur_wdp = cur_wdp->wd_next_wdp)) {
					/* Find the lowest byte offset */
					if (cur_wdp->wd_task.task_byte_offset < restart_wdp->wd_task.task_byte_offset) {
						restart_wdp = cur_wdp;
					}
				}
					
				/* The located task effectively becomes the restart point */
				rp->byte_offset = restart_wdp->wd_task.task_byte_offset;
				rp->last_committed_byte_offset = -1;
				rp->last_committed_length = restart_wdp->wd_task.task_xfer_size;
				rp->last_committed_op = -1;
				/*
				// In the case of Restart processing it is ok to use the tot_byte_offset
				// rather than the tot_op_number.
				// Scan the Target Offset Table (TOT) for the *lowest* committed offset 
				// then add N blocks to that number where N = number of TOT entries
				// to produce the *byte_offset* which is where to start writing 
				// data into the file when restarted.
				lowest_offset = LONGLONG_MAX;
				for (te = 0; te < current_tdp->td_totp->tot_entries; te++) {
					tep = &current_tdp->td_totp->tot_entry[te];
					if (tep->tot_byte_offset < 0) {
						// A byte_offset of -1 indicates that this TOT Entry has not been updated yet.
						// Thus the number of bytes of data processed up to this point is equal to
						// te-1 times the iosize. 
						if (te == 0) { // No restart offset is available yet
							// The *byte_offset* is the offset into the file where the first new byte should be written
							rp->byte_offset = 0;
							// The *last_committed_byte_offset* is the offset into the file where the last block of data was written
							rp->last_committed_byte_offset = -1;
							rp->last_committed_length = -1;
							rp->last_committed_op = -1;
						} else { // Restart point is te times the iosize
							// The *byte_offset* is the offset into the file where the first new byte should be written
							rp->byte_offset = (long long int)te * (long long int)current_tdp->td_xfer_size;
							// The *last_committed_byte_offset* is the offset into the file where the last block of data was written
							rp->last_committed_byte_offset = (long long int)(te - 1) * (long long int)current_tdp->td_xfer_size;
							rp->last_committed_length = (long long int)current_tdp->td_xfer_size;
							rp->last_committed_op = rp->last_committed_byte_offset / (long long int)current_tdp->td_xfer_size;
						}
						break;
					} else {
						// Check to see if this is the lowest-numbered offset and use it as the restart point if it is.
						if (tep->tot_byte_offset < lowest_offset) {
							lowest_offset = tep->tot_byte_offset;
							// The *byte_offset* is the offset into the file where the first new byte should be written
							rp->byte_offset = tep->tot_byte_offset + (long long int)((long long int)current_tdp->td_xfer_size * (long long int)current_tdp->td_totp->tot_entries);
							// The *last_committed_byte_offset* is the offset into the file where the last block of data was written
							rp->last_committed_byte_offset = tep->tot_byte_offset + ((long long int)current_tdp->td_xfer_size *(long long int)(current_tdp->td_totp->tot_entries-1));
							rp->last_committed_length = (long long int)tep->tot_io_size;
							rp->last_committed_op = rp->last_committed_byte_offset / (long long int)current_tdp->td_xfer_size;
						}
					}
					
				} // End of FOR loop that scans the TOT for the restart offset to use
	            */
				// ...and write it to the restart file and sync sync sync
				if (current_tdp->td_target_options & TO_E2E_DESTINATION) // Restart files are only written on the destination side
					xdd_restart_write_restart_file(rp);

			}
			// UNLOCK the restart struct
			pthread_mutex_unlock(&rp->restart_lock);

		} // End of FOR loop that checks all targets 

		// If it is time to leave then leave - the Worker Thread cleanup will take care of closing the restart files
		if (xgp->abort | xgp->canceled) 
			break;
	} // End of FOREVER loop that checks stuff

	fprintf(xgp->output,"%s: RESTART Monitor is exiting\n",xgp->progname);
	return(0);

} // End of xdd_restart_monitor() thread

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
