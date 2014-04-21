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
 * This file contains the subroutines necessary to parse the command line
 * arguments  set up all the global and target-specific variables.
 */

#include "xint.h"
#include "parse.h"
extern xdd_func_t xdd_func[];

/*----------------------------------------------------------------------------*/
/* xdd_parse_args() - Parameter parsing - called by xdd_parse().
 * This routine will scan the command line options from left to right as they are
 * specified on the command line.
 * The xdd function table contains all the command line options and a pointer
 * to the xddfunc_xxxx() that handles the specifics of each command line option.
 * This routine will take a given command line option, strip off the preceeding "-",
 * scan the xddfunc_xxxx() table for this specific option (or its alternate), and call
 * the specified xddfunc_xxxx() subroutine to process the commandline option and
 * any arguments to that option. 
 * For example, for the "-op write" command line option, this routine will scan the
 * xddfunc table for the "op" command line option and subsequently call the routine
 * specified in the table entry for "op" to handle this option. The routines in the
 * table that handle the options are called xddfunc_xxxx() where xxxx is generally
 * the name of the function. Thus, for this example, the xddfunc_operation() routine
 * will be invoked to do whatever is necessary for the -op command line option. It is
 * worth noting that the "-op" is an alternate for the actual option name which is 
 * "-operation" but I am lazy so there you have it. 
 *
 * NOTE: There are other characters that look like a "-" (dash or minus sign - 0x2D)
 * but are the 16-bit equivalent and this subroutine does not recognize them. 
 * Hence it is possible that the parse_args subroutine will go right off the edge 
 * and core dump if the weird dashes are used.
 */
void
xdd_parse_args(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) {
    int funci;      // Index into xdd_func[]
    int argi;       // Index into argv
    int arg_count;  // Number of args left to look at
    int status;     // Status from the option-specific subroutine
    int not_found;  // Indicates that an option was not found
    int invalid;    // Indicates an invalid option from one of the xddfunc routines
    char **argvp;

    arg_count=argc-1;
    argi = 1; // argv[0] is the program name so we start at argv[1]
    while (arg_count) { // Ok, here we go - through all the argurments in the command line...
        while (argi < argc && *(argv[argi]) != '-') { // command line options must be proceeded with a "-" or they are ignored
                argi++;
        } 
        if (argi >= argc || arg_count == 0)
	    break; // Somehow we got to the end of the command line and did not find any valid options
        
		// At this point argv[], uint32_t flags must be pointing to a reasonably legit option so
		// set up to scan the xddfunc table for this option
        funci = 0;
        not_found = 1;
        invalid = 0;
        while (xdd_func[funci].func_name) { // The last entry in the xddfunc table is 0 which will kick us out of this loop.
            if ((strcmp(xdd_func[funci].func_name, (char *)((argv[argi])+1)) == 0) || 
                (strcmp(xdd_func[funci].func_alt, (char *)((argv[argi])+1)) == 0)) {
                argvp = &(argv[argi]);
                status = (int)xdd_func[funci].func_ptr(planp, arg_count, argvp, flags);

                if (status == 0) {
                    invalid = 1;
                    break;
                } else if (status == -1) exit(XDD_RETURN_VALUE_INVALID_OPTION);
                argi += status;
                arg_count -= status;
                not_found = 0;
                break;
            }
            funci++;
        }
        // Check to see if things worked...
        if (invalid) // Indicates a valid option but invalid arguments to the option most likely
            exit(XDD_RETURN_VALUE_INVALID_ARGUMENT);

        if (not_found) { // Indicates that the specified option was not found in the xddfunc table. 
            xddfunc_invalid_option(argi+1, &(argv[argi]), flags);
            exit(XDD_RETURN_VALUE_INVALID_OPTION);
        }      
    } // End of WHILE loop that processes all arguments on the command line
} /* End of xdd_parse_args() */

/*----------------------------------------------------------------------------*/
/* xdd_parse() - Command line parser.
 */
void
xdd_parse(xdd_plan_t *planp, int32_t argc, char *argv[]) {

	
	if (argc < 1) { // Ooopppsss - nothing specified...
		fprintf(stderr,"Error: No command line options specified\n");
		xdd_usage(0);
		exit(XDD_RETURN_VALUE_INVALID_OPTION);
	}
	/* parse the command line arguments */
	xdd_parse_args(planp, argc, argv, XDD_PARSE_PHASE1);

	xdd_parse_args(planp, argc, argv, XDD_PARSE_PHASE2);
	if (planp->target_datap[0] == NULL) {
		fprintf(xgp->errout,"You must specify a target device or filename\n");
		xdd_usage(0);
		exit(XDD_RETURN_VALUE_INVALID_ARGUMENT);
	}

	// Build the Target Data Struct substructure for all targets
	xdd_build_target_data_substructure(planp);

} /* end of xdd_parse() */
/*----------------------------------------------------------------------------*/
/* xdd_usage() - Display usage information
 */
void
xdd_usage(int32_t fullhelp) {
    int i,j;

    fprintf(stderr,"Usage: %s command-line-options\n",xgp->progname);
    i = 0;
    while(xdd_func[i].func_name) {
		if (xdd_func[i].flags & XDD_FUNC_INVISIBLE) { // This function is not visible
			i++;
			continue;
		}
        fprintf(stderr,"%s",xdd_func[i].help);
        if (fullhelp) {
            j=0;
            while (xdd_func[i].ext_help[j] && (j < XDD_EXT_HELP_LINES)) {
                fprintf(stderr,"%s",xdd_func[i].ext_help[j]);
                j++;
            }
            fprintf(stderr,"\n");
        }
        i++;
    }
}
/*----------------------------------------------------------------------------*/
/* xdd_check_option() - Check to see if the specified option exists.
 * If the option is in the option table, then return 1
 * Otherwise, return 0.
 * If the option pointer is 0 then return -1 - this should never happen.
 */
int
xdd_check_option(char *op) {
	int i;


	if (op == 0) {
		fprintf(stderr,"xdd_check_option: no option specified to check for\n");
		return(-1);
	}
	
    i = 0;
    while(xdd_func[i].func_name) {
		if ((strcmp(xdd_func[i].func_name, op) == 0) || 
			(strcmp(xdd_func[i].func_alt, op) == 0)) { 
			return(1);
        }
        i++;
    }
    return(0);
}
/*----------------------------------------------------------------------------*/
/* xdd_process_paramfile() - process the parameter file. 
 */
int32_t
xdd_process_paramfile(xdd_plan_t *planp, char *fnp) {
	int32_t  i;   /* working variable */
	int32_t  fd;   /* file descriptor for paramfile */
	int32_t  newsize;
	int32_t  bytesread; /* The number of bytes actually read from the param file */
	int32_t  argc;
	char *argv[8192];
	char *cp;
	char *parambuf;
	struct stat statbuf;


	/* open the parameter file */
	fd = open(fnp,O_RDONLY);
	if (fd < 0) {
		fprintf(xgp->errout, "%s: could not open parameter file '%s'\n",
			xgp->progname, fnp);
		perror("  reason");
		return(-1);
	}
	/* stat the file to get the file size in bytes */
	i = fstat(fd, &statbuf);
	if (i < 0) {
		fprintf(xgp->errout, "%s: could not stat parameter file '%s'\n",
			xgp->progname, fnp);
		perror("  reason");
		return(-1);
	}
	/* allocate a buffer for the file */
	parambuf = malloc(statbuf.st_size*2);
	if (parambuf == 0) {
		fprintf(xgp->errout,"%s: could not allocate %d bytes for parameter file\n",
			xgp->progname, (int32_t)statbuf.st_size);
		perror("  reason");
		return(-1);
	}
	memset(parambuf,0,statbuf.st_size*2);
	/* read in the entire commands file. The "bytesread" is used as the real size of the file since
	 * in *some* brain-damaged operating systems the number of bytes read is not equal to the
	 * file size reported by fstat.
	 */
	bytesread = read(fd,parambuf,statbuf.st_size*2);
	if (bytesread < 0) {
		fprintf(xgp->errout, "%s: Error: only read %d bytes of %d from parameter file\n",
			xgp->progname, bytesread, (int32_t)statbuf.st_size);
		perror("  reason");
		return(-1);
	}
	/* close the parameter file */
	close(fd);
	/* set argv[0] to the program name for consistency */
	argv[0] = xgp->progname;
	argc = 0;
	cp = parambuf;
	i = 0;
	if ((*cp == ' ') || (*cp == '\t') || (*cp == '\n')) {
		while (((*cp == ' ') || (*cp == '\t') || (*cp != '\n')) && 
			(i < bytesread)) {
			cp++;
			i++;
		}
	}
	/* at this point i points to the start of the first token in the file */
	if (i >= bytesread) return(SUCCESS); /* This means that it was essentially an empty file */
	newsize = bytesread - i;
	for (i = 0; i < newsize; i++) {
		if (*cp == '#') { // this is a comment line - skip to next line
			while ( (*cp != '\n' ) && (i < newsize)) {
				cp++;
				i++;
			}
			if (i >= newsize) break;
			cp++;
			i++;
			continue;
		}

		argc++;
		argv[argc] = cp;
		/* skip over the token */
		while ((*cp != ' ') && (*cp != '\t') && (*cp != '\n') && 
			(i < newsize)) {
			cp++;
			i++;
		}
		*cp = '\0';
		if (i >= newsize) break;
		cp++;
		/* skip over blanks looking for start of next token */
		while (((*cp == ' ') || (*cp == '\t') || (*cp == '\n')) && 
			(i < newsize)) {
			cp++;
			i++;
		}
		if (i >= newsize) break;
	}
	argc++;   /* add one for the 0th argument */
	xdd_parse_args(planp,argc,argv,XDD_PARSE_PHASE1);
	xdd_parse_args(planp,argc,argv,XDD_PARSE_PHASE2);
	return(SUCCESS);

} /* end of xdd_process_param() */
/*----------------------------------------------------------------------------*/
/* xdd_parse_target_number() - return the target number for this option.
 * The calling routine passes in the number of arguments left in the command
 * line. If the number of arguments remaining is 0 then we will not check 
 * to see if there are any other suboptions beacuse this would cause a segv.
 * In this case, we just set the target number to -1 (all targets) and return 0.
 *
 * The calling routine passes in a pointer to the first argument of the option.
 * For example, if the option is 
 *       -reqsize  target   3        1024 
 * then  ^argv[0]  ^argv[1] ^argv[2] ^argv[3]
 *
 * The calling routine also passes a pointer to the place to return the target
 * number. This routine will set the target number to the correct value and
 * return the number of arguments that were processed. 
 * 
 * If the "target" suboption is specified for this option then put the specified
 * target number into the location pointed to by *target_number. In the example 
 * above  *target_number=3 and a value of 2 will be returned since two arguments
 * were processed ( "target" and "3" ). 
 *
 * If the "previous" or "prev" or "p" suboption is specified then return
 * the number of the last target specified by the "-targets 1" option.
 * For example, "-reqsize prev 1024" will set "target_number" to the
 * current number of targets that have been specified minus 1 and return 1.
 *
 * If there is no "target" or "previous" suboption then return a -1 as 
 * the target number. For example, if the option "-reqsize 1024" is specified
 * then "target_number" is set to -1 and a value of 0 will be returned.
 *
 */
int
xdd_parse_target_number(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags, int *target_number) {
	int32_t tn; // Temporary target number 
	target_data_t	*tdp; // Pointer to a target_data for the specified target number


    if (argc <= 1) { // not enough arguments in this line to specify a valid target number
        *target_number = -1;
        return(0);
    }
	if (strcmp(argv[1], "target") == 0) {
        if (argc < 3) { // Not enough arguments in this line to specify a valid target number 
            fprintf(xgp->errout,"%s: ERROR: No target number specified for option %s\n",xgp->progname, argv[0]);
            return(-1);
		} else { // The next argument specifies a target number as an integer
		    tn = atoi(argv[2]);
		    if (tn >= MAX_TARGETS) { // Check to make sure the target number is a reasonable number
                fprintf(xgp->errout,"%s: ERROR: Target number of %d for option %s is too high - must be an integer from 0 to %d\n",
			    	xgp->progname, tn, argv[0], (MAX_TARGETS-1));
                return(-1);
			}
		}
		// Make sure the Target Data Struct for this target exists - if it does not, the xdd_get_target_data() subroutine will create one
		tdp = xdd_get_target_datap(planp, tn, "xdd_parse_target_number, target");
		if (tdp == NULL) return(-1);

		// At this point the target number is valid and a Target Data Struct exists for this target
		*target_number = tn;
        return(2);
	} else { // The user could have specified the word "previous" to indicate the current target number minus 1
		if ((strcmp(argv[0], "previous") == 0) || (strcmp(argv[0], "prev") == 0) || (strcmp(argv[0], "p") == 0)) {
			tn = planp->number_of_targets - 1;
			// Make sure the Target Data Struct for this target exists - if it does not, the xdd_get_target_data() subroutine will create one
			tdp = xdd_get_target_datap(planp, tn, "xdd_parse_target_number, previous");
			if (tdp == NULL) return(-1);

		    // At this point the target number is valid and a Target Data Struct exists for this target
            *target_number = tn;
            return(1);
		} else { // Apparently the user did not specify a target number for this option.
		    *target_number = -1;
		    return(0);
		}
	} 
}/* end of xdd_parse_target_number() */

/*----------------------------------------------------------------------------*/
/* xdd_get_target_datap() - return a pointer to the Target Data Struct for the specified target
 */
target_data_t * 
xdd_get_target_datap(xdd_plan_t *planp, int32_t target_number, char *op) {
    target_data_t *tdp;

    if (0 == planp->target_datap[target_number]) {
		planp->target_datap[target_number] = malloc(sizeof(target_data_t));
		if (planp->target_datap[target_number] == NULL) {
	    	fprintf(xgp->errout,"%s: ERROR: Could not get a pointer to a Target Data Struct for target number %d for option %s\n",
		    	xgp->progname, target_number, op);
	    	return(NULL);
		}
		// Zero out the memory first
		memset((unsigned char *)planp->target_datap[target_number], 0, sizeof(target_data_t));
		tdp = planp->target_datap[target_number];

		// Allocate and initialize the data pattern structure
		tdp->td_dpp = (xint_data_pattern_t *)malloc(sizeof(xint_data_pattern_t));
		if (tdp->td_dpp == NULL) {
	    	fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for Target Data Struct Data Pattern Structure for target %d\n",
		    	xgp->progname, (int)sizeof(xint_data_pattern_t), target_number);
	    	return(NULL);
		}
		xdd_data_pattern_init(tdp->td_dpp);
	
		if (xgp->global_options & GO_EXTENDED_STATS) 
	    	xdd_get_esp(tdp);
	
		if (planp->plan_options & PLAN_ENDTOEND) {
			if (NULL == tdp->td_e2ep) { // If there is no e2e struct then allocate one.
	    		tdp->td_e2ep = xdd_get_e2ep();
				if (NULL == tdp->td_e2ep) {
	    			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for Target Data Struct END TO END Data Structure for target %d\n",
		    			xgp->progname, (int)sizeof(xint_data_pattern_t), target_number);
	    			return(NULL);
				}
			}
		}
	
		// Initialize the new Target Data Struct and lets rock and roll!
		xdd_init_new_target_data(tdp, target_number);
		planp->target_average_resultsp[target_number] = malloc(sizeof(results_t));
		if (planp->target_average_resultsp[target_number] == NULL) {
	    	fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for RESULTS struct for target %d\n",
		    	xgp->progname, (int)sizeof(results_t), target_number);
	    	return(NULL);
		}
	}
	return(planp->target_datap[target_number]);
} /* End of xdd_get_target_datap() */

/*----------------------------------------------------------------------------*/
/* xdd_get_restartp() - return a pointer to the RESTART structure 
 * for the specified target
 */
xint_restart_t *
xdd_get_restartp(target_data_t *tdp) {
	
	if (tdp->td_restartp == 0) { // Since there is no existing Restart Structure, allocate a new one for this target, initialize it, and move on...
		tdp->td_restartp = malloc(sizeof(xint_restart_t));
		if (tdp->td_restartp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for RESTART structure for target %d\n",
			xgp->progname, (int)sizeof(xint_restart_t), tdp->td_target_number);
			return(NULL);
		}
		memset(tdp->td_restartp, 0, sizeof(*tdp->td_restartp));
	}
	return(tdp->td_restartp);
} /* End of xdd_get_restartp() */

/*----------------------------------------------------------------------------*/
/* xdd_get_rawp() - return a pointer to the ReadAfterWrite Data Structure 
 * for the specified target
 */
xint_raw_t *
xdd_get_rawp(target_data_t *tdp) {
	
	if (tdp->td_rawp == 0) { // Since there is no existing RAW structure, allocate a new one for this target, initialize it, and move on...
		tdp->td_rawp = malloc(sizeof(xint_raw_t));
		if (tdp->td_rawp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for RAW structure for target %d\n",
			xgp->progname, (int)sizeof(xint_raw_t), tdp->td_target_number);
			return(NULL);
		}
	}
	return(tdp->td_rawp);
} /* End of xdd_get_rawp() */

/*----------------------------------------------------------------------------*/
/* xdd_get_trigp() - return a pointer to the Triggers Data Structure 
 * for the specified target
 */
xint_triggers_t *
xdd_get_trigp(target_data_t *tdp) {
	
	if (tdp->td_trigp == 0) { // Since there is no existing triggers structure, allocate a new one for this target, initialize it, and move on...
		tdp->td_trigp = malloc(sizeof(xint_triggers_t));
		if (tdp->td_trigp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for TRIGGERS variables for target %d\n",
			xgp->progname, (int)sizeof(xint_triggers_t), tdp->td_target_number);
			return(NULL);
		}
	}
	return(tdp->td_trigp);
} /* End of xdd_get_trigp() */

/*----------------------------------------------------------------------------*/
/* xdd_get_esp() - return a pointer to the Extended Stats Data Structure 
 * for the specified target
 */
xint_extended_stats_t *
xdd_get_esp(target_data_t *tdp) {
	
	if (tdp->td_esp == 0) { // Since there is no existing Extended Stats structure, allocate a new one for this target, initialize it, and move on...
		tdp->td_esp = malloc(sizeof(xint_extended_stats_t));
		if (tdp->td_esp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for EXTENDED STATS variables for target %d\n",
			xgp->progname, (int)sizeof(xint_extended_stats_t), tdp->td_target_number);
			return(NULL);
		}
	}
	return(tdp->td_esp);
} /* End of xdd_get_esp() */

/*----------------------------------------------------------------------------*/
/* xdd_get_throtp() - return a pointer to the XDD Throttle Data Structure 
 */
xint_throttle_t *
xdd_get_throtp(target_data_t *tdp) {

	if (tdp->td_throtp == 0) { // If there is no existing Time Stamp structure, allocate a new one 
		tdp->td_throtp = malloc(sizeof(xint_throttle_t));
		if (tdp->td_throtp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for THROTTLE variables for target %d\n",
			xgp->progname, (int)sizeof(xint_throttle_t), tdp->td_target_number);
			return(NULL);
		}
		// Set default Throttle values in the newly allocated throttle structure
		tdp->td_throtp->throttle = XINT_DEFAULT_THROTTLE;
		tdp->td_throtp->throttle_variance = XINT_DEFAULT_THROTTLE_VARIANCE;
		tdp->td_throtp->throttle_type = XINT_DEFAULT_THROTTLE_TYPE;
	}
	return(tdp->td_throtp);

} /* End of xdd_get_throtp() */

/*----------------------------------------------------------------------------*/
/* xdd_get_tsp() - return a pointer to the Time Stamp Variables
 * for the specified target
 */
//xint_timestamp_t *
//xdd_get_tsp(target_data_t *tdp) {

	//if (tdp->td_tsp == 0) { // If there is no existing Time Stamp structure, allocate a new one 
		//tdp->td_tsp = malloc(sizeof(xint_timestamp_t));
		//if (tdp->td_tsp == NULL) {
			//fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for TIMESTAMP variables for target %d\n",
			//xgp->progname, (int)sizeof(xint_timestamp_t), tdp->td_target_number);
			//return(NULL);
		//}
	//}
	//return(tdp->td_tsp);
//
//} /* End of xdd_get_tsp() */
#if (LINUX)
/*----------------------------------------------------------------------------*/
/* xdd_linux_cpu_count() - return the number of CPUs on  this system
 */
int32_t
xdd_linux_cpu_count(void) {
	size_t	 	cpumask_size; 	/* size of the CPU mask in bytes */
	cpu_set_t 	cpumask; 	/* mask of the CPUs configured on this system */
	int		status; 	/* System call status */
	int32_t 	cpus; 		/* the number of CPUs configured on this system */
	size_t		j;

	cpumask_size = (size_t)sizeof(cpumask);
	status = sched_getaffinity(getpid(), cpumask_size, &cpumask);
	if (status != 0) {
		fprintf(xgp->errout,"%s: WARNING: Error getting the CPU mask - running on a single processor\n",xgp->progname);
		perror("Reason");
		cpus = 1;
	} else {
		cpus = 0;
		for (j=0; j<sizeof(cpumask)*8; j++ ) { /* Increment the cpus by 1 for every bit that is on in the cpu mask */
			if (CPU_ISSET(j,&cpumask)) cpus++;
		}
	}
	return(cpus);
} /* end of xdd_linux_cpu_count() */
#endif

/*----------------------------------------------------------------------------*/
/* xdd_cpu_count() - return the number of CPUs on  this system
 */
int32_t
xdd_cpu_count(void) {
	int32_t		cpus; // Number of CPUs found
#if (DARWIN)
	cpus = 1;
	fprintf(xgp->errout,"%s: WARNING: Multiple processors not supported in this release\n",xgp->progname);
#elif (SOLARIS || AIX)
	/* SOLARIS or AIX */ 
	cpus = sysconf(_SC_NPROCESSORS_ONLN);
#elif (IRIX || WIN32)
	/* IRIX */
	cpus = sysmp(MP_NPROCS,0);
#elif (LINUX)
	cpus = xdd_linux_cpu_count();
#endif
	return(cpus);
} /* end of xdd_cpu_count() */

/*----------------------------------------------------------------------------*/
/* xdd_atohex() - convert the ascii string into real hex numbers.
 * If any of the ascii digits are not in the 0-9, a-f range then substitute
 * zeros. If there are an odd number of ascii digits then add leading zeros.
 * The way this routine works is to copy the incoming ASCII string into a temp
 * buffer and perform the conversions from that buffer into the  buffer
 * pointed to by "destp". That way the actual hex data is where it should be 
 * when we are finished. The source buffer is not touched. This routine 
 * returns the length of the hex data destination buffer in 4-bit nibbles.
 */
int32_t
xdd_atohex(unsigned char *destp, char *sourcep) {
	size_t		i;
	size_t     	length;
	size_t		nibbles;
	int		hi;
	char    	*cp;
	unsigned char 	*ucp;
	char 	*tmpp;


	nibbles = strlen(sourcep);
	tmpp = (char *)malloc((size_t)nibbles+2);
	memset(tmpp, '\0', (size_t)nibbles+2);
	if (nibbles % 2) 
		 strcpy(tmpp,"0"); // Odd length so start with zero
	else strcpy(tmpp,""); // terminate with a null character
	strcat(tmpp,sourcep); // put the string in the tmp area
	length = strlen(tmpp); // Get the real length of the string 
	memset(destp, '\0', length); // clear out the destination area

	hi = 1; // Indicates we are on the high-order nibble of the byte
	ucp = (unsigned char *)destp; // moving pointer to destination 
	for (i=0, cp=tmpp; i < length; i++, cp++) {
		switch(*cp) {
			case '0':
				break;
			case '1':
				if (hi) *ucp |= 0x10; else *ucp |= 0x01;
				break;
			case '2':
				if (hi) *ucp |= 0x20; else *ucp |= 0x02;
				break;
			case '3':
				if (hi) *ucp |= 0x30; else *ucp |= 0x03;
				break;
			case '4':
				if (hi) *ucp |= 0x40; else *ucp |= 0x04;
				break;
			case '5':
				if (hi) *ucp |= 0x50; else *ucp |= 0x05;
				break;
			case '6':
				if (hi) *ucp |= 0x60; else *ucp |= 0x06;
				break;
			case '7':
				if (hi) *ucp |= 0x70; else *ucp |= 0x07;
				break;
			case '8':
				if (hi) *ucp |= 0x80; else *ucp |= 0x08;
				break;
			case '9':
				if (hi) *ucp |= 0x90; else *ucp |= 0x09;
				break;
			case 'a':
				if (hi) *ucp |= 0xa0; else *ucp |= 0x0a;
				break;
			case 'b':
				if (hi) *ucp |= 0xb0; else *ucp |= 0x0b;
				break;
			case 'c':
				if (hi) *ucp |= 0xc0; else *ucp |= 0x0c;
				break;
			case 'd':
				if (hi) *ucp |= 0xd0; else *ucp |= 0x0d;
				break;
			case 'e':
				if (hi) *ucp |= 0xe0; else *ucp |= 0x0e;
				break;
			case 'f':
				if (hi) *ucp |= 0xf0; else *ucp |= 0x0f;
				break;
			case 'A':
				if (hi) *ucp |= 0xa0; else *ucp |= 0x0a;
				break;
			case 'B':
				if (hi) *ucp |= 0xb0; else *ucp |= 0x0b;
				break;
			case 'C':
				if (hi) *ucp |= 0xc0; else *ucp |= 0x0c;
				break;
			case 'D':
				if (hi) *ucp |= 0xd0; else *ucp |= 0x0d;
				break;
			case 'E':
				if (hi) *ucp |= 0xe0; else *ucp |= 0x0e;
				break;
			case 'F':
				if (hi) *ucp |= 0xf0; else *ucp |= 0x0f;
				break;
			default:
				fprintf(xgp->errout, "%s: illegal hex character of %c speficied, using 0 instead\n",xgp->progname, *cp);
				break;
		}
		hi ^= 1;
		if (hi) ucp++;
	}
	free(tmpp);
	return(nibbles); // The length is the number of nibbles

} /* end of xdd_atohex() */ 

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
