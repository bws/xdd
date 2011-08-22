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
 * This file contains the subroutines that display information about the
 *    - System
 *    - Target(s)
 *    - Configuration
 *    - memory usage
 *    - Environment
 */
#include "xdd.h"

/*----------------------------------------------------------------------------*/
/* xdd_display_kmgt() - Display the given quantity in either KBytes, MBytes, GBytes, or TBytes.
 */
void
xdd_display_kmgt(FILE *out, long long int n, int block_size) {

	if (n <= 0) {
		fprintf(out," 0\n");
		return;
	}
	fprintf(out, "\n\t\t\t%lld, %d-byte Blocks", (long long int)(n), block_size);
	fprintf(out, "\n\t\t\t%15lld,     Bytes", (long long int)(n));
	fprintf(out, "\n\t\t\t%19.3f, KBytes", (double)(n/FLOAT_KILOBYTE));
	fprintf(out, "\n\t\t\t%19.3f, MBytes", (double)(n/FLOAT_MEGABYTE));
	fprintf(out, "\n\t\t\t%19.3f, GBytes", (double)(n/FLOAT_GIGABYTE));
	fprintf(out, "\n\t\t\t%19.3f, TBytes\n", (double)(n/FLOAT_TERABYTE));

} // End of xdd_display_kmgt()

/*----------------------------------------------------------------------------*/
/* xdd_system_info() - Display information about the system this program
 * is being run on. This includes hardware, software, and environmental info.
 */
void
xdd_system_info(FILE *out) {
#if (SOLARIS || IRIX || LINUX || AIX || FREEBSD)
	int32_t page_size;
	int32_t physical_pages;
	int32_t memory_size;
	struct rusage xdd_rusage;
	char	*userlogin;
#if ( IRIX )
	inventory_t *inventp;
	int64_t mem_size;
#endif // IRIX inventory
	uname(&xgp->hostname);
	userlogin = getlogin();
	if (!userlogin)
		userlogin = "***unknown user login***";
	fprintf(out, "Computer Name, %s, User Name, %s\n",xgp->hostname.nodename, userlogin);
	fprintf(out, "OS release and version, %s %s %s\n",xgp->hostname.sysname, xgp->hostname.release, xgp->hostname.version);
	fprintf(out, "Machine hardware type, %s\n",xgp->hostname.machine);
#if (SOLARIS)
	xgp->number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
	xgp->clock_tick = sysconf(_SC_CLK_TCK);
#elif (AIX)
	xgp->number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
	xgp->clock_tick = sysconf(_SC_CLK_TCK);
#elif (LINUX)
	getrusage(RUSAGE_SELF, &xdd_rusage);
 //fprintf(stderr," maxrss  %d\n",xdd_rusage.ru_maxrss);        /* maximum resident set size */
 //fprintf(stderr," ixrss   %d\n",xdd_rusage.ru_ixrss);         /* integral shared memory size */
 //fprintf(stderr," idrss   %d\n",xdd_rusage.ru_idrss);         /* integral unshared data size */
 //fprintf(stderr," isrss   %d\n",xdd_rusage.ru_isrss);         /* integral unshared stack size */
 //fprintf(stderr," minflt  %d\n",xdd_rusage.ru_minflt);        /* page reclaims */
 //fprintf(stderr," majflt  %d\n",xdd_rusage.ru_majflt);        /* page faults */
 //fprintf(stderr," nswap   %d\n",xdd_rusage.ru_nswap);         /* swaps */
 //fprintf(stderr," inblock %d\n",xdd_rusage.ru_inblock);       /* block input operations */
 //fprintf(stderr," oublock %d\n",xdd_rusage.ru_oublock);       /* block output operations */
 //fprintf(stderr," msgsnd  %d\n",xdd_rusage.ru_msgsnd);        /* messages sent */
 //fprintf(stderr," msgrcv  %d\n",xdd_rusage.ru_msgrcv);        /* messages received */
 //fprintf(stderr," nsignals %d\n",xdd_rusage.ru_nsignals);      /* signals received */
 //fprintf(stderr," nvcsw   %d\n",xdd_rusage.ru_nvcsw);         /* voluntary context switches */
 //fprintf(stderr," nivcsw  %d\n",xdd_rusage.ru_nivcsw);        /* involuntary context switches */
	xgp->number_of_processors = xdd_linux_cpu_count();
	physical_pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGE_SIZE);
	xgp->clock_tick = sysconf(_SC_CLK_TCK);
#elif (IRIX )
	xgp->number_of_processors = sysconf(_SC_NPROC_ONLN);
	page_size = sysconf(_SC_PAGE_SIZE);
	xgp->clock_tick = sysconf(_SC_CLK_TCK);
	physical_pages = 0;
	setinvent();
	inventp = getinvent();
	while (inventp) {
		if ((inventp->inv_class == INV_MEMORY) && 
			(inventp->inv_type == INV_MAIN_MB)) {
			mem_size = inventp->inv_state;
			mem_size *= (1024 * 1024);
			physical_pages = mem_size / page_size;
			break;
		}
		inventp = getinvent();
	}
#endif
	fprintf(out, "Number of processors on this system, %d\n",xgp->number_of_processors);
	memory_size = (physical_pages * (page_size/1024))/1024;
	fprintf(out, "Page size in bytes, %d\n",page_size);
	fprintf(out, "Number of physical pages, %d\n", physical_pages);
	fprintf(out, "Megabytes of physical memory, %d\n", memory_size);
	fprintf(out, "Clock Ticks per second, %d\n", xgp->clock_tick);
#elif (WIN32)
	SYSTEM_INFO system_info; /* Structure to receive system information */
	OSVERSIONINFOEXA osversion_info;
	char computer_name[256];
	DWORD szcomputer_name = sizeof(computer_name);
	char user_name[256];
	DWORD szuser_name = sizeof(user_name);
	MEMORYSTATUS memorystatus;
	BOOL i;
	LPVOID lpMsgBuf;


	GetSystemInfo(&system_info);
	osversion_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	i = GetVersionEx((LPOSVERSIONINFOA)&osversion_info);
	if (i == 0) { 
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(xgp->errout,"%s: Error getting version\n",xgp->progname);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
	}
	GetComputerName(computer_name,&szcomputer_name);
	GetUserName(user_name,&szuser_name);
	GlobalMemoryStatus(&memorystatus);
	xgp->clock_tick = sysconf(_SC_CLK_TCK);
	fprintf(out, "Computer Name, %s, User Name, %s\n",computer_name, user_name);
	fprintf(out, "Operating System Info: %s %d.%d Build %d %s\n",
		((osversion_info.dwPlatformId == VER_PLATFORM_WIN32_NT) ? "NT":"Windows95"),
		osversion_info.dwMajorVersion, osversion_info.dwMinorVersion,
		osversion_info.dwBuildNumber,
		osversion_info.szCSDVersion);
	fprintf(out, "Page size in bytes, %d\n",system_info.dwPageSize);
	fprintf(out, "Number of processors on this system, %d\n", system_info.dwNumberOfProcessors);
	fprintf(out, "Megabytes of physical memory, %d\n", memorystatus.dwTotalPhys/(1024*1024));
#endif // WIN32

	fprintf(out,"Seconds before starting, %lld\n",(long long)xgp->gts_seconds_before_starting);

} /* end of xdd_system_info() */

/*----------------------------------------------------------------------------*/
/* xdd_options_info() - Display command-line options information about this run.
 */
void
xdd_options_info(FILE *out) {
	char *c; 


	fprintf(out,"IOIOIOIOIOIOIOIOIOIOI XDD version %s based on %s IOIOIOIOIOIOIOIOIOIOIOI\n",
                PACKAGE_STRING, PACKAGE_VERSION);
	fprintf(out,"%s\n",XDD_COPYRIGHT);
	fprintf(out,"%s\n",XDD_DISCLAIMER);
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(out,"Starting time for this run, %s\n",c);
	fprintf(out,"ID for this run, '%s'\n", xgp->id);
	fprintf(out,"Maximum Process Priority, %s", (xgp->global_options & GO_MAXPRI)?"enabled\n":"disabled\n");
	fprintf(out, "Passes, %d\n", xgp->passes);
	fprintf(out, "Pass Delay in seconds, %f\n", xgp->pass_delay); 
	fprintf(out, "Maximum Error Threshold, %lld\n", (long long)xgp->max_errors);
	fprintf(out, "Target Offset, %lld\n",(long long)xgp->target_offset);
	fprintf(out, "I/O Synchronization, %d\n", xgp->syncio);
	fprintf(out, "Total run-time limit in seconds, %f\n", xgp->run_time);

	fprintf(out, "Output file name, %s\n",xgp->output_filename);
	fprintf(out, "CSV output file name, %s\n",xgp->csvoutput_filename);
	fprintf(out, "Error output file name, %s\n",xgp->errout_filename);
	if (xgp->global_options & GO_COMBINED)
		fprintf(out,"Combined output file name, %s\n",xgp->combined_output_filename);
	fprintf(out,"Pass synchronization barriers, %s", (xgp->global_options & GO_NOBARRIER)?"disabled\n":"enabled\n");
	if (xgp->gts_hostname) {
			fprintf(out,"Timeserver hostname, %s\n", xgp->gts_hostname);
			fprintf(out,"Timeserver port number, %d\n", xgp->gts_port);
			fprintf(out,"Global start time, %lld\n", (long long)xgp->gts_time/BILLION);
	}
	fprintf(out,"Number of Targets, %d\n",xgp->number_of_targets);
	fprintf(out,"Number of I/O Threads, %d\n",xgp->number_of_iothreads);
	if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(out, "Size of PTDS is %d bytes, %d Aggregate\n",(int)sizeof(ptds_t), (int)sizeof(ptds_t)*xgp->number_of_iothreads);
		fprintf(out, "Size of RESULTS is %d bytes, %d Aggregate\n",(int)sizeof(results_t), (int)sizeof(results_t)*(xgp->number_of_iothreads*2+xgp->number_of_targets));
	}
	fprintf(out, "\n");
	fflush(out);
} /* end of xdd_options_info() */

/*----------------------------------------------------------------------------*/
/* xdd_target_info() - Display command-line options information about this run.
 */
void
xdd_target_info(FILE *out, ptds_t *p) {
	int 				i;
	ptds_t 				*mp, *sp; /* Master and Slave ptds pointers */
	xdd_data_pattern_t	*dpp;


	fprintf(out,"\tTarget number, %d\n",p->my_target_number);
	fprintf(out,"\t\tFully qualified target pathname, '%s'\n",p->target_full_pathname);
	fprintf(out,"\t\tTarget directory, %s\n",(strlen(p->target_directory)==0)?"\"./\"":p->target_directory);
	fprintf(out,"\t\tProcess ID, %d\n",p->my_pid);
	fprintf(out,"\t\tThread ID, %d\n",p->my_thread_id);
    if (p->processor == -1) 
		    fprintf(out,"\t\tProcessor, all/any\n");
	else fprintf(out,"\t\tProcessor, %d\n",p->processor);
	fprintf(out,"\t\tRead/write ratio, %5.2f READ, %5.2f WRITE\n",p->rwratio*100.0,(1.0-p->rwratio)*100.0);
	fprintf(out,"\t\tNetwork Operation Ordering is,");
	if (p->target_options & TO_ORDERING_NETWORK_SERIAL) 
		fprintf(out,"serial\n");
	else if (p->target_options & TO_ORDERING_NETWORK_LOOSE) 
		fprintf(out,"loose\n");
	else fprintf(out,"none\n");
	fprintf(out,"\t\tStorage Operation Ordering is,");
	if (p->target_options & TO_ORDERING_STORAGE_SERIAL) 
		fprintf(out,"serial\n");
	else if (p->target_options & TO_ORDERING_STORAGE_LOOSE) 
		fprintf(out,"loose\n");
	else fprintf(out,"none\n");
	fprintf(out,"\t\tThrottle in %s is, %6.2f\n",
		(p->throttle_type & PTDS_THROTTLE_OPS)?"ops/sec":((p->throttle_type & PTDS_THROTTLE_BW)?"MB/sec":"Delay"), p->throttle);
	fprintf(out,"\t\tPer-pass time limit in seconds, %f\n",p->time_limit);
	fprintf(out,"\t\tPass seek randomization, %s", (p->target_options & TO_PASS_RANDOMIZE)?"enabled\n":"disabled\n");
	fprintf(out,"\t\tFile write synchronization, %s", (p->target_options & TO_SYNCWRITE)?"enabled\n":"disabled\n");
	fprintf(out,"\t\tBlocksize in bytes, %d\n", p->block_size);
	fprintf(out,"\t\tRequest size, %d, %d-byte blocks, %d, bytes\n",p->reqsize,p->block_size,p->reqsize*p->block_size);
	fprintf(out,"\t\tNumber of Operations, %lld\n", (long long int)p->target_ops);

	// Total Data Transfer for this TARGET
	fprintf(out, "\t\tTotal data transfer for this TARGET, ");
	xdd_display_kmgt(out, p->target_bytes_to_xfer_per_pass, p->block_size);

	// Start Offset
	fprintf(out, "\t\tStart offset,  ");
	xdd_display_kmgt(out, p->start_offset*p->block_size, p->block_size);

	// Pass Offset
	fprintf(out, "\t\tPass offset,  ");
	xdd_display_kmgt(out, p->pass_offset*p->block_size, p->block_size);

	// Seek Range
	if (p->seekhdr.seek_range > 0) {
		fprintf(out, "\t\tSeek Range,  ");
		xdd_display_kmgt(out, p->seekhdr.seek_range*p->block_size, p->block_size);
	}
	fprintf(out, "\t\tSeek pattern, %s\n", p->seekhdr.seek_pattern);
	if (p->seekhdr.seek_stride > p->reqsize) 
		fprintf(out, "\t\tSeek Stride, %d, %d-byte blocks, %d, bytes\n",p->seekhdr.seek_stride,p->block_size,p->seekhdr.seek_stride*p->block_size);
	fprintf(out, "\t\tFlushwrite interval, %lld\n", (long long)p->flushwrite);
	fprintf(out,"\t\tI/O memory buffer is %s\n", 
		(p->target_options & TO_SHARED_MEMORY)?"a shared memory segment":"a normal memory buffer");
	fprintf(out,"\t\tI/O memory buffer alignment in bytes, %d\n", p->mem_align);
	if (p->dpp) {
		dpp = p->dpp;
		fprintf(out,"\t\tData pattern in buffer");
		if (dpp->data_pattern_options & (DP_RANDOM_PATTERN | DP_SEQUENCED_PATTERN | DP_INVERSE_PATTERN | DP_ASCII_PATTERN | DP_HEX_PATTERN | DP_PATTERN_PREFIX)) {
			if (dpp->data_pattern_options & DP_RANDOM_PATTERN) fprintf(out,",random ");
			if (dpp->data_pattern_options & DP_SEQUENCED_PATTERN) fprintf(out,",sequenced ");
			if (dpp->data_pattern_options & DP_INVERSE_PATTERN) fprintf(out,",inversed ");
			if (dpp->data_pattern_options & DP_ASCII_PATTERN) fprintf(out,",ASCII: '%s' <%d bytes> %s ",
				dpp->data_pattern,(int)dpp->data_pattern_length, (dpp->data_pattern_options & DP_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
			if (dpp->data_pattern_options & DP_HEX_PATTERN) {
				fprintf(out,",HEX: 0x");
				for (i=0; i<dpp->data_pattern_length; i++) 
					fprintf(out,"%02x",dpp->data_pattern[i]);
				fprintf(out, " <%d bytes>, %s\n",
					(int)dpp->data_pattern_length, (dpp->data_pattern_options & DP_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
			}
			if (dpp->data_pattern_options & DP_PATTERN_PREFIX)  {
				fprintf(out,",PREFIX: 0x");
				for (i=0; i<dpp->data_pattern_prefix_length; i+=2) 
					fprintf(out,"%02x",dpp->data_pattern_prefix[i]);
				fprintf(out, " <%d nibbles>\n", (int)dpp->data_pattern_prefix_length);
			}
		} else { // Just display the one-byte hex pattern 
			fprintf(out,",0x%02x\n",dpp->data_pattern[0]);
		}
		if (dpp->data_pattern_options & DP_FILE_PATTERN) 
			fprintf(out," From file: %s\n",dpp->data_pattern_filename);
	}
	fprintf(out,"\t\tData buffer verification is");
	if ((p->target_options & (TO_VERIFY_LOCATION | TO_VERIFY_CONTENTS)))
		fprintf(out," enabled for %s verification.\n", (p->target_options & TO_VERIFY_LOCATION)?"Location":"Content");
	else fprintf(out," disabled.\n");
	fprintf(out,"\t\tDirect I/O, %s", (p->target_options & TO_DIO)?"enabled\n":"disabled\n");
	fprintf(out, "\t\tPreallocation, %lld\n",(long long int)p->preallocate);
	fprintf(out, "\t\tQueue Depth, %d\n",p->queue_depth);
	/* Timestamp options */
	if (p->ts_options & TS_ON) {
		fprintf(out, "\t\tTimestamping, enabled for %s %s\n",(p->ts_options & TS_DETAILED)?"DETAILED":"", (p->ts_options & TS_SUMMARY)?"SUMMARY":"");
		fprintf(out, "\t\tTimestamp ASCII output file name, %s\n",p->ts_output_filename);
		if (p->ts_options & TS_DUMP) 
			fprintf(out, "\t\tTimestamp binary output file name, %s\n",p->ts_binary_filename);
	} else fprintf(out, "\t\tTimestamping, disabled\n");

	// Print the heartbeat interval and options
	if (p->hb.hb_interval > 0) { // Display the Heartbeat information
		fprintf(out, "\t\tHeartbeat Interval, %d, options, ", p->hb.hb_interval);
		if (p->hb.hb_options & HB_TOD)  // Time of Day
			fprintf(out,"/TOD");
		if (p->hb.hb_options & HB_ELAPSED)  // Elapsed Seconds for run
			fprintf(out,"/ELAPSED");
		if (p->hb.hb_options & HB_HOST)  	// Host name of this computer
			fprintf(out,"/HOSTNAME");
		if (p->hb.hb_options & HB_TARGET)  	// Display the target number of this info
			fprintf(out,"/Target#");
		if (p->hb.hb_options & HB_OPS)  	// Display current number of OPS performed 
			fprintf(out,"/Ops");
		if (p->hb.hb_options & HB_BYTES)  	// Display current number of BYTES transferred 
			fprintf(out,"/Bytes");
		if (p->hb.hb_options & HB_KBYTES)  	// Display current number of KILOBYTES transferred 
			fprintf(out,"/KBytes");
		if (p->hb.hb_options & HB_MBYTES)  	// Display current number of MEGABYTES transferred 
			fprintf(out,"/MBytes");
		if (p->hb.hb_options & HB_GBYTES)  	// Display current number of GIGABYTES transferred 
			fprintf(out,"/GBytes");
		if (p->hb.hb_options & HB_BANDWIDTH)// Display current Aggregate BANDWIDTH 
			fprintf(out,"/Bandwidth");
		if (p->hb.hb_options & HB_IOPS)  	// Display current Aggregate IOPS 
			fprintf(out,"/IOPS");
		if (p->hb.hb_options & HB_PERCENT)  // Display Percent Complete 
			fprintf(out,"/PercentComplete");
		if (p->hb.hb_options & HB_ET)  		// Display Estimated Time to Completion
			fprintf(out,"/EstimatedTimeLeft");
		if (p->hb.hb_options & HB_LF)  		// LineFeed or Carriage Return
			fprintf(out,"/LF");
		fprintf(out,"\n");
		if (p->hb.hb_filename)  			// display name of the file that Heartbeat will send data
		 	fprintf(out,"\t\tHeartbeart Output File, %s\n",p->hb.hb_filename);
		else fprintf(out,"\t\tHeartbeart Output File, stderr\n");
	} else {
		fprintf(out, "\t\tHeartbeat Disabled, \n");
	}
	fflush(out);
	fprintf(out,"\t\tDelete file, %s", (p->target_options & TO_DELETEFILE)?"enabled\n":"disabled\n");

	// Display Lockstep Info
	if (p->my_qthread_number == 0) {
		if (p->lockstepp) {
			if (p->lockstepp->ls_master >= 0) {
				mp = xgp->ptdsp[p->lockstepp->ls_master];
				fprintf(out,"\t\tMaster Target, %d\n", p->lockstepp->ls_master);
				fprintf(out,"\t\tMaster Interval value and type, %lld,%s\n", (long long)mp->lockstepp->ls_interval_value, mp->lockstepp->ls_interval_units);
			}
			if (p->lockstepp->ls_slave >= 0) {
				sp = xgp->ptdsp[p->lockstepp->ls_slave];
				fprintf(out,"\t\tSlave Target, %d\n", p->lockstepp->ls_slave);
				fprintf(out,"\t\tSlave Task value and type, %lld,%s\n", (long long)sp->lockstepp->ls_task_value,sp->lockstepp->ls_task_units);
				fprintf(out,"\t\tSlave initial condition, %s\n",(sp->lockstepp->ls_ms_state & LS_SLAVE_RUN_IMMEDIATELY)?"Run":"Wait");
				fprintf(out,"\t\tSlave termination, %s\n",(sp->lockstepp->ls_ms_state & LS_SLAVE_COMPLETE)?"Complete":"Abort");
			}
		}
	}

	// Display information about any End-to-End operations for this target 
	// Only qthread 0 displays the inforamtion
	if (p->target_options & TO_ENDTOEND) { // This target is part of an end-to-end operation
		// Display info
		fprintf(out,"\t\tEnd-to-End ACTIVE: this target is the %s side\n",
			(p->target_options & TO_E2E_DESTINATION) ? "DESTINATION":"SOURCE");
		// Display all the hostname:base_port,port_count entries in the e2e_address_table
		for (i = 0; i < p->e2e_address_table_host_count; i++) {
			fprintf(out,"\t\tEnd-to-End Destination Address %d of %d '%s' base port %d for %d ports [ports %d - %d]\n",
				i+1,
				p->e2e_address_table_host_count,
				p->e2e_address_table[i].hostname,
				p->e2e_address_table[i].base_port,
				p->e2e_address_table[i].port_count,
				p->e2e_address_table[i].base_port,
				p->e2e_address_table[i].base_port + p->e2e_address_table[i].port_count -1);
		}

		// Check for RESTART and setup restart structure if required
		if (p->target_options & TO_RESTART_ENABLE) { 
			// Set up the restart structure in this PTDS
			if (p->restartp == NULL) {
				fprintf(out,"\t\tRESTART - Internal Error - no restart structure assigned to this target!\n");
				p->target_options &= ~TO_RESTART_ENABLE; // turn off restart
				return;
			} 
			// ok - we have a good restart structure
			p->restartp->source_host = p->e2e_src_hostname; 		// Name of the Source machine 
			p->restartp->destination_host = p->e2e_dest_hostname; 	// Name of the Destination machine 
			if (p->restartp->flags & RESTART_FLAG_ISSOURCE) { // This is the SOURCE sside of the biz
				 p->restartp->source_filename = p->target_full_pathname; 		// The source_filename is the name of the file being copied on the source side
				 p->restartp->destination_filename = NULL;		// The destination_filename is the name of the file being copied on the destination side
				if (p->restartp->flags & RESTART_FLAG_RESUME_COPY) { // Indicate that this is the resumption of a previous copy from source file XXXXX
						fprintf(out,"\t\tRESTART: RESUMING COPY: Source File, %s, on source host name, %s\n", 
								p->restartp->source_filename,
								p->restartp->source_host);
				}
			} else { // This is the DESTINATION side of the biz
				 p->restartp->source_filename = NULL; 		// The source_filename is the name of the file being copied on the source side
				 p->restartp->destination_filename = p->target_full_pathname;		// The destination_filename is the name of the file being copied on the destination side
				if (p->restartp->flags & RESTART_FLAG_RESUME_COPY) {  // Indicate that this is the resumption of a previous copy from source file XXXXX
						fprintf(out,"\t\tRESTART: RESUMING COPY: Destination File, %s, on destination host name, %s\n", 
								p->restartp->destination_filename,
								p->restartp->destination_host);
				}
			}

			if (p->restartp->flags & RESTART_FLAG_RESUME_COPY) { // Indicate that this is the resumption of a previous copy
				fprintf(out,"\t\tRESTART: RESUMING COPY: Starting offset, \n\t\t\t%lld Bytes, \n\t\t\t%f, KBytes, \n\t\t\t%f, MBytes, \n\t\t\t%f, GBytes, \n\t\t\t%f, TBytes\n",
					(long long)p->start_offset,
					(double)(p->start_offset/FLOAT_KILOBYTE),
					(double)(p->start_offset/FLOAT_MEGABYTE),
					(double)(p->start_offset/FLOAT_GIGABYTE),
					(double)(p->start_offset/FLOAT_TERABYTE));
				fprintf(out,"\t\tRESTART: RESUMING COPY: Remaining data,  \n\t\t\t%lld Bytes, \n\t\t\t%f, KBytes, \n\t\t\t%f, MBytes, \n\t\t\t%f, GBytes, \n\t\t\t%f, TBytes\n",
					(long long)p->target_bytes_to_xfer_per_pass,
					(double)(p->target_bytes_to_xfer_per_pass/FLOAT_KILOBYTE),
					(double)(p->target_bytes_to_xfer_per_pass/FLOAT_MEGABYTE),
					(double)(p->target_bytes_to_xfer_per_pass/FLOAT_GIGABYTE),
					(double)(p->target_bytes_to_xfer_per_pass/FLOAT_TERABYTE));
			}	
		}
	}
	fflush(out);
} /* end of xdd_target_info() */

/*----------------------------------------------------------------------------*/
/* xdd_memory_usage_info() - Display information about the system this program
 * is being run on. This includes hardware, software, and environmental info.
 */
void
xdd_memory_usage_info(FILE *out) {
	return;

} /* end of xdd_memory_usage_info() */

/*----------------------------------------------------------------------------*/
/* xdd_config_info() - Display configuration information about this run.
 */
void
xdd_config_info(void) {
	xdd_options_info(xgp->output);
	xdd_system_info(xgp->output);
	xdd_memory_usage_info(xgp->output);
	if (xgp->global_options & GO_CSV) {
		xdd_options_info(xgp->csvoutput);
		xdd_system_info(xgp->csvoutput);
	}
	fflush(xgp->output);
} /* end of xdd_config_info() */
