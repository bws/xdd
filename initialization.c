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
 * This file contains the subroutines that perform various initialization 
 * functions when xdd is started.
 */
#include "xdd.h"
#include "datapatterns.h"
/*----------------------------------------------------------------------------*/
/* xdd_init_globals() - Initialize a xdd global variables  
 */
void
xdd_init_globals(char *progname) {

	xgp = (xdd_globals_t *)malloc(sizeof(struct xdd_globals));
	if (xgp == 0) {
		fprintf(stderr,"%s: Cannot allocate %d bytes of memory for global variables!\n",progname, sizeof(struct xdd_globals));
		perror("Reason");
		exit(1);
	}
	memset(xgp,0,sizeof(struct xdd_globals));

	xgp->progname = progname;
	xgp->global_options = 0;
	xgp->output = stdout;
	xgp->output_filename = "stdout";
	xgp->errout = stderr;
	xgp->errout_filename = "stderr";
	xgp->csvoutput_filename = "";
	xgp->csvoutput = NULL;
	xgp->combined_output_filename = ""; /* name of the combined output file */
	xgp->combined_output = NULL;       /* Combined output file */
	xgp->canceled = 0;       /* Normally set to 0. Set to 1 by xdd_sigint when an interrupt occurs */
	xgp->id_firsttime = 1;
	
	/* Initialize the global variables */
	// Some of these settings may seem redundant because they are set to zero after clearing the entire data structure but they
	// are basically place holders in case their default values need to be changed.
	xgp->passes = DEFAULT_PASSES;
	xgp->pass_delay = DEFAULT_PASSDELAY;
	xgp->tsbinary_filename = DEFAULT_TIMESTAMP;
	xgp->syncio = 0;
	xgp->target_offset = 0;
	xgp->number_of_targets = 0;
	xgp->tsoutput_filename = 0;
	xgp->max_errors = 0; /* set to total_ops later when we know what it is */
	xgp->max_errors_to_print = DEFAULT_MAX_ERRORS_TO_PRINT;  /* max number of errors to print information about */ 
	/* Information needed to access the Global Time Server */
	xgp->gts_hostname = 0;
	xgp->gts_addr = 0;
	xgp->gts_time = 0;
	xgp->gts_port = DEFAULT_PORT;
	xgp->gts_bounce = DEFAULT_BOUNCE;
	xgp->gts_delta = 0;
	xgp->gts_seconds_before_starting = 0; /* number of seconds before starting I/O */
	xgp->heartbeat = 0;
	xgp->run_ring = 0;       /* The alarm that goes off when the total run time has been exceeded */
	xgp->deskew_ring = 0;    /* The alarm that goes off when the the first thread finishes */
	xgp->abort_io = 0;       /* abort the run due to some catastrophic failure */
	xgp->heartbeat = 0;              /* seconds between heartbeats */
	xgp->number_of_iothreads = 0;    /* number of threads spawned for all targets */
	xgp->runtime = 0;                /* Length of time to run all targets, all passes */
	xgp->estimated_end_time = 0;     /* The time at which this run (all passes) should end */
	xgp->number_of_processors = 0;   /* Number of processors */ 
	xgp->random_initialized = 0;     /* Random number generator has not been initialized  */
	xgp->ActualLocalStartTime = 0;   /* The time to start operations */
	xgp->heartbeat_holdoff = 0;  	/* used by results manager to suspend or cancel heartbeat displays */
	xgp->format_string = DEFAULT_OUTPUT_FORMAT_STRING;

/* teporary until DESKEW results are fixed */
	xgp->deskew_total_rates = 0;
	xgp->deskew_total_time = 0;
	xgp->deskew_total_bytes = 0;

	// Allocate a suitable buffer for the run ID ASCII string
	xgp->id = (char *)malloc(MAX_IDLEN);
	if (xgp->id == NULL) {
		fprintf(xgp->errout,"%s: Cannot allocate %d bytes of memory for ID string\n",xgp->progname,MAX_IDLEN);
		exit(1);
	}
	strcpy(xgp->id,DEFAULT_ID);

} /* end of xdd_init_globals() */

/*----------------------------------------------------------------------------*/
/* xdd_init_new_ptds() - Initialize the default Per-Target-Data-Structure 
 * Note: CLO == Command Line Option
 */
void
xdd_init_new_ptds(ptds_t *p, int32_t n) {

		// Zero out the memory first
		memset((unsigned char *)p, 0, sizeof(ptds_t));

		p->my_target_number = n; // set upon creation of this PTDS
		p->my_qthread_number = 0; // Set upon creation
		p->mypid = getpid(); // Set upon creation
		p->mythreadid = 0; // This is set later by the actual thread 
		p->pass_complete = 0; // set upon creation
		p->run_complete = 0; // set upon creation
		p->nextp = 0; // set upon creation, used when other qthreads are created
		p->pm1 = 0; // set upon creation
		p->rwbuf = 0; // set during rwbuf allocation
		p->rwbuf_shmid = -1; // set upon creation of a shared memory segment
		p->rwbuf_save = 0; // used by the rwbuf allocation routine
		p->targetdir = DEFAULT_TARGETDIR; // can be changed by CLO
		p->target = DEFAULT_TARGET;  // can be changed by CLO
		sprintf(p->targetext,"%08d",1);  // can be changed by CLO
		p->reqsize = DEFAULT_REQSIZE;  // can be changed by CLO
		p->throttle = DEFAULT_THROTTLE;
		p->throttle_variance = DEFAULT_VARIANCE;
		p->throttle_type = PTDS_THROTTLE_BW;
		p->ts_options = DEFAULT_TS_OPTIONS;
		p->target_options = DEFAULT_TARGET_OPTIONS; // Zero the target options field
		p->time_limit = DEFAULT_TIME_LIMIT;
		p->numreqs = DEFAULT_NUMREQS;
		p->report_threshold = DEFAULT_REPORT_THRESHOLD;
		p->flushwrite_current_count = 0;
		p->flushwrite = DEFAULT_FLUSHWRITE;
		p->bytes = DEFAULT_BYTES;
		p->kbytes = DEFAULT_KBYTES;
		p->mbytes = DEFAULT_MBYTES;
		p->start_offset = DEFAULT_STARTOFFSET;
		p->pass_offset = DEFAULT_PASSOFFSET;
		p->preallocate = DEFAULT_PREALLOCATE;
		p->queue_depth = DEFAULT_QUEUEDEPTH;
		p->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
		p->data_pattern = (unsigned char *)DEFAULT_DATA_PATTERN;
		p->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
		p->data_pattern_prefix = (unsigned char *)DEFAULT_DATA_PATTERN_PREFIX;
		p->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
		p->block_size = DEFAULT_BLOCKSIZE;
		p->mem_align = getpagesize();

		p->processor = -1;
		p->start_delay = DEFAULT_START_DELAY;
		p->start_trigger_time = 0; /* Time to trigger another target to start */
		p->stop_trigger_time = 0; /* Time to trigger another target to stop */
		p->start_trigger_op = 0; /* Operation number to trigger another target to start */
		p->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		p->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		p->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		p->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		p->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		p->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		p->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		p->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
		p->trigger_types = 0;
		p->ls_master = -1; /* The default master number  */
		p->ls_slave  = -1; /* The default slave number */
		p->ls_interval_type  = 0; /* The default interval type */
		p->ls_interval_value  = 0; /* The default interval value  */
		p->ls_interval_units  = "not defined"; /* The default interval units  */
		p->ls_task_type  = 0; /* The default task type */
		p->ls_task_value  = 0; /* The default task value  */
		p->ls_task_units  = "not defined"; /* The default task units  */
		p->ls_task_counter = 0; /* the default task counter */
		/* Init the seeklist header fields */
		p->seekhdr.seek_options = 0;
		p->seekhdr.seek_range = DEFAULT_RANGE;
		p->seekhdr.seek_seed = DEFAULT_SEED;
		p->seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
		p->seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
		p->seekhdr.seek_num_rw_ops = DEFAULT_NUMREQS;
		p->seekhdr.seek_total_ops = DEFAULT_NUMREQS;
		p->seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
		p->seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
		p->seekhdr.seek_savefile = NULL; /* file to save seek locations into */
		p->seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
		p->seekhdr.seek_pattern = "sequential";
		/* Init the read-after-write fields */
		p->raw_sd = 0; /* raw socket descriptor */
		p->raw_hostname = NULL;  /* Reader hostname */
		p->raw_lag = DEFAULT_RAW_LAG; 
		p->raw_port = DEFAULT_RAW_PORT;
		p->raw_trigger = PTDS_RAW_MP; /* default to a message passing */
		/* Init the end-to-end fields */
		p->e2e_sd = 0; /* destination machine socket descriptor */
		p->e2e_src_hostname = NULL;  /* E2E source hostname */
		p->e2e_dest_hostname = NULL;  /* E2E destination hostname */
		p->e2e_dest_port = DEFAULT_E2E_PORT;
		p->e2e_dest_addr = 0;
		p->e2e_wait_1st_msg = 0;
		p->e2e_useUDP = 0;
		/* Init the results structure */
		memset((unsigned char *)&p->qthread_average_results,0,sizeof(results_t)); 
} /* end of xdd_init_new_ptds() */

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
	struct utsname name;
	struct rusage xdd_rusage;
	char	*userlogin;
#if ( IRIX )
	inventory_t *inventp;
	int64_t mem_size;
#endif // IRIX inventory
	uname(&name);
	userlogin = getlogin();
	if (!userlogin)
		userlogin = "***unknown user login***";
	fprintf(out, "Computer Name, %s, User Name, %s\n",name.nodename, userlogin);
	fprintf(out, "OS release and version, %s %s %s\n",name.sysname, name.release, name.version);
	fprintf(out, "Machine hardware type, %s\n",name.machine);
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


	fprintf(out,"IOIOIOIOIOIOIOIOIOIOI XDD version %s based on %s IOIOIOIOIOIOIOIOIOIOIOI\n",XDD_VERSION, XDD_BASE_VERSION);
	fprintf(out,"%s\n",XDD_COPYRIGHT);
	fprintf(out,"%s\n",XDD_DISCLAIMER);
	xgp->current_time_for_this_run = time(NULL);
	c = ctime(&xgp->current_time_for_this_run);
	fprintf(out,"Starting time for this run, %s\n",c);
	fprintf(out,"ID for this run, '%s'\n", xgp->id);
	fprintf(out,"Maximum Process Priority, %s", (xgp->global_options & GO_MAXPRI)?"enabled\n":"disabled\n");
	fprintf(out, "Passes, %d\n", xgp->passes);
	fprintf(out, "Pass Delay in seconds, %d\n", xgp->pass_delay); 
	fprintf(out, "Maximum Error Threshold, %lld\n", (long long)xgp->max_errors);
	fprintf(out, "Target Offset, %lld\n",(long long)xgp->target_offset);
	fprintf(out, "I/O Synchronization, %d\n", xgp->syncio);
	fprintf(out, "Total run-time limit in seconds, %d\n", xgp->runtime);
	fprintf(out, "Output file name, %s\n",xgp->output_filename);
	fprintf(out, "CSV output file name, %s\n",xgp->csvoutput_filename);
	fprintf(out, "Error output file name, %s\n",xgp->errout_filename);
	if (xgp->global_options & GO_COMBINED)
		fprintf(out,"Combined output file name, %s\n",xgp->combined_output_filename);
	fprintf(out,"Pass synchronization barriers, %s", (xgp->global_options & GO_NOBARRIER)?"disabled\n":"enabled\n");
	if (xgp->gts_hostname) {
			fprintf(out,"Timeserver hostname, %s\n", xgp->gts_hostname);
			fprintf(out,"Timeserver port number, %d\n", xgp->gts_port);
			fprintf(out,"Global start time, %lld\n", (long long)xgp->gts_time/TRILLION);
	}
	fprintf(out,"Number of Targets, %d\n",xgp->number_of_targets);
	fprintf(out,"Number of I/O Threads, %d\n",xgp->number_of_iothreads);
	if (xgp->global_options & GO_REALLYVERBOSE) {
		fprintf(out, "Size of PTDS is %d bytes, %d Aggregate\n",sizeof(ptds_t), sizeof(ptds_t)*xgp->number_of_iothreads);
		fprintf(out, "Size of RESULTS is %d bytes, %d Aggregate\n",sizeof(results_t), sizeof(results_t)*(xgp->number_of_iothreads*2+xgp->number_of_targets));
	}
	fprintf(out, "\n");
	fflush(out);
} /* end of xdd_options_info() */

/*----------------------------------------------------------------------------*/
/* xdd_target_info() - Display command-line options information about this run.
 */
void
xdd_target_info(FILE *out, ptds_t *p) {
	int i;
	ptds_t *mp, *sp; /* Master and Slave ptds pointers */

    // Only display information for qthreads if requested
    if (!(p->target_options & TO_QTHREAD_INFO) && (p->my_qthread_number > 0))
        return;

	fprintf(out,"\tTarget[%d] Q[%d], %s\n",p->my_target_number, p->my_qthread_number, p->target);
	fprintf(out,"\t\tTarget directory, %s\n",(p->targetdir=="")?"\"./\"":p->targetdir);
	fprintf(out,"\t\tProcess ID, %d\n",p->mypid);
	fprintf(out,"\t\tThread ID, %d\n",p->mythreadid);
    if (p->processor == -1) 
		    fprintf(out,"\t\tProcessor, all/any\n");
	else fprintf(out,"\t\tProcessor, %d\n",p->processor);
	fprintf(out,"\t\tRead/write ratio, %5.2f, %5.2f\n",p->rwratio*100.0,(1.0-p->rwratio)*100.0);
	fprintf(out,"\t\tThrottle in %s, %6.2f\n",
		(p->throttle_type & PTDS_THROTTLE_OPS)?"ops/sec":((p->throttle_type & PTDS_THROTTLE_BW)?"MB/sec":"Delay"), p->throttle);
	fprintf(out,"\t\tPer-pass time limit in seconds, %d\n",p->time_limit);
	fprintf(out,"\t\tPass seek randomization, %s", (p->target_options & TO_PASS_RANDOMIZE)?"enabled\n":"disabled\n");
	fprintf(out,"\t\tFile write synchronization, %s", (p->target_options & TO_SYNCWRITE)?"enabled\n":"disabled\n");
	fprintf(out, "\t\tBlocksize in bytes, %d\n", p->block_size);
	fprintf(out,"\t\tRequest size, %d, blocks, %d, bytes\n",p->reqsize,p->reqsize*p->block_size);

	if (p->numreqs > 0) 
		fprintf(out, "\t\tNumber of Requests, %lld, of, %lld, target ops for all qthreads\n", (long long)p->numreqs, (long long)p->target_ops);

	fprintf(out, "\t\tStart offset, %lld\n",(long long)p->start_offset);
	fprintf(out, "\t\tFlushwrite interval, %lld\n", (long long)p->flushwrite);
	if (p->bytes > 0)
		fprintf(out, "\t\tNumber of Bytes, %lld, of %lld, total Bytes to transfer\n", (long long)(p->numreqs*(p->reqsize*p->block_size)),(long long)p->bytes);
	else if (p->kbytes > 0)
		fprintf(out, "\t\tNumber of KiloBytes, %lld, of %lld, total KiloBytes to transfer\n", (long long)((p->numreqs*(p->reqsize*p->block_size))/1024),(long long)p->kbytes);
	else if (p->mbytes > 0)
		fprintf(out, "\t\tNumber of MegaBytes, %lld, of %lld, total MegaBytes to transfer\n", (long long)((p->numreqs*(p->reqsize*p->block_size))/(1024*1024)),(long long)p->mbytes);
	fprintf(out, "\t\tPass Offset in blocks, %lld\n", (long long)p->pass_offset);
	fprintf(out,"\t\tI/O memory buffer is %s\n", 
		(p->target_options & TO_SHARED_MEMORY)?"a shared memory segment":"a normal memory buffer");
	fprintf(out,"\t\tI/O memory buffer alignment in bytes, %d\n", p->mem_align);
	fprintf(out,"\t\tData pattern in buffer");
	if (p->target_options & (TO_RANDOM_PATTERN | TO_SEQUENCED_PATTERN | TO_INVERSE_PATTERN | TO_ASCII_PATTERN | TO_HEX_PATTERN | TO_PATTERN_PREFIX)) {
		if (p->target_options & TO_RANDOM_PATTERN) fprintf(out,",random ");
		if (p->target_options & TO_SEQUENCED_PATTERN) fprintf(out,",sequenced ");
		if (p->target_options & TO_INVERSE_PATTERN) fprintf(out,",inversed ");
		if (p->target_options & TO_ASCII_PATTERN) fprintf(out,",ASCII: '%s' <%d bytes> %s ",
			p->data_pattern,p->data_pattern_length, (p->target_options & TO_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
		if (p->target_options & TO_HEX_PATTERN) {
			fprintf(out,",HEX: 0x");
			for (i=0; i<p->data_pattern_length; i++) 
				fprintf(out,"%02x",p->data_pattern[i]);
			fprintf(out, " <%d bytes>, %s\n",
				p->data_pattern_length, (p->target_options & TO_REPLICATE_PATTERN)?"Replicated":"Not Replicated");
		}
		if (p->target_options & TO_PATTERN_PREFIX)  {
			fprintf(out,",PREFIX: 0x");
			for (i=0; i<p->data_pattern_prefix_length; i+=2) 
				fprintf(out,"%02x",p->data_pattern_prefix[i]);
			fprintf(out, " <%d nibbles>\n", p->data_pattern_prefix_length);
		}
	} else { // Just display the one-byte hex pattern 
		fprintf(out,",0x%02x\n",p->data_pattern[0]);
	}
	if (p->target_options & TO_FILE_PATTERN) 
		fprintf(out," From file: %s\n",p->data_pattern_filename);
	fprintf(out,"\t\tData buffer verification is");
	if ((p->target_options & (TO_VERIFY_LOCATION | TO_VERIFY_CONTENTS)))
		fprintf(out," enabled for %s verification.\n", (p->target_options & TO_VERIFY_LOCATION)?"Location":"Content");
	else fprintf(out," disabled.\n");
	fprintf(out,"\t\tDirect I/O, %s", (p->target_options & TO_DIO)?"enabled\n":"disabled\n");
	fprintf(out, "\t\tSeek pattern, %s\n", p->seekhdr.seek_pattern);
	if (p->seekhdr.seek_range > 0)
		fprintf(out, "\t\tSeek range, %lld\n",(long long)p->seekhdr.seek_range);
	fprintf(out, "\t\tPreallocation, %d\n",p->preallocate);
	fprintf(out, "\t\tQueue Depth, %d\n",p->queue_depth);
	/* Timestamp options */
	if (p->ts_options & TS_ON) {
		fprintf(out, "\t\tTimestamping, enabled for %s %s\n",(p->ts_options & TS_DETAILED)?"DETAILED":"", (p->ts_options & TS_SUMMARY)?"SUMMARY":"");
		fprintf(out, "\t\tTimestamp ASCII output file name, %s.target.%04d.qthread.%04d.csv\n",xgp->tsoutput_filename,p->my_target_number,p->my_qthread_number);
		if (p->ts_options & TS_DUMP) 
			fprintf(out, "\t\tTimestamp binary output file name, %s.target.%04d.qthread.%04d.bin\n",xgp->tsbinary_filename,p->my_target_number,p->my_qthread_number);
	} else fprintf(out, "\t\tTimestamping, disabled\n");
	fflush(out);
	fprintf(out,"\t\tDelete file, %s", (p->target_options & TO_DELETEFILE)?"enabled\n":"disabled\n");
	if (p->my_qthread_number == 0) {
		if (p->ls_master >= 0) {
			mp = xgp->ptdsp[p->ls_master];
			fprintf(out,"\t\tMaster Target, %d\n", p->ls_master);
			fprintf(out,"\t\tMaster Interval value and type, %lld,%s\n", (long long)mp->ls_interval_value, mp->ls_interval_units);
		}
		if (p->ls_slave >= 0) {
			sp = xgp->ptdsp[p->ls_slave];
			fprintf(out,"\t\tSlave Target, %d\n", p->ls_slave);
			fprintf(out,"\t\tSlave Task value and type, %lld,%s\n", (long long)sp->ls_task_value,sp->ls_task_units);
			fprintf(out,"\t\tSlave initial condition, %s\n",(sp->ls_ms_state & LS_SLAVE_RUN_IMMEDIATELY)?"Run":"Wait");
			fprintf(out,"\t\tSlave termination, %s\n",(sp->ls_ms_state & LS_SLAVE_COMPLETE)?"Complete":"Abort");
		}
	}

	// Display information about any End-to-End operations for this target 
	// Only qthread 0 displays the inforamtion
//	if (p->my_qthread_number == 0) {
		if (p->target_options & TO_ENDTOEND) { // This target is part of an END-TO-END operation
				fprintf(out,"\t\tEnd-to-End ACTIVE: this target is the %s side\n",
					(p->target_options & TO_E2E_DESTINATION) ? "DESTINATION":"SOURCE");
				fprintf(out,"\t\tEnd-to-End Destination Address is %s using port %d",
					p->e2e_dest_hostname, p->e2e_dest_port);
				if (p->queue_depth > 1) 
					fprintf(out," of ports %d thru %d", 
						(p->e2e_dest_port - p->my_qthread_number), 
						((p->e2e_dest_port - p->my_qthread_number) + p->queue_depth - 1));
				fprintf(out,"\n");
		}
//	}
	fprintf(out, "\n");
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
/*----------------------------------------------------------------------------*/
/* xdd_sigint() - Routine that gets called when an Interrupt occurs. This will
 * call the appropriate routines to remove all the barriers and semaphores so
 * that we shut down gracefully.
 */
void
xdd_sigint(int n) {
	fprintf(xgp->errout,"Program canceled - destroying all barriers...");
	fflush(xgp->errout);
	xgp->canceled = 1;
	xdd_destroy_all_barriers();
	fprintf(xgp->errout,"done. Exiting\n");
	fflush(xgp->errout);
} /* end of xdd_sigint() */

/*----------------------------------------------------------------------------*/
/* xdd_init_signals() - Initialize all the signal handlers
 */
void
xdd_init_signals(void) {
	signal(SIGINT, xdd_sigint);
} /* end of xdd_init_signals() */
/*----------------------------------------------------------------------------*/
/* xdd_init_global_clock_network() - Initialize the network so that we can
 *   talk to the global clock timeserver.
 *   
 */
in_addr_t
xdd_init_global_clock_network(char *hostname) {
	struct hostent *hostent; /* used to init the time server info */
	in_addr_t addr;  /* Address of hostname from hostent */
#ifdef WIN32
	WSADATA wsaData; /* Data structure used by WSAStartup */
	int wsastatus; /* status returned by WSAStartup */
	char *reason;
	wsastatus = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsastatus != 0) { /* Error in starting the network */
		switch (wsastatus) {
		case WSASYSNOTREADY:
			reason = "Network is down";
			break;
		case WSAVERNOTSUPPORTED:
			reason = "Request version of sockets <2.2> is not supported";
			break;
		case WSAEINPROGRESS:
			reason = "Another Windows Sockets operation is in progress";
			break;
		case WSAEPROCLIM:
			reason = "The limit of the number of sockets tasks has been exceeded";
			break;
		case WSAEFAULT:
			reason = "Program error: pointer to wsaData is not valid";
			break;
		default:
			reason = "Unknown error code";
			break;
		};
		fprintf(xgp->errout,"%s: Error initializing network connection\nReason: %s\n",
			xgp->progname, reason);
		fflush(xgp->errout);
		WSACleanup();
		return(-1);
	} 
	/* Check the version number */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Couldn't find WinSock DLL version 2.2 or better */
		fprintf(xgp->errout,"%s: Error initializing network connection\nReason: Could not find version 2.2\n",
			xgp->progname);
		fflush(xgp->errout);
		WSACleanup();
		return(-1);
	}
#endif
	/* Network is initialized and running */
	hostent = gethostbyname(hostname);
	if (!hostent) {
		fprintf(xgp->errout,"%s: Error: Unable to identify host %s\n",xgp->progname,hostname);
		fflush(xgp->errout);
#ifdef WIN32
		WSACleanup();
#endif
		return(-1);
	}
	/* Got it - unscramble the address bytes and return to caller */
	addr = ntohl(((struct in_addr *)hostent->h_addr)->s_addr);
	return(addr);
} /* end of xdd_init_global_clock_network() */
/*----------------------------------------------------------------------------*/
/* xdd_init_global_clock() - Initialize the global clock if requested
 */
void
xdd_init_global_clock(pclk_t *pclkp) {
	pclk_t  now;  /* the current time returned by pclk() */


	/* Global clock stuff here */
	if (xgp->gts_hostname) {
		xgp->gts_addr = xdd_init_global_clock_network(xgp->gts_hostname);
		if (xgp->gts_addr == -1) { /* Problem with the network */
			fprintf(xgp->errout,"%s: Error initializing global clock - network malfunction\n",xgp->progname);
			fflush(xgp->errout);
                        *pclkp = 0;
			return;
		}
		clk_initialize(xgp->gts_addr, xgp->gts_port, xgp->gts_bounce, &xgp->gts_delta);
		pclk_now(&now);
		xgp->ActualLocalStartTime = xgp->gts_time - xgp->gts_delta; 
		xgp->gts_seconds_before_starting = ((xgp->ActualLocalStartTime - now) / TRILLION); 
		fprintf(xgp->errout,"Global Time now is %lld. Starting in %lld seconds at Global Time %lld\n",
			(long long)(now+xgp->gts_delta)/TRILLION, 
			(long long)xgp->gts_seconds_before_starting, 
			(long long)xgp->gts_time/TRILLION); 
		fflush(xgp->errout);
		*pclkp = xgp->ActualLocalStartTime;
		return;
	}
	pclk_now(pclkp);
	return;
} /* end of xdd_init_global_clock() */

/*----------------------------------------------------------------------------*/
/* xdd_pattern_buffer() - init the I/O buffer with the appropriate pattern
 * This routine will put the requested pattern in the rw buffer.
 */
void
xdd_pattern_buffer(ptds_t *p) {
	int32_t i;
	int32_t pattern_length; // Length of the pattern
	int32_t remaining_length; // Length of the space in the pattern buffer
	unsigned char    *ucp;          // Pointer to an unsigned char type, duhhhh
	uint32_t *lp;			// pointer to a pattern


	if (p->target_options & TO_RANDOM_PATTERN) { // A nice random pattern
			lp = (uint32_t *)p->rwbuf;
			xgp->random_initialized = 0;
            /* Set each four-byte field in the I/O buffer to a random integer */
			for(i = 0; i < (int32_t)(p->iosize / sizeof(int32_t)); i++ ) {
				*lp=xdd_random_int();
				lp++;
			}
	} else if ((p->target_options & TO_ASCII_PATTERN) ||
	           (p->target_options & TO_HEX_PATTERN)) { // put the pattern that is in the pattern buffer into the io buffer
			// Clear out the buffer before putting in the string so there are no strange characters in it.
			memset(p->rwbuf,'\0',p->iosize);
			if (p->target_options & TO_REPLICATE_PATTERN) { // Replicate the pattern throughout the buffer
				ucp = (unsigned char *)p->rwbuf;
				remaining_length = p->iosize;
				while (remaining_length) { 
					if (p->data_pattern_length < remaining_length) 
						pattern_length = p->data_pattern_length;
					else pattern_length = remaining_length;

					memcpy(ucp,p->data_pattern,pattern_length);
					remaining_length -= pattern_length;
					ucp += pattern_length;
				}
			} else { // Just put the pattern at the beginning of the buffer once 
				if (p->data_pattern_length < p->iosize) 
					 pattern_length = p->data_pattern_length;
				else pattern_length = p->iosize;
				memcpy(p->rwbuf,p->data_pattern,pattern_length);
			}
	} else if (p->target_options & TO_LFPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(lfpat);
                fprintf(stderr,"LFPAT length is %d\n", p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,lfpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_LTPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(ltpat);
                fprintf(stderr,"LTPAT length is %d\n", p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,ltpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CJTPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(cjtpat);
                fprintf(stderr,"CJTPAT length is %d\n", p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,cjtpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CRPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(crpat);
                fprintf(stderr,"CRPAT length is %d\n", p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,crpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CSPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(cspat);
                fprintf(stderr,"CSPAT length is %d\n", p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,cspat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
    } else { // Otherwise set the entire buffer to the character in "data_pattern"
                memset(p->rwbuf,*(p->data_pattern),p->iosize);
	}
		
	return;
} // end of xdd_pattern_buffer()
/*----------------------------------------------------------------------------*/
/* xdd_init_io_buffers() - set up the I/O buffers
 * This routine will allocate the memory used as the I/O buffer for a target.
 * For some operating systems, you can use a shared memory segment instead of 
 * a normal malloc/valloc memory chunk. This is done using the "-sharedmemory"
 * command line option.
 */
unsigned char *
xdd_init_io_buffers(ptds_t *p) {
	unsigned char *rwbuf; /* the read/write buffer for this op */
	void *shmat_status;
#ifdef WIN32
	LPVOID lpMsgBuf; /* Used for the error messages */
#endif



	/* allocate slightly larger buffer for meta data for end-to-end ops */
	if ((p->target_options & TO_ENDTOEND)) 
		p->e2e_iosize = p->iosize + sizeof(p->e2e_msg);
	else	p->e2e_iosize = p->iosize;
	/* Check to see if we want to use a shared memory segment and allocate it using shmget() and shmat().
	 * NOTE: This is not supported by all operating systems. 
	 */
	if (p->target_options & TO_SHARED_MEMORY) {
#if (AIX || LINUX || SOLARIS || OSX || FREEBSD)
		/* In AIX we need to get memory in a shared memory segment to avoid
	     * the system continually trying to pin each page on every I/O operation */
#if (AIX)
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->e2e_iosize, IPC_CREAT | SHM_LGPAGE |SHM_PIN );
#else
		p->rwbuf_shmid = shmget(IPC_PRIVATE, p->e2e_iosize, IPC_CREAT );
#endif
		if (p->rwbuf_shmid < 0) {
			fprintf(xgp->errout,"%s: Cannot create shared memory segment\n", xgp->progname);
			perror("Reason");
			rwbuf = 0;
			p->rwbuf_shmid = -1;
		} else {
			shmat_status = (void *)shmat(p->rwbuf_shmid,NULL,0);
			if (shmat_status == (void *)-1) {
				fprintf(xgp->errout,"%s: Cannot attach to shared memory segment\n",xgp->progname);
				perror("Reason");
				rwbuf = 0;
				p->rwbuf_shmid = -1;
			}
			else rwbuf = (unsigned char *)shmat_status;
		}
		if (xgp->global_options & GO_REALLYVERBOSE)
				fprintf(xgp->output,"Shared Memory ID allocated and attached, shmid=%d\n",p->rwbuf_shmid);
#elif (IRIX || WIN32 )
		fprintf(xgp->errout,"%s: Shared Memory not supported on this OS - using valloc\n",
			xgp->progname);
		p->target_options &= ~TO_SHARED_MEMORY;
#if (IRIX || SOLARIS || LINUX || AIX || OSX || FREEBSD)
		rwbuf = valloc(p->e2e_iosize);
#else
		rwbuf = malloc(p->e2e_iosize);
#endif
#endif 
	} else { /* Allocate memory the normal way */
#if (IRIX || SOLARIS || LINUX || AIX || OSX || FREEBSD)
		rwbuf = valloc(p->e2e_iosize);
#else
		rwbuf = malloc(p->e2e_iosize);
#endif
	}
	/* Check to see if we really allocated some memory */
	if (rwbuf == NULL) {
		fprintf(xgp->errout,"%s: cannot allocate %d bytes of memory for I/O buffer\n",
			xgp->progname,p->e2e_iosize);
		fflush(xgp->errout);
#ifdef WIN32 
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
		fprintf(xgp->errout,"Reason:%s",lpMsgBuf);
		fflush(xgp->errout);
#else 
		perror("Reason");
#endif
		return(NULL);
	}
	/* Memory allocation must have succeeded */

	/* Lock all rwbuf pages in memory */
	xdd_lock_memory(rwbuf, p->e2e_iosize, "RW BUFFER");

	return(rwbuf);
} /* end of xdd_init_io_buffers() */

/*----------------------------------------------------------------------------*/
int32_t
xdd_process_directive(char *lp) {
	fprintf(xgp->errout,"directive %s being processed\n",lp);
	return(0);
} /* end of xdd_process_directive() */
/*----------------------------------------------------------------------------*/
char *
xdd_getnexttoken(char *tp) {
	char *cp;
	cp = tp;
	while ((*cp != TAB) && (*cp != SPACE)) cp++;
	while ((*cp == TAB) || (*cp == SPACE)) cp++;
	return(cp);
} /* end of xdd_getnexttoken() */
/*----------------------------------------------------------------------------*/
void
xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not lock memory 
	if (xgp->global_options & GO_NOMEMLOCK)
		return;

#if (AIX)
	int32_t liret;
	off_t newlim;
	/* Get the current memory stack limit - required before locking the the memory */
	liret = -1;
	newlim = -1;
	liret = ulimit(GET_STACKLIM,0);
	if (liret == -1) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not get memory stack limit\n",
				getpid(),xgp->progname);
		perror("Reason");
	}
	/* Get 8 pages more than the stack limit */
	newlim = liret - (PAGESIZE*8);
	return;
#else
#if  (LINUX || SOLARIS || OSX || AIX || FREEBSD)
	if (getuid() != 0) {
		fprintf(xgp->errout,"(PID %d) %s: You must run as superuser to lock memory for %s\n",
			getpid(),xgp->progname, sp);
		return;
	}
#endif
	status = mlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not lock %d bytes of memory for %s\n",
				getpid(),xgp->progname,bsize,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_lock_memory() */
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
	if (xgp->global_options&GO_REALLYVERBOSE);
		fprintf(xgp->output,"%s: INFORMATION: Assigned processor %d to pid %d threadid %d \n",
			xgp->progname,
			p->processor,
			p->mypid,
			p->mythreadid);
	return;
#elif (AIX)
	int32_t status;
	if (xgp->global_options & GO_REALLYVERBOSE)
		fprintf(xgp->output, "Binding process/thread %d/%d to processor %d\n",p->mypid, p->mythreadid, p->processor);
	status = bindprocessor( BINDTHREAD, p->mythreadid, p->processor );
	if (status) {
		fprintf(xgp->errout,"%s: Processor assignment failed for target %d to processor %d, thread ID %d, process ID %d\n",
			xgp->progname, p->my_target_number, p->processor, p->mythreadid, p->mypid);
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
void
xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp) {
	int32_t status; /* status of a system call */

	// If the nomemlock option is set then do not unlock memory because it was never locked
	if (xgp->global_options & GO_NOMEMLOCK)
		return;
#if (AIX)
#ifdef notdef
	status = plock(UNLOCK);
	if (status) {
		fprintf(xgp->errout,"(PID %d) %s: AIX Could not unlock memory for %s\n",
				getpid(),xgp->progname, sp);
		perror("Reason");
	}
#endif
	return;
#else
#if (IRIX || SOLARIS || LINUX || OSX || FREEBSD)
	if (getuid() != 0) {
		return;
	}
#endif
	status = munlock((char *)bp, bsize);
	if (status < 0) {
			fprintf(xgp->errout,"(PID %d) %s: Could not unlock memory for %s\n",
				getpid(),xgp->progname,sp);
		perror("Reason");
	}
#endif
} /* end of xdd_unlock_memory() */
/*----------------------------------------------------------------------------*/
/* xdd_random_int() - returns a random integer
 */
int
xdd_random_int(void) {
#ifdef  LINUX


	if (xgp->random_initialized == 0) {
		initstate(72058, xgp->random_init_state, 256);
		xgp->random_initialized = 1;
	}
#endif
#ifdef WIN32
	return(rand());
#else
	return(random());
#endif
} /* end of xdd_random_int() */
/*----------------------------------------------------------------------------*/
/* xdd_random_float() - returns a random floating point number in double.
 */
double
xdd_random_float(void) {
	double rm;
	double recip;
	double rval;
#ifdef WIN32
	return((double)(1.0 / RAND_MAX) * rand());
#elif LINUX
	rm = RAND_MAX;
#else
	rm = 2^31-1;
#endif
	recip = 1.0 / rm;
	rval = random();
	rval = recip * rval;
	if (rval > 1.0)
		rval = 1.0/rval;
	return(rval);
} /* end of xdd_random_float() */
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
/*----------------------------------------------------------------------------*/
/* xdd_open_target() - open the target device and do all necessary 
 * sanity checks.  If the open fails or the target cannot be used  
 * with DIRECT I/O then this routine will exit the program.   
 */
#ifdef WIN32
HANDLE
#else
int32_t
#endif 
xdd_open_target(ptds_t *p) {
	char target_name[MAX_TARGET_NAME_LENGTH]; /* target directory + target name */
#ifdef WIN32
	HANDLE hfile;  /* The file handle */
	LPVOID lpMsgBuf; /* Used for the error messages */
	unsigned long flags; /* Open flags */
	DWORD disp;

	/* create the fully qualified target name */
	memset(target_name,0,sizeof(target_name));
	if (strlen(p->targetdir) > 0)
		sprintf(target_name, "%s%s", p->targetdir, p->target);
	else sprintf(target_name, "%s",p->target);
	
	// Check to see if this is a "device" target - must have \\.\ before the name
	// Otherwise, it is assumed that this is a regular file
	if (strncmp(target_name,"\\\\.\\",4) == 0) { // This is a device file *not* a regular file
		p->target_options |= TO_DEVICEFILE;
		p->target_options &= ~TO_REGULARFILE;
	}
	else { // This is a regular file *not* a device file
		p->target_options &= ~TO_DEVICEFILE;
		p->target_options |= TO_REGULARFILE;
	}

	// We can only create new files if the target is a regular file (not a device file)
	if ((p->target_options & TO_CREATE_NEW_FILES) && (p->target_options & TO_REGULARFILE)) { 
		// Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}

	/* open the target */
	if (p->target_options & TO_DIO) 
		flags = FILE_FLAG_NO_BUFFERING;
	else flags = 0;
	/* Device files and files that are being read MUST exist
	 * in order to be opened. For files that are being written,
	 * they can be created if they do not exist or use the existing one.
	 */
	if ((p->target_options & TO_DEVICEFILE) || (p->rwratio == 1.0))
		disp = OPEN_EXISTING; 
	else if (p->rwratio < 1.0)
		disp = OPEN_ALWAYS;
	else disp = OPEN_EXISTING;

	pclk_now(&p->open_start_time); // Record the starting time of the open

	if (p->rwratio < 1.0)  { /* open for write operations */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if (p->rwratio == 0.0) { /* write only */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if ((p->rwratio > 0.0) && p->rwratio < 1.0) { /* read/write mix */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else { /* open for read operations */
		hfile = CreateFile(target_name, 
			GENERIC_READ, 
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	}
	if (hfile == INVALID_HANDLE_VALUE) {
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
		fprintf(xgp->errout,"%s: xdd_open_target: Could not open target for %s: %s\n",
			xgp->progname,(p->rwratio < 1.0)?"write":"read",target_name);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			return((void *)-1);
	}
	
	pclk_now(&p->open_end_time); // Record the ending time of the open
	return(hfile);
#else /* UNIX-style open */
// --------------------------------- Unix-style Open -----------------------------------
#if ( IRIX )
	struct dioattr dioinfo; /* Direct IO information */
	struct flock64 flock; /* Structure used by preallocation fcntl */
#endif
#if (IRIX || SOLARIS || AIX )
	struct stat64 statbuf; /* buffer for file statistics */
#elif ( LINUX || OSX || FREEBSD )
	struct stat statbuf; /* buffer for file statistics */
#endif
	int32_t  i; /* working variable */
	int32_t  status; /* working variable */
	int32_t  fd; /* the file descriptor */
	int32_t  flags; /* file open flags */
	char	*bnp; /* Pointer to the base name of the target */

	/* create the fully qualified target name */
	memset(target_name,0,sizeof(target_name));
	if (strlen(p->targetdir) > 0)
		sprintf(target_name, "%s%s", p->targetdir, p->target);
	else sprintf(target_name, "%s",p->target);

	if (p->target_options & TO_CREATE_NEW_FILES) { // Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}
	/* Set the open flags according to specific OS requirements */
	flags = O_CREAT;
#if (SOLARIS || AIX)
	flags |= O_LARGEFILE;
#endif
#if (AIX || LINUX)
	/* setup for DIRECTIO for AIX & perform sanity checks */
	if (p->target_options & TO_DIO) {
		/* make sure it is a regular file, otherwise fail */
		flags |= O_DIRECT;
	}
#endif
	/* Stat the file before it is opened */
#if (IRIX || SOLARIS || AIX )
	status = stat64(target_name,&statbuf);
#else
	status = stat(target_name,&statbuf);
#endif
	// If stat did not work then it could be that the file does not exist.
	// For write operations this is ok because the file will just be created.
	// Hence, if the errno is ENOENT then set the target file type to
	// be a "regular" file, issue an information message, and continue.
	// If this is a read or a mixed read/write operation then issue an
	// error message and return a -1 indicating an error on open.
	//
	if (status < 0) { 
		if ((ENOENT == errno) && (p->rwratio == 0.0)) { // File does not yet exist
			p->target_options |= TO_REGULARFILE;
			fprintf(xgp->errout,"%s: (%d): xdd_open_target: NOTICE: target %s does not exist so it will be created.\n",
				xgp->progname,
				p->my_target_number,
				target_name);
		} else { // Something is wrong with the file
			fprintf(xgp->errout,"%s: (%d): xdd_open_target: ERROR: Could not stat target: %s\n",
				xgp->progname,
				p->my_target_number,
				target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	}
	/* If this is a regular file, and we are trying to read it, then
	 * check to see that we are not going to read off the end of the
	 * file. Send out a WARNING if this is a possibility
	 */
	if (S_ISREG(statbuf.st_mode)) {  // Is this a Regular File?
		p->target_options |= TO_REGULARFILE;
	} else if (S_ISDIR(statbuf.st_mode)) { // Is this a Directory?
		p->target_options |= TO_REGULARFILE;
		fprintf(xgp->errout,"%s: (%d): xdd_open_target: WARNING: Target %s is a DIRECTORY\n",
			xgp->progname,
			p->my_target_number,
			target_name);
		fflush(xgp->errout);
	} else if (S_ISCHR(statbuf.st_mode)) { // Is this a Character Device?
		p->target_options |= TO_DEVICEFILE;
		bnp = basename(target_name);
		if (strncmp(bnp, "sg", 2) == 0 ) // is this an "sg" device? 
			p->target_options |= TO_SGIO; // Yes - turn on SGIO Mode
	} else if (S_ISBLK(statbuf.st_mode)) {  // Is this a Block Device?
		p->target_options |= TO_DEVICEFILE;
	} else if (S_ISSOCK(statbuf.st_mode)) {  // Is this a Socket?
		p->target_options |= TO_SOCKET;
	}
	/* If this is a regular file, and we are trying to read it, then
 	* check to see that we are not going to read off the end of the
 	* file. Send out a WARNING if this is a possibility
 	*/
	if ( (p->target_options & TO_REGULARFILE) && !(p->rwratio < 1.0)) { // This is a purely read operation
		if (p->bytes_to_xfer_per_pass > statbuf.st_size) { // Check to make sure that the we won't read off the end of the file
		fprintf(xgp->errout,"%s (%d): xdd_open_target: WARNING! The target file <%lld bytes> is shorter than the the total requested transfer size <%lld bytes>\n",
			xgp->progname,
			p->my_target_number,
			(long long)statbuf.st_size, 
			(long long)p->bytes_to_xfer_per_pass);
		fflush(xgp->errout);
		}
	}

	// Time to actually peform the OPEN operation

	pclk_now(&p->open_start_time); // Record the starting time of the open

	/* open the target */
	if (p->rwratio == 0.0) {
		if (p->target_options & TO_SGIO) {
#if (LINUX)
			fd = open(target_name,flags|O_RDWR, 0777); /* Must open RDWR for SGIO */
			i = (p->block_size*p->reqsize);
			status = ioctl(fd, SG_SET_RESERVED_SIZE, &i);
			if (status < 0) {
				fprintf(xgp->errout,"%s: xdd_open_target: SG_SET_RESERVED_SIZE error - request for %d bytes denied",
					xgp->progname, 
					(p->block_size*p->reqsize));
			}
			status = ioctl(fd, SG_GET_VERSION_NUM, &i);
			if ((status < 0) || (i < 30000)) 
				fprintf(xgp->errout, "%s: xdd_open_target: sg driver prior to 3.x.y - specifically %d\n",xgp->progname,i);
#endif // LINUX SGIO open stuff
		} else fd = open(target_name,flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		if (p->target_options & TO_SGIO) {
#if (LINUX)
				fd = open(target_name,flags|O_RDWR, 0777); /* Must open RDWR for SGIO  */
				i = (p->block_size*p->reqsize);
				status = ioctl(fd, SG_SET_RESERVED_SIZE, &i);
				if (status < 0) {
					fprintf(xgp->errout,"%s: xdd_open_target: SG_SET_RESERVED_SIZE error - request for %d bytes denied",
						xgp->progname, 
						(p->block_size*p->reqsize));
				}
				status = ioctl(fd, SG_GET_VERSION_NUM, &i);
				if ((status < 0) || (i < 30000)) {
					fprintf(xgp->errout, "%s: xdd_open_target: sg driver prior to 3.x.y - specifically %d\n",xgp->progname,i);
				}
#endif // LINUX SGIO open stuff
		} else fd = open(target_name,flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		fd = open(target_name,flags|O_RDWR, 0666);
	}

	pclk_now(&p->open_end_time); // Record the ending time of the open

	// Check the status of the OPEN operation to see if it worked
	if (fd < 0) {
			fprintf(xgp->errout,"%s (%d): xdd_open_target: could not open target: %s\n",
				xgp->progname,
				p->my_target_number,
				target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}
	/* Stat the file so we can do some sanity checks */
#if (IRIX || SOLARIS || AIX )
	i = fstat64(fd,&statbuf);
#else
	i = fstat(fd,&statbuf);
#endif
	if (i < 0) { /* Check file type */
		fprintf(xgp->errout,"%s (%d): xdd_open_target: could not fstat target: %s\n",
			xgp->progname,
			p->my_target_number,
			target_name);
		fflush(xgp->errout);
		perror("reason");
		return(-1);
	}
#if (IRIX || SOLARIS )
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: target %s must be a regular file when used with the -dio flag\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				return(-1);
			}
#if ( IRIX ) /* DIO for IRIX */
			/* set the DIRECTIO flag */
			flags = fcntl(fd,F_GETFL);
			i = fcntl(fd,F_SETFL,flags|FDIRECT);
			if (i < 0) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: could not set DIRECTIO flag for: %s\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				perror("reason");
				if (EINVAL == errno) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: target %s is not on an EFS or xFS filesystem\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				}
				return(-1);
		}
		i = fcntl(fd, F_DIOINFO, &dioinfo);
		if (i != 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: Cannot get DirectIO info for target %s\n",
					p->my_target_number,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("Reason");
		} else { /* check the DIO size & alignment */
			/* If the io size is less than the minimum then exit */
			if (p->iosize < dioinfo.d_miniosz) {
fprintf(xgp->errout,"%s (%d): xdd_open_target: The iosize of %d bytes is smaller than the minimum DIRECTIO iosize <%d bytes> for target %s\n",
					xgp->progname,
					p->my_target_number,
					p->iosize,
					dioinfo.d_miniosz,
					target_name);
				fflush(xgp->errout);
				return(-1);
			}
			if (p->iosize > dioinfo.d_maxiosz) {
fprintf(xgp->errout,"(%d) %s: xdd_open_target: The iosize of %d bytes is greater than the maximum DIRECTIO iosize <%d bytes> for target %s\n",
					p->my_target_number,xgp->progname,p->iosize,dioinfo.d_maxiosz,target_name);
				fflush(xgp->errout);
				return(-1);
			}
			/* If the iosize is not a multiple of the alignment
			 * then exit 
			 */
			if ((p->iosize % dioinfo.d_miniosz) != 0) {
fprintf(xgp->errout,"(%d) %s: xdd_open_target: The iosize of %d bytes is not an integer mutiple of the DIRECTIO iosize <%d bytes> for target %s\n",
					p->my_target_number,xgp->progname,p->iosize,dioinfo.d_miniosz,target_name);
				fflush(xgp->errout);
				return(-1);
			}
		} /* end of DIO for IRIX */
#elif SOLARIS /* DIO for SOLARIS */
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
#endif /* end of DIO for SOLARIS */
	} /* end of IF stmnt that opens with DIO */
#if (IRIX)
	/* Preallocate storage space if necessary */
	if (p->preallocate) {
		flock.l_whence = SEEK_SET;
		flock.l_start = 0;
		flock.l_len = (uint64_t)(p->preallocate * p->block_size);
		i = fcntl(fd, F_RESVSP64, &flock);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: **WARNING** Preallocation of %lld bytes <%d blocks> failed for target %s\n",
				p->my_target_number,
				xgp->progname,(long long)(p->preallocate * p->block_size),
				p->preallocate,
				target_name);
			fflush(xgp->errout);
			perror("Reason");
		}
	} /* end of Preallocation */
#endif /* Preallocate */
#endif
#if (SNFS)
	if (p->queue_depth > 1) { /* Set this SNFS flag so that we can do parallel writes to a file */
		status = fcntl(fd, F_CVSETCONCWRITE);
		if (status < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set SNFS flag F_CVSETCONCWRITE for: %s\n",
				p->my_target_number,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
		} else {
			if (xgp->global_options & GO_VERBOSE) {
				fprintf(xgp->output,"(%d) %s: xdd_open_target: SNFS flag F_CVSETCONCWRITE set for parallel I/O for: %s\n",
					p->my_target_number,xgp->progname,target_name);
				fflush(xgp->output);
			}
		}
	}
#endif
	return(fd);
#endif /* Unix open stuff */  
} /* end of xdd_open_target() */

/*----------------------------------------------------------------------------*/
/* xdd_show_ptds() - Display values in the specified Per-Target-Data-Structure 
 * Note: CLO == Command Line Option
 */
void
xdd_show_ptds(ptds_t *p) {
		fprintf(stderr,"********* Start of PTDS for target my_target_number=%d **********\n",p->my_target_number);
		//p->my_qthread_number = 0;
		fprintf(stderr,"mypid=%d\n",p->mypid);
		//p->mythreadid = 0; // This is set later by the actual thread 
		//p->thread_complete = 0; // set upon creation
		//p->nextp = 0; // set upon creation, used when other qthreads are created
		//p->pm1 = 0; // set upon creation
		//p->rwbuf = 0; // set during rwbuf allocation
		//p->rwbuf_shmid = -1; // set upon creation of a shared memory segment
		//p->rwbuf_save = 0; // used by the rwbuf allocation routine
		//p->targetdir = DEFAULT_TARGETDIR; // can be changed by CLO
		fprintf(stderr,"target=%s\n",p->target);
		//sprintf(p->targetext,"%08d",1);  // can be changed by CLO
		fprintf(stderr,"reqsize=%d\n",p->reqsize); 
		//p->throttle = DEFAULT_THROTTLE;
		//p->throttle_variance = DEFAULT_VARIANCE;
		//p->throttle_type = PTDS_THROTTLE_BW;
		fprintf(stderr,"ts_options=0x%16x\n",p->ts_options);
		fprintf(stderr,"target_options=0x%16x\n",p->target_options);
		//p->time_limit = DEFAULT_TIME_LIMIT;
		fprintf(stderr,"numreqs=%lld\n",(long long)p->numreqs);
		fprintf(stderr,"flushwrite=%lld\n",(long long)p->flushwrite);
		fprintf(stderr,"bytes=%lld\n",(long long)p->bytes); 
		fprintf(stderr,"kbytes=%lld\n",(long long)p->kbytes); 
		fprintf(stderr,"mbytes=%lld\n",(long long)p->mbytes); 
		//p->report_threshold = DEFAULT_REPORT_THRESHOLD;
		//p->start_offset = DEFAULT_STARTOFFSET;
		//p->pass_offset = DEFAULT_PASSOFFSET;
		//p->preallocate = DEFAULT_PREALLOCATE;
		//p->queue_depth = DEFAULT_QUEUEDEPTH;
		//p->data_pattern_filename = (char *)DEFAULT_DATA_PATTERN_FILENAME;
		//p->data_pattern = DEFAULT_DATA_PATTERN;
		//p->data_pattern_length = DEFAULT_DATA_PATTERN_LENGTH;
		//p->data_pattern_prefix = DEFAULT_DATA_PATTERN_PREFIX;
		//p->data_pattern_prefix_length = DEFAULT_DATA_PATTERN_PREFIX_LENGTH;
		fprintf(stderr,"block_size=%d\n",p->block_size);
		//p->mem_align = getpagesize();
        //p->my_current_elapsed_time = 0;
        //p->my_current_end_time = 0;
        //p->my_current_start_time = 0;
        //p->my_end_time = 0;
        //p->my_start_time = 0;
		//p->processor = -1;
		//p->start_delay = DEFAULT_START_DELAY;
		//p->start_trigger_time = 0; /* Time to trigger another target to start */
		//p->stop_trigger_time = 0; /* Time to trigger another target to stop */
		//p->start_trigger_op = 0; /* Operation number to trigger another target to start */
		//p->stop_trigger_op = 0; /* Operation number  to trigger another target to stop */
		//p->start_trigger_percent = 0; /* Percentage of ops before triggering another target to start */
		//p->stop_trigger_percent = 0; /* Percentage of ops before triggering another target to stop */
		//p->start_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to start */
		//p->stop_trigger_bytes = 0; /* Number of bytes to transfer before triggering another target to stop */
		//p->start_trigger_target = -1; /* The number of the target to send the start trigger to */
		//p->stop_trigger_target = -1; /* The number of the target to send the stop trigger to */
		//p->run_status = 1;   /* This is the status of this thread 0=not started, 1=running */
		//p->trigger_types = 0;
		//p->ls_master = -1; /* The default master number  */
		//p->ls_slave  = -1; /* The default slave number */
		//p->ls_interval_type  = 0; /* The default interval type */
		//p->ls_interval_value  = 0; /* The default interval value  */
		//p->ls_interval_units  = "not defined"; /* The default interval units  */
		//p->ls_task_type  = 0; /* The default task type */
		//p->ls_task_value  = 0; /* The default task value  */
		//p->ls_task_units  = "not defined"; /* The default task units  */
		//p->ls_task_counter = 0; /* the default task counter */
		/* Init the seeklist header fields */
		//p->seekhdr.seek_options = 0;
		//p->seekhdr.seek_range = DEFAULT_RANGE;
		//p->seekhdr.seek_seed = DEFAULT_SEED;
		//p->seekhdr.seek_interleave = DEFAULT_INTERLEAVE;
		//p->seekhdr.seek_iosize = DEFAULT_REQSIZE*DEFAULT_BLOCKSIZE;
		//p->seekhdr.seek_num_rw_ops = DEFAULT_NUMREQS;
		//p->seekhdr.seek_total_ops = DEFAULT_NUMREQS;
		//p->seekhdr.seek_NumSeekHistBuckets = DEFAULT_NUM_SEEK_HIST_BUCKETS;/* Number of buckets for seek histogram */
		//p->seekhdr.seek_NumDistHistBuckets = DEFAULT_NUM_DIST_HIST_BUCKETS;/* Number of buckets for distance histogram */
		//p->seekhdr.seek_savefile = NULL; /* file to save seek locations into */
		//p->seekhdr.seek_loadfile = NULL; /* file from which to load seek locations from */
		//p->seekhdr.seek_pattern = "sequential";
		/* Init the read-after-write fields */
		//p->raw_sd = 0; /* raw socket descriptor */
		//p->raw_hostname = NULL;  /* Reader hostname */
		//p->raw_lag = DEFAULT_RAW_LAG; 
		//p->raw_port = DEFAULT_RAW_PORT;
		//p->raw_trigger = PTDS_RAW_MP; /* default to a message passing */
		
		fprintf(stderr,"+++++++++++++ End of PTDS for target my_target_number=%d +++++++++++++\n",p->my_target_number);
} /* end of xdd_show_ptds() */ 
 
 
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
