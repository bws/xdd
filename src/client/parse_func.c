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
#include "xni.h"
#include "parse.h"
#include <ctype.h>

#ifdef HAVE_NUMA_H
#include <numa.h>
#endif

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

extern	xdd_func_t xdd_func[];

/*-----------------------------------------------------All the function sub routines ---------------------------------------------*/
// These routines are specified in alphabetical order 
// There are two basic types of options that can be specified: global and target options.
// Global options affect all targets and the entire execution of a specific xdd run.
// A global option cannot be made specific to a single target. For example, "-verbose" is a global option.
// It is not possible to have one target report verbose information and another target not report
// verbose information. 
// A target option is one that can be made specific to a single target or global for all targets.
// In general, options are global to all targets unless the "target #" is specified with
// the option in which case processing of that option is specific to that target. 
// For example, the command line option "-op write" will cause the write operation to 
// be set for all targets. However, the command line option "-op target 3 write"
// will cause only target number 3 (relative to target 0) to have the write operation set.
// 
// Now, because of the order that command line options can be specified and the way
// that the Target Data are allocated in memory, there is a multi-phase
// approach to applying command line options to each target. 
// Phase 1 is the initial pass through all the command line options in order to determine how many targets there will be
//     and to allocate one Target Data struct for each of those targets during this phase.
// Phase 2 is when all the command line options and their arguments are actually processed and the associated values are
//     put into the proper Target Data structs. This is because some options may be for a specific target whereas other options will
//     be for all the targets. Since Phase 1 allocated all the Target Data Struct's then the options for any target can be set without
//     worrying about whether or not a Target Data Struct has been allocated.
//
// Precedence: The target-specific options have precedence over the global options which have precedence over the default options. 
// Man I hope this works....
// 
int32_t
xdd_parse_arg_count_check(int32_t args, int32_t argc, char *option) {

	if (argc <= args+1) {
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments specified for option '%s'\n",xgp->progname, option);
		return(0);
	}
	return(1);

} // End of xdd_parse_arg_count_check()
/*----------------------------------------------------------------------------*/
int
xddfunc_blocksize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args,i; 
    int target_number;
	int32_t block_size;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	block_size = atoi(argv[args+1]);
	if (block_size <= 0) {
		fprintf(xgp->errout,"%s: blocksize of %d is not valid. blocksize must be a number greater than 0\n",
			xgp->progname,block_size);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
	
		tdp->td_block_size = block_size;
		if (tdp->td_block_size <= 0) {
			fprintf(xgp->errout,"%s: blocksize of %d is not valid. blocksize must be a number greater than 0\n",xgp->progname,tdp->td_block_size);
			return(0);
		}
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) { 
				tdp->td_block_size = block_size;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
} // End of xddfunc_blocksize()
/*----------------------------------------------------------------------------*/
// Specify the number of Bytes to transfer per pass
// Arguments: -bytes [target #] #
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the tdp->td_numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_bytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int args, i; 
	int target_number;
	target_data_t *tdp;
	int64_t bytes;


	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	bytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL)
			return(-1);

		tdp->td_bytes = bytes;
		tdp->td_numreqs = 0;
		return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_bytes = bytes;
				tdp->td_numreqs = 0;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
} // End of xddfunc_bytes()
/*----------------------------------------------------------------------------*/
int
xddfunc_combinedout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	
	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No file name specified for Combined output\n", xgp->progname);
		xgp->combined_output_filename = "";
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		xgp->combined_output_filename = argv[1];
		xgp->combined_output = fopen(xgp->combined_output_filename,"a");
		if (xgp->combined_output == NULL) {
			fprintf(stderr,"%s: Error: Cannot open Combined output file %s\n", xgp->progname,argv[1]);
			xgp->combined_output_filename = "";
		} else xgp->global_options |= GO_COMBINED;
	}
	return(2);
} // End of xddfunc_combinedout()
/*----------------------------------------------------------------------------*/
// Set the congestion control algorithm.
int xddfunc_congestion(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int args; 
	int target_number;
	const char *congestion;

	args = xdd_parse_target_number(planp, argc, &argv[0],
								   flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	/* Get the congestion control algorithm name */
	congestion = argv[args + 1];
	
	/* Set the congestion name for the relevant targets */
	if (target_number >= 0) {
		/* Set this option value for a specific target */
		target_data_t *tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL)
			return(-1);
		tdp->xni_tcp_congestion = congestion;
		return(args+2);
	} else {
        /* Put this option into all Targets */ 
		if (flags & XDD_PARSE_PHASE2) {
			target_data_t *tdp = planp->target_datap[0];
			int i = 0;
			while (tdp) {
				tdp->xni_tcp_congestion = congestion;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
} // End of xddfunc_congestion()
/*----------------------------------------------------------------------------*/
// Set the magic cookie for network transfers
int xddfunc_cookie(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int target_number = -1;
	int args = xdd_parse_target_number(planp, argc, &argv[0],
								   flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	// Get the hex string representation of the cookie
	const char *cookie_str = argv[args + 1];
	const target_data_t tmptdp_;   // hack to later find
								   // sizeof(target_data_t.td_magic_cookie)

	// two hex digits represent one byte
	if (strlen(cookie_str) != sizeof(tmptdp_.td_magic_cookie)*2) {
		fprintf(xgp->errout,"%s: ERROR: invalid '-cookie' %s\n", xgp->progname, cookie_str);
		return(-1);
	}

	// Convert the hex string to its binary representation
	unsigned char magic_cookie[sizeof(tmptdp_.td_magic_cookie)] = { 0 };
	for (int i = 0; i < 32; i += 2) {
		unsigned int bite = 0;  // just in case `byte' is a typedef

		if (!isxdigit(cookie_str[i]) || !isxdigit(cookie_str[i+1]) || sscanf(cookie_str+i, "%2x", &bite) != 1) {
			fprintf(xgp->errout,"%s: ERROR: invalid '-cookie' %s\n", xgp->progname, cookie_str);
			return(-1);
		}

		magic_cookie[i>>1] = (unsigned char)bite;
	}
	
	// Set the magic cookie for the relevant targets
	if (target_number >= 0) {
		/* Set this option value for a specific target */
		target_data_t *tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL)
			return(-1);
		memcpy(tdp->td_magic_cookie, magic_cookie, sizeof(tdp->td_magic_cookie));
		return(args+2);
	} else {
        /* Put this option into all Targets */ 
		if (flags & XDD_PARSE_PHASE2) {
			target_data_t *tdp = planp->target_datap[0];
			int i = 0;
			while (tdp) {
				memcpy(tdp->td_magic_cookie, magic_cookie, sizeof(tdp->td_magic_cookie));
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
} // End of xddfunc_cookie()
/*----------------------------------------------------------------------------*/
// Create new target files for each pass.
int
xddfunc_createnewfiles(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_CREATE_NEW_FILES;

        return(args+1);
    } else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) { 
				tdp->td_target_options |= TO_CREATE_NEW_FILES;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(1);
	}
} // End of xddfunc_createnewfiles()
/*----------------------------------------------------------------------------*/
int
xddfunc_csvout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) {
		if (argc <= 1) {
			fprintf(stderr,"%s: Error: No file name specified for CSV output\n", xgp->progname);
			xgp->csvoutput_filename = "";
			return(-1);
		}
		xgp->csvoutput_filename = argv[1];
		xgp->csvoutput = fopen(xgp->csvoutput_filename,"a");
		if (xgp->csvoutput == NULL) {
			fprintf(stderr,"%s: Error: Cannot open Comma Separated Values output file %s\n", xgp->progname,argv[1]);
			xgp->csvoutput_filename = "";
		} else xgp->global_options |= GO_CSV;
	}
    return(2);
} // End of xddfunc_csvout()
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -datapattern [target #] # <option> 
// 
int
xddfunc_datapattern(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int           		args;
	size_t              i;
	int           		target_number;
	target_data_t        		*tdp;
	char          		*pattern_type; // The pattern type of ascii, hex, random, ...etc
	unsigned char 		*pattern; // The ACSII representation of the specified data pattern
	unsigned char 		*pattern_value; // The ACSII representation of the specified data pattern
	uint64_t      		pattern_binary = 0; // The 64-bit value shifted all the way to the left
	size_t        		pattern_length; // The length of the pattern string from the command line
	unsigned char 		*tmpp;
	int           		retval;
  

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	pattern_type = (char *)argv[args+1];
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

        retval = args+2;
    } else {
        tdp = (target_data_t *)NULL;
        args = 0;
        retval = 2;
    }
	if (strcmp(pattern_type, "random") == 0) {  /* make it random data  */
		if (tdp)  /* set option for the specific target */
            tdp->td_dpp->data_pattern_options |= DP_RANDOM_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) { 
					tdp->td_dpp->data_pattern_options |= DP_RANDOM_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
    } else if (strcmp(pattern_type, "randbytarget") == 0) {  /* make it random data seeded by target number  */
		if (tdp)  /* set option for the specific target */
            tdp->td_dpp->data_pattern_options |= DP_RANDOM_BY_TARGET_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) { 
					tdp->td_dpp->data_pattern_options |= DP_RANDOM_BY_TARGET_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "ascii") == 0) {
		retval++;
		if (argc <= args+2) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-datapattern'\n",xgp->progname);
			return(0);
		}
		pattern = (unsigned char *)argv[args+2];
		pattern_length = strlen((char *)pattern);
		if (tdp) { /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_ASCII_PATTERN;
			tdp->td_dpp->data_pattern = (unsigned char *)argv[args+2];
			tdp->td_dpp->data_pattern_length = strlen((char *)tdp->td_dpp->data_pattern);
		}
		else  {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) { 
					tdp->td_dpp->data_pattern_options |= DP_ASCII_PATTERN;
					tdp->td_dpp->data_pattern = pattern;
					tdp->td_dpp->data_pattern_length = pattern_length;
					i++;
					tdp = planp->target_datap[i];
				}
			}			
		}
	} else if (strcmp(pattern_type, "hex") == 0) {
		retval++;
		if (argc <= args+2) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-datapattern'\n",xgp->progname);
			return(0);
		}
		pattern = (unsigned char *)argv[args+2];
		pattern_length = strlen((char *)pattern);
		pattern_value = 0;
		if (pattern_length <= 0) {
			fprintf(xgp->errout, "%s: WARNING: 0-length hex data pattern specified - using 0x00 instead.\n",xgp->progname);
			pattern = (unsigned char *)"\0";
		}
		else {
		    pattern_value = malloc(((pattern_length+1)/2));
		    if (pattern_value == 0) {
			fprintf(xgp->errout, "%s: WARNING: cannot allocate %d bytes for hex data pattern - defaulting to no pattern\n",
				xgp->progname,
				(int)((pattern_length+1)/2));
		    }
		    memset(pattern_value,0,((pattern_length+1)/2));
		}
		// At this point "pattern" points to the ascii string of hex characters (0-9,a-f)
		// and pattern_length will be set to the length of that string in bytes
		// Now we call  xdd_atohex() to convert the ascii sting to a hex bunch of digits
		// xdd_atohex will return the pattern length as the number of nibbles.
		// If there are an odd number of nibbles specified, then the high-order bits will be filled in as 0.
		// The "patter_length" will be reset to the number of "bytes" for this pattern whieh is usually
		// the number of nibbles divided by 2
		if (pattern_value) {
			pattern_length = xdd_atohex((unsigned char *)pattern_value, (char *)pattern);
			if (pattern_length > 1) 
				pattern_length = (pattern_length+1)/2; // make sure this is rounded up
			else 	pattern_length = 1;
		} else pattern_length = 0;

		if (tdp) { /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_HEX_PATTERN;
			tdp->td_dpp->data_pattern = pattern_value; // The actual 64-bit value left-justtified
			tdp->td_dpp->data_pattern_length = pattern_length; // length in nibbles 
		}
		else  {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) { 
					tdp->td_dpp->data_pattern_options |= DP_HEX_PATTERN;
					tdp->td_dpp->data_pattern = pattern_value; // The actual 64-bit value right-justtified
					tdp->td_dpp->data_pattern_length = pattern_length; // length in bytes
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "prefix") == 0) {
		retval++;
		if (argc <= args+2) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-datapattern'\n",xgp->progname);
			return(0);
		}
		pattern = (unsigned char *)argv[args+2];
		pattern_length = strlen((char *)pattern); 
		
		// It is important to remember that the prefix pattern length is in the number of 4-bit nibbles not 8-bit bytes.
		if (pattern_length <= 0) {
			fprintf(xgp->errout, "%s: WARNING: 0-length data pattern prefix specified - using 0x00 instead.\n",xgp->progname);
			pattern = (unsigned char *)"\0";
			pattern_length = 2;
		}
		if (pattern_length > 16) { // 16 nibbles or 8 bytes
			fprintf(xgp->errout, "%s: WARNING: Length of data pattern is too long <%d> - truncating to 8 bytes.\n",xgp->progname, (int)pattern_length);
			pattern[16] = '\0';
		}
		pattern_value = malloc(((pattern_length+1)/2));
		if (pattern_value == 0) {
			fprintf(xgp->errout, "%s: WARNING: cannot allocate %d bytes for prefix data pattern - defaulting to no prefix\n",
				xgp->progname,
				(int)((pattern_length+1)/2));
		}
		memset(pattern_value,0,((pattern_length+1)/2));
		
		// At this point "pattern" points to the ascii string of hex characters (0-9,a-f)
		// and pattern_length will be set to the length of that string in bytes
		// Now we call  xdd_atohex() to convert the ascii sting to a bunch of hex digits
		// xdd_atohex will return the pattern length as the number of 4-bit nibbles
		if (pattern_value) {
			pattern_length = xdd_atohex((unsigned char *)pattern_value, (char *)pattern);
			if (pattern_length > 1) 
				pattern_length = (pattern_length+1)/2; // make sure this is rounded up
			else 	pattern_length = 1;
		} else pattern_length = 0;
		if (pattern_value) { // This section will put the prfix pattern into the high-order N bits of a 64-bit unsigned integer
			tmpp = (unsigned char *)pattern_value;
			pattern_binary = 0;
			for (i=0; i<pattern_length; i++) {
				pattern_binary <<= 8;
				pattern_binary |= *tmpp;
				tmpp++;
			}
			pattern_binary <<= ((sizeof(uint64_t)*8)-(pattern_length*2));
		}
		if (tdp) { /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_PATTERN_PREFIX;
			tdp->td_dpp->data_pattern_prefix = pattern;
			tdp->td_dpp->data_pattern_prefix_value = pattern_value; // Pointer to the  N-bit value in BIG endian (left justified)
			tdp->td_dpp->data_pattern_prefix_binary = pattern_binary; // The actual 64-bit binary value left-justtified
			tdp->td_dpp->data_pattern_prefix_length = pattern_length; // Length in nibbles
		} else  {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) { 
					tdp->td_dpp->data_pattern_options |= DP_PATTERN_PREFIX;
					tdp->td_dpp->data_pattern_prefix = pattern;
					tdp->td_dpp->data_pattern_prefix_value = pattern_value; // Pointer to the  N-bit value in BIG endian (left justified)
					tdp->td_dpp->data_pattern_prefix_binary = pattern_binary; // The actual 64-bit binary value left-justtified
					tdp->td_dpp->data_pattern_prefix_length = pattern_length; // Length in nibbles
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "file") == 0) {
		retval++;
		if (argc <= args+2) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-datapattern'\n",xgp->progname);
			return(0);
		}
		if (tdp) {/* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_FILE_PATTERN;
			tdp->td_dpp->data_pattern_filename = (char *)argv[args+2];
		} else {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_FILE_PATTERN;
					tdp->td_dpp->data_pattern_filename = (char *)argv[args+2];
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "sequenced") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_SEQUENCED_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_SEQUENCED_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "inverse") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_INVERSE_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_INVERSE_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strncmp(pattern_type, "replicate", 9) == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_REPLICATE_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_REPLICATE_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "lfpat") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_LFPAT_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_LFPAT_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "ltpat") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_LTPAT_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_LTPAT_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "cjtpat") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_CJTPAT_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_CJTPAT_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "crpat") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_CRPAT_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_CRPAT_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "cspat") == 0) {
		if (tdp) /* set option for specific target */
			tdp->td_dpp->data_pattern_options |= DP_CSPAT_PATTERN;
		else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_CSPAT_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	} else {
		if (tdp) { /* set option for a specific target */ 
			tdp->td_dpp->data_pattern_options |= DP_SINGLECHAR_PATTERN;
			tdp->td_dpp->data_pattern = (unsigned char *)pattern_type;
		} else {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_dpp->data_pattern_options |= DP_SINGLECHAR_PATTERN;
					tdp->td_dpp->data_pattern = (unsigned char *)pattern_type;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
	}
    return(retval);
} // End of xddfunc_datapattern()

/*----------------------------------------------------------------------------*/
// Set the DEBUG global option
int
xddfunc_debug(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (argv[1] == NULL) {
		fprintf(stderr,"xddfunc_debug: No option for -debug specified: defaulting to -debug ALL\n");
		xgp->global_options |= GO_DEBUG_ALL;
		return(1);
	}
    if ((strcmp(argv[1], "io") == 0) ||
		(strcmp(argv[1], "IO") == 0)) {
			xgp->global_options |= GO_DEBUG_IO;
	} else if ((strcmp(argv[1], "E2E") == 0) ||
			   (strcmp(argv[1], "e2e") == 0) ||
			   (strcmp(argv[1], "END2END") == 0) ||
			   (strcmp(argv[1], "end2end") == 0)) {
			xgp->global_options |= GO_DEBUG_E2E;
	} else if ((strcmp(argv[1], "LOCKSTEP") == 0) ||
			   (strcmp(argv[1], "lockstep") == 0) ||
			   (strcmp(argv[1], "LS") == 0) ||
			   (strcmp(argv[1], "ls") == 0)) {
			xgp->global_options |= GO_DEBUG_LOCKSTEP;
	} else if ((strcmp(argv[1], "OPEN") == 0) ||
			   (strcmp(argv[1], "open") == 0)) {
			xgp->global_options |= GO_DEBUG_OPEN;
	} else if ((strcmp(argv[1], "TASK") == 0) ||
			   (strcmp(argv[1], "task") == 0)) {
			xgp->global_options |= GO_DEBUG_TASK;
	} else if ((strcmp(argv[1], "TOT") == 0) ||
			   (strcmp(argv[1], "tot") == 0)) {
			xgp->global_options |= GO_DEBUG_TOT;
	} else if ((strcmp(argv[1], "TS") == 0) ||
			   (strcmp(argv[1], "ts") == 0)) {
			xgp->global_options |= GO_DEBUG_TS;
	} else if ((strcmp(argv[1], "THROTTLE") == 0) ||
			   (strcmp(argv[1], "THROT") == 0) ||
			   (strcmp(argv[1], "throt") == 0) ||
			   (strcmp(argv[1], "throttle") == 0)) {
			xgp->global_options |= GO_DEBUG_THROTTLE;
	} else if ((strcmp(argv[1], "TIMELIMIT") == 0) ||
			   (strcmp(argv[1], "timelimit") == 0) ||
			   (strcmp(argv[1], "TL") == 0) ||
			   (strcmp(argv[1], "tl") == 0)) {
			xgp->global_options |= GO_DEBUG_TIME_LIMIT;
	} else if ((strcmp(argv[1], "USER1") == 0) ||
			   (strcmp(argv[1], "user1") == 0)) {
			xgp->global_options |= GO_DEBUG_USER1;
	} else if ((strcmp(argv[1], "ALL") == 0) ||
			   (strcmp(argv[1], "all") == 0) ||
			   (strcmp(argv[1], "a") == 0)) {
			xgp->global_options |= GO_DEBUG_ALL;
	} else {
		fprintf(stderr,"xddfunc_debug: Unknown option for -debug: '%s' - defaulting to -debug ALL\n",argv[1]);
		xgp->global_options |= GO_DEBUG_ALL;
		if (*argv[1] == '-') // Perhaps this is another option so return 1 and we'll assume that the debug option was not specified
			return(1);
	}
	return(2);
} // End of xddfunc_debug()

/*----------------------------------------------------------------------------*/
// Delete the target file when complete
int
xddfunc_deletefile(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_DELETEFILE;
        return(args+1);
    } else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_DELETEFILE;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(1);
	}
} // End of xddfunc_deletefile()
/*----------------------------------------------------------------------------*/
// Arguments: -devicefile [target #] - OBSOLETE
int
xddfunc_devicefile(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_DEVICEFILE;
        return(args+1);
    } else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_DEVICEFILE;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(1);
	}
} 
/*----------------------------------------------------------------------------*/
// Specify the use of direct I/O for a single target or for all targets
// Arguments: -dio [target #]
int
xddfunc_dio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_DIO;
        return(args+1);
    } else {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_DIO;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_dryrun(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_DRYRUN;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the end-to-end options for either the source or the destination
// Arguments: -endtoend [target #] 
//				destination <hostname> 
//				port <number>
//				issource
//				isdestination
// 
int
xddfunc_endtoend(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{	
    int i,j;
    int args, args_index; 
    int target_number;
    target_data_t *tdp;
    char *hostname, *base_port, *port_count, *numa_node;
    char *cp, *sp;
    char *source_path;
    time_t source_mtime;
    int	base_port_number;
    int	number_of_ports;
    int len;
    char cmdline[256];


    if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for EndToEnd option\n", xgp->progname);
		return(-1);
    }
	planp->plan_options |= PLAN_ENDTOEND;
    args_index = 1;
    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	// Make sure that the Target Data Structs exists and that the E2E structures have been allocated
	if (target_number >= 0) {
    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	   	if (tdp == NULL) return(-1);
		if (NULL == tdp->td_e2ep) { // If there is no e2e struct then allocate one.
	    	tdp->td_e2ep = xdd_get_e2ep();
			if (NULL == tdp->td_e2ep) {
	    		fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for Target Data Struct END TO END Data Structure for target %d\n",
		    		xgp->progname, (int)sizeof(xint_data_pattern_t), target_number);
	    		return(-1);
			}
		}
	} else {
		tdp = planp->target_datap[0];
		i = 0;
		while (tdp) {
			if (NULL == tdp->td_e2ep) { // If there is no e2e struct then allocate one.
	    		tdp->td_e2ep = xdd_get_e2ep();
				if (NULL == tdp->td_e2ep) {
	    			fprintf(xgp->errout,"%s: ERROR: Cannot allocate %d bytes of memory for Target Data Struct END TO END Data Structure for target %d\n",
		    			xgp->progname, (int)sizeof(xint_data_pattern_t), target_number);
	    			return(-1);
				}
			}
			i++;
			tdp = planp->target_datap[i];
		}
	}
    
	// Now check to see how many args were specified
    if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);
    
    if (target_number >= 0) { /* Set this option value for a specific target */
		args_index += args; 
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
	    	fprintf(stderr,"%s: Invalid Target Number %d specified for End-to-End option %s\n",
		    	xgp->progname, target_number, argv[args_index]);
	    	return(0);
		}
    }

    if ((strcmp(argv[args_index], "destination") == 0) ||
		(strcmp(argv[args_index], "dest") == 0) ||
		(strcmp(argv[args_index], "host") == 0)) {
		/* Record the name of the "destination" host name for the -e2e option */
		// Address can be in one of several forms based on:
		//     IP:base_port,#ports,numa_node 
		// where IP is the IP address or hostname
		// "base_port" is the base port number to use
		// and "#ports" is the number of ports to use starting
		//    with the previously specified "base_port"
		// These are hierarchical and only the IP address is required.
		// Hence if just IP:base_port is specified then it is assumed
		// to use only that port for that address.
		// If only the IP address is specified then the base port
		// specified by the "-e2e port #" option is used as the base
		// port number and the number of ports is derived from the queue depth.
		// 
		// Incr args_index to point to the "address" argument
		args_index++;
		strcpy(cmdline,argv[args_index]);
		hostname = base_port = port_count = 0;
	
		cp = cmdline;
		sp = cp;
		hostname = sp;
		len = 0;
		while(cp && *cp) {     
	    	if (*cp == ':') {
				// The hostname/address is everything up to this point
				hostname = sp;
				*cp = '\0'; // Null terminate
				cp++;
				break;
	    	}
	    	if (isspace(*cp) && (len == 0))  { // skip preceeding white space
				cp++;
				sp = cp;
				continue;
	    	}
	    	if (isspace(*cp)) { // embedded white space marks the end of just the hostname 
				// The hostname/address is everything up to this point
				hostname = sp;
				*cp = '\0'; // Null terminate
				cp = 0; // Indicate that there is nothing after this
				break;
	    	}
	    	cp++;
	    	len++;
		} // End of WHILE loop that parses the "address" or "hostname"
	
		// Lets get the base port number if specified
		sp = cp;
		base_port = sp;
		len = 0;
		while(cp && *cp) {     
	    	if (*cp == ',') {
				// The base_port number is everything up to this point
				base_port = sp;
				*cp = '\0'; // Null terminate
				cp++;
				break;
	    	}
	    	if (isspace(*cp) && (len == 0))  { // skip preceeding white space
				cp++;
				sp = cp;
				continue;
	    	}
	    	if (isspace(*cp)) { // embedded white space marks the end of just the hostname 
				// The hostname/address is everything up to this point
				base_port = sp;
				*cp = '\0'; // Null terminate
				cp = 0; // Indicate that there is nothing after this
				break;
	    	}
	    	cp++;
	    	len++;
		}
	
		if (NULL == base_port || *base_port == '\0')
	    	base_port = 0; // Indicates that a base port was not specified
	
		// Lets get the number of ports if specified
		sp = cp;
		port_count = sp;
		len = 0;
		while(cp && *cp) {
	    	// Skip preceding whitespace
	    	if (isspace(*cp) && (0 == len)) {
				cp++;
				sp = cp;
				continue;
	    	}
	    	if (',' == *cp) {
				port_count=sp;
				*cp = '\0';
				cp++;
				break;
	    	}
	    	if (isspace(*cp)) { // embedded white space marks the end of just the hostname 
				// The hostname/address is everything up to this point
				port_count = sp;
				*cp = '\0'; // Null terminate
				cp = 0; // Indicate that there is nothing after this
				break;
	    	}
	    	cp++;
	    	len++;
		}

		if (NULL == port_count || *port_count == '\0')
	    	port_count = 0; // Indicates that port_count was not specified
	
		// Parse NUMA node if specified
		sp = cp;
		numa_node = sp;
		len=0;
		while (cp && *cp) {
	    	if (isspace(*cp)) {
		    	numa_node = sp;
		    	*cp = '\0';
		    	cp = 0;
		    	break;
	    	}
	    	cp++;
	    	len++;
		}
		if (NULL == numa_node || '\0' == *numa_node)
		    numa_node = 0;
	
		///////////// Done parsing the address:base_port,port_count,numa_node
		

		// Now we need to put the address and base_port and number of ports into the Target Data Struct for this Target or all Targets
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);

	    	tdp->td_target_options |= TO_ENDTOEND;
	    	strcpy(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].hostname, hostname); 
	    	tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].base_port = DEFAULT_E2E_PORT; 
			tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = 0;
            // Set a default NUMA node value if possible
#if defined(HAVE_CPU_SET_T)
			CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
				sched_getaffinity(getpid(), 
				sizeof(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set),
				&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
#endif

	    	if (base_port) { // Set the requested Port Number and possible Port Count
				tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].base_port = atoi(base_port); 
				if (port_count) {
		    		tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = atoi(port_count); 
				} else {
		    		tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = 0;
				}

#if defined(HAVE_CPU_SET_T) && defined(HAVE_NUMA_NODE_TO_CPUS) && defined(HAVE_NUMA_ALLOCATE_CPUMASK) 
				if (numa_node && -1 != numa_available()) {
		    		int i;
		    		struct bitmask* numa_mask = numa_allocate_cpumask();
		    		int numa_node_no = atoi(numa_node);
		    		CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
		    		numa_node_to_cpus(numa_node_no, numa_mask);
		    		for (i = 0; i <= CPU_SETSIZE; i++) {
							if (numa_bitmask_isbitset(numa_mask, i))
			    				CPU_SET(i, &tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
		    		}
		    		numa_free_cpumask(numa_mask);
				} else {
		    		CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
		    			sched_getaffinity(getpid(),
						sizeof(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set),
						&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
				}
#else
				if (numa_node) {
		    		fprintf(stderr,"%s: NUMA node end-to-end option ignored: %s\n",xgp->progname, numa_node);
				}
#endif
	    	} 
	    	tdp->td_e2ep->e2e_address_table_port_count += tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count;
	    	tdp->td_e2ep->e2e_address_table_next_entry++;
	    	tdp->td_e2ep->e2e_address_table_host_count++;
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
		    		tdp->td_target_options |= TO_ENDTOEND;
		    		strcpy(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].hostname, hostname); 
					tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].base_port = DEFAULT_E2E_PORT; 
                    tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = 0;
                    // Set a default NUMA node value if possible
#if defined(HAVE_CPU_SET_T)
		    		CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
						sched_getaffinity(getpid(),
						sizeof(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set),
						&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
#endif

		    		if (base_port) { // Set the requested Port Number and possible Port Count
						tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].base_port = atoi(base_port); 
						if (port_count) {
							tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = atoi(port_count); 
						} else {
			    			tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count = 0;
						}
#if defined(HAVE_CPU_SET_T) && defined(HAVE_NUMA_NODE_TO_CPUS) && defined(HAVE_NUMA_ALLOCATE_CPUMASK)
						if (numa_node && -1 != numa_available()) {
			    			int i;
			    			struct bitmask* numa_mask = numa_allocate_cpumask();
			    			int numa_node_no = atoi(numa_node);
			    			CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
								numa_node_to_cpus(numa_node_no, numa_mask);
			    			for (i = 0; i <= CPU_SETSIZE; i++) {
								if (numa_bitmask_isbitset(numa_mask, i))
				    				CPU_SET(i, &tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
			    			}
			    			numa_free_cpumask(numa_mask);
						} else {
			    			CPU_ZERO(&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
								sched_getaffinity(getpid(),
								sizeof(tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set),
								&tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].cpu_set);
						}
#else
						if (numa_node) {
			    			fprintf(stderr,"%s: NUMA node end-to-end option ignored: %s\n",xgp->progname, numa_node);
						}
#endif
		    		} // End of IF stmnt that sets the Port/NPorts
				    tdp->td_e2ep->e2e_address_table_port_count += tdp->td_e2ep->e2e_address_table[tdp->td_e2ep->e2e_address_table_next_entry].port_count;
		    		tdp->td_e2ep->e2e_address_table_host_count++;
		    		tdp->td_e2ep->e2e_address_table_next_entry++;
		    		i++;
		    		tdp = planp->target_datap[i];
				}
	    	}
		}
		return(args_index+1);
    } else if ((strcmp(argv[args_index], "issource") == 0) ||
	       (strcmp(argv[args_index], "issrc") == 0)) {
		// Set the target option flags that indicates this is the 
		// "source" host name for the -e2e option
		args_index++;
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
	    	tdp->td_target_options |= TO_E2E_SOURCE;
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
		    		tdp->td_target_options |= TO_E2E_SOURCE;
		    		i++;
		    		tdp = planp->target_datap[i];
				}
	    	}
		}
		return(args_index);
    } else if ((strcmp(argv[args_index], "isdestination") == 0) ||
	       (strcmp(argv[args_index], "isdest") == 0)) { 
		// Set the target option flags that indicates this is the 
		// "destination" host name for the -e2e option
		args_index++;
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
	    	tdp->td_target_options |= TO_E2E_DESTINATION;
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
		    		tdp->td_target_options |= TO_E2E_DESTINATION;
		    		i++;
		    		tdp = planp->target_datap[i];
				}
	    	}
		}
		return(args_index);
    } else if ((strcmp(argv[args_index], "port") == 0) ||  /* set the base port number to use for -e2e */
	       (strcmp(argv[args_index], "baseport") == 0)) { 
		args_index++;
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
		
	    	tdp->td_target_options |= TO_ENDTOEND;
	    	base_port_number = atoi(argv[args_index]);
	    	for (j = 0; j < E2E_ADDRESS_TABLE_ENTRIES; j++) 
				tdp->td_e2ep->e2e_address_table[j].base_port = base_port_number;
	    	// Set the base_port number in each of the currently specified e2e_address_table entries
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				base_port_number = atoi(argv[args_index]);
				while (tdp) {
		    		tdp->td_target_options |= TO_ENDTOEND;
		    		// Set the base_port number in each of the currently specified e2e_address_table entries
		    		for (j = 0; j < E2E_ADDRESS_TABLE_ENTRIES; j++) 
						tdp->td_e2ep->e2e_address_table[j].base_port = base_port_number;
	
		    		i++;
		    		tdp = planp->target_datap[i];
				} // End of while(tdp) loop
	    	}
		}
		return(args_index+1);
    } else if (strcmp(argv[args_index], "portcount") == 0) {  /* set the port count in each address table entry  */
		args_index++;
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
	    	tdp->td_target_options |= TO_ENDTOEND;
	    	number_of_ports = atoi(argv[args_index]);
	    	for (j = 0; j < E2E_ADDRESS_TABLE_ENTRIES; j++) 
				tdp->td_e2ep->e2e_address_table[j].port_count = number_of_ports;
	    	// Set the base_port number in each of the currently specified e2e_address_table entries
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				number_of_ports = atoi(argv[args_index]);
				while (tdp) {
		    		tdp->td_target_options |= TO_ENDTOEND;
		    		// Set the base_port number in each of the currently specified e2e_address_table entries
		    		for (j = 0; j < E2E_ADDRESS_TABLE_ENTRIES; j++) 
						tdp->td_e2ep->e2e_address_table[j].port_count = number_of_ports;
		    		i++;
		    		tdp = planp->target_datap[i];
				} // End of while(tdp) loop
	    	}
		}
		return(args_index+1);
    } else if ((strcmp(argv[args_index], "sourcemonitor") == 0) ||
	       (strcmp(argv[args_index], "srcmon") == 0)) { 
		// Monitor the Source Side in target_pass_loop()
		args_index++;
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
	    	tdp->td_target_options |= TO_E2E_SOURCE_MONITOR;
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
		    	tdp->td_target_options |= TO_E2E_SOURCE_MONITOR;
		    	i++;
		    	tdp = planp->target_datap[i];
			}
	    	}
		}
		return(args_index);
    } else if ((strcmp(argv[args_index], "sourcepath") == 0) ||  /* complete source file path for restart option */
	       (strcmp(argv[args_index], "srcpath") == 0)) { 
		if (target_number >= 0) {
	    	tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    	if (tdp == NULL) return(-1);
	    	tdp->td_target_options |= TO_ENDTOEND;
	    	source_path  = argv[args_index+1];
	    	source_mtime = atoll(argv[args_index+2]);
	    	tdp->td_e2ep->e2e_src_file_path  = source_path;
	    	tdp->td_e2ep->e2e_src_file_mtime = source_mtime;
		} else {  /* set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				source_path  = argv[args_index+1];
		    	source_mtime = atoll(argv[args_index+2]);
		    	while (tdp) {
					tdp->td_target_options |= TO_ENDTOEND;
					tdp->td_e2ep->e2e_src_file_path  = source_path;
					tdp->td_e2ep->e2e_src_file_mtime = source_mtime;
					i++;
					tdp = planp->target_datap[i];
		    	}
	    	}
		}
		return(args_index+3);
    } else {
		fprintf(stderr,"%s: Invalid End-to-End option %s\n",xgp->progname, argv[args_index]);
		return(0);
    }/* End of the -endtoend (e2e or war) sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_errout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{

	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No file name specified for Error output\n", xgp->progname);
		xgp->errout_filename = "";
		return(-1);
	}

	if (flags & XDD_PARSE_PHASE2) {
			xgp->errout_filename = argv[1];
			xgp->errout = fopen(xgp->errout_filename,"w");
			if (xgp->errout == NULL) {
				fprintf(stderr,"%s: Error: Cannot open error output file %s\n", xgp->progname,argv[1]);
				xgp->errout = stderr;
				xgp->errout_filename = "stderr";
			}
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_extended_stats(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_EXTENDED_STATS;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Perform a flush (sync) operation every so many write operations
// Arguments: -flushwrite [target #] #
// 
int
xddfunc_flushwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int64_t flushwrite;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	flushwrite = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_flushwrite = flushwrite;

        return(args+2);
	} else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_flushwrite = flushwrite;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(2);
	}
} // End of flushwrite()
/*----------------------------------------------------------------------------*/
int
xddfunc_fullhelp(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    xdd_usage(1);
    return(-1);
}
/*----------------------------------------------------------------------------*/
/*  The -heartbeat option accepts one argument that is any of the following:
 *      - A positive integer that indicates the number of seconds between beats
 *      - The word "operations" or "ops" to display the current number of operations complete
 *      - The word "bytes" to display the current number of bytes transfered 
 *      - The word "kbytes" or "kb" to display the current number of Kilo Bytes transfered 
 *      - The word "mbytes" or "mb" to display the current number of Mega Bytes transfered 
 *      - The word "gbytes" or "gb" to display the current number of Giga Bytes transfered 
 *      - The word "percent" or "pct" to display the %complete along with 
 *      - The word "bandwidth" or "bw" to display the aggregate bandiwdth
 *      - The word "iops" to display the aggregate I/O operations per second
 *      - The word "et" or "etc" or "eta" to display the estimated time to completion
 *      - The word "lf" or "cr" or to put a LineFeed (aka Carriage Return) at the end of the 
 *            line after all targets have been displayed
 *      - The word "tod" to display the "time of day" on each heartbeat line
 *      - The word "sec" to display the "number of seconds" into the run on each heartbeat line
 *      - The word "host" to display the "host name" on each heartbeat line
 *      - The word "output" or "out" will send all the heartbeat output to a specific file
 * Specifying -heartbeat multiple times will add these to the heartbeat output string FOR EACH TARGET
 */
int
xddfunc_heartbeat(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 		args, i; // Number of args and a counter
    int 		target_number; // The specific target number to update
    target_data_t 		*tdp;		// Current Target Data Struct being updated
	char		*sp;	// String pointer
	int			c1,c2;	// A single character
	char		*cp;	// A single Character pointer
	int			len;	// Length of the option string
	int			digits;	// if set to 1 then string contains only numerical digits
	int			return_value;
	heartbeat_t	hb;		// A heartbeat structure for these options
	char		*filename; // Pointer to the area that will store the file name


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (argc < 1) {
		fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-heartbeat'\n",xgp->progname);
		return(0);
	}

	// The zeroth argument in argv[] is the option name: "-heartbeat"
	// The first argument in argv[] is the heartbeat option string
	// Check to see that there is an option string
	sp = argv[1];
	len = strlen(sp);
	if (len <= 0) {
		fprintf(xgp->errout,"%s: WARNING: The option for '-heartbeat' is zero length\n",xgp->progname);
		return(1);
	}
	
	// Convert the "option" string to lower case for consistency
	digits = 0;
	cp = sp;
	for (i = 0; i < len; i++) {
		c1 = *cp;
		c2 = tolower(c1);
		*cp = c2;
		digits = isdigit(c2); // If this is ever a non-digit then digits will be set to 0
		cp++;
	}
		
	hb.hb_interval = 0;
	if (digits) { // The heartbeat option is "number of seconds"
		hb.hb_interval = atoi(sp);
		if (hb.hb_interval <= 0) { // This should not happen but let's check just to be certain
			fprintf(stderr,"%s: Error: Heartbeat value of '%d' cannot be negative\n", xgp->progname, hb.hb_interval);
			return(-1);
		}
		return(2);
	}

	return_value = -1;
	hb.hb_options = 0;
	hb.hb_filename = NULL;
	// Otherwise check to see if it is any of the recognized words
	if ((strcmp(sp, "operations") == 0) || (strcmp(sp, "ops") == 0)) { // Report OPERATIONS 
			hb.hb_options |= HB_OPS;
			return_value = 2;
	} else if ((strcmp(sp, "bytes") == 0) || (strcmp(sp, "b") == 0)) { // Report Bytes 
			hb.hb_options |= HB_BYTES;
			return_value = 2;
	} else if ((strcmp(sp, "kbytes") == 0) || (strcmp(sp, "kb") == 0)) { // Report KiloBytes 
			hb.hb_options |= HB_KBYTES;
			return_value = 2;
	} else if ((strcmp(sp, "mbytes") == 0) || (strcmp(sp, "mb") == 0)) { // Report MegaBytes 
			hb.hb_options |= HB_MBYTES;
			return_value = 2;
	} else if ((strcmp(sp, "gbytes") == 0) || (strcmp(sp, "gb") == 0)) { // Report GigaBytes 
			hb.hb_options |= HB_GBYTES;
			return_value = 2;
	} else if ((strcmp(sp, "percent") == 0) || (strcmp(sp, "pct") == 0)) { // Report Percent complete
			hb.hb_options |= HB_PERCENT; 
			return_value = 2;
	} else if ((strcmp(sp, "bandwidth") == 0) || (strcmp(sp, "bw") == 0)) { // Report Aggregate Bandwidth
			hb.hb_options |= HB_BANDWIDTH;
			return_value = 2;
	} else if ((strcmp(sp, "iops") == 0) || (strcmp(sp, "IOPS") == 0)) { // Report IOPS
			hb.hb_options |= HB_IOPS;
			return_value = 2;
	} else if ((strcmp(sp, "etc") == 0) || (strcmp(sp, "eta") == 0) || (strcmp(sp, "et") == 0)) { // Report Estimated time to completion
			hb.hb_options |= HB_ET;
			return_value = 2;
	} else if ((strcmp(sp, "lf") == 0) || (strcmp(sp, "cr") == 0) || (strcmp(sp, "nl") == 0)) { // LF/CR/NL at the end of each heartbeat
			hb.hb_options |= HB_LF;
			return_value = 2;
	} else if ((strcmp(sp, "tod") == 0) || (strcmp(sp, "time") == 0)) { // Report the "time of day" on each line
			hb.hb_options |= HB_TOD;
			return_value = 2;
	} else if ((strcmp(sp, "elapsed") == 0) || (strcmp(sp, "sec") == 0)) { // Report the number of "elapsed seconds" on each line
			hb.hb_options |= HB_ELAPSED;
			return_value = 2;
	} else if ((strcmp(sp, "target") == 0) || (strcmp(sp, "tgt") == 0)) { // Report the "Target Number" on each line
			hb.hb_options |= HB_TARGET;
			return_value = 2;
	} else if ((strcmp(sp, "hostname") == 0) || (strcmp(sp, "host") == 0)) { // Report the "host name" on each line
			hb.hb_options |= HB_HOST;
			return_value = 2;
	} else if ((strcmp(sp, "output") == 0) || (strcmp(sp, "out") == 0)) { // Change the name of the output file
			hb.hb_filename=argv[2];
			return_value = 3;
	} else if ((strcmp(sp, "ignorerestart") == 0) || (strcmp(sp, "ir") == 0) || (strcmp(sp, "ignore") == 0)) { // Ignore the restart adjustments
			hb.hb_options |= HB_IGNORE_RESTART;
			return_value = 2;
	} 
	if (return_value == -1) {
		// Not a recognizable option
		fprintf(stderr,"%s: ERROR: Unknown option specified for the heartbeat: '%s'\n", xgp->progname, sp);
		return(-1);
	}

	// At this point we have figured out what was specified. Now we just have to put it into the proper Target Data Struct.
	xgp->global_options |= GO_HEARTBEAT;
    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		if (hb.hb_options)
			tdp->td_hb.hb_options |= hb.hb_options;
		if (hb.hb_interval > 0) 
			tdp->td_hb.hb_interval = hb.hb_interval;
		if (hb.hb_filename) {
			len = strlen(hb.hb_filename);
			filename = (char *)malloc(len+16);
			if (filename == NULL) {
				fprintf(stderr,"%s: ERROR: Cannot allocate %d bytes for heartbeat output file name: '%s'\n", xgp->progname, len+16,hb.hb_filename);
				return(-1);
			}
			sprintf(filename,"%s.T%04d.csv",hb.hb_filename,tdp->td_target_number);
			tdp->td_hb.hb_filename = filename;
		}
		// Check to see if the heartbeat interval has been set yet. If not, then set it to 1.
		if (tdp->td_hb.hb_interval == 0)
			tdp->td_hb.hb_interval = 1;
        return(args+return_value);
    } else {// Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				if (hb.hb_options)
					tdp->td_hb.hb_options |= hb.hb_options;
				if (hb.hb_interval > 0) 
					tdp->td_hb.hb_interval = hb.hb_interval;
				if (hb.hb_filename) {
					len = strlen(hb.hb_filename);
					filename = (char *)malloc(len+16);
					if (filename == NULL) {
						fprintf(stderr,"%s: ERROR: Cannot allocate %d bytes for heartbeat output file name: '%s'\n", xgp->progname, len+16,hb.hb_filename);
						return(-1);
					}
					sprintf(filename,"%s.T%04d.csv",hb.hb_filename,tdp->td_target_number);
					tdp->td_hb.hb_filename = filename;
				}
				// Check to see if the heartbeat interval has been set yet. If not, then set it to 1.
				if (tdp->td_hb.hb_interval == 0)
					tdp->td_hb.hb_interval = 1;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(return_value);
	}

} // End of xddfunc_heartbeat()

/*----------------------------------------------------------------------------*/
int
xddfunc_help(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int i,j,found;


	if (argv[1] == 0) {
		fprintf(stderr,"xdd_help: no option specified to provide help with\n");
		return(1);
	}
	fprintf(stderr,"Help for the '%s' option: \n",argv[1]);
    i = 0;
	found=0;
    while(xdd_func[i].func_name) {
		if ((strcmp(xdd_func[i].func_name, argv[1]) == 0) ||
			(strcmp(xdd_func[i].func_alt, argv[1]) == 0)) { 
			found=1;
            fprintf(stderr,"Option name is %s or abbreviated as %s",
				xdd_func[i].func_name,
				xdd_func[i].func_alt);
            fprintf(stderr,"%s",xdd_func[i].help);
            j=0;
            while (xdd_func[i].ext_help[j] && (j < XDD_EXT_HELP_LINES)) {
                fprintf(stderr,"%s",xdd_func[i].ext_help[j]);
                j++;
            }
            fprintf(stderr,"\n");
        }
        i++;
    }
	if (found == 0) {
		fprintf(stderr,"xdd_help: cannot find specified option '%s' to provide help on\n", argv[1]);
	    xdd_usage(0);
	}
    return(-1);
}
/*----------------------------------------------------------------------------*/
// Specify an identification string for this run
// Arguments: -id commandline|"string"
int
xddfunc_id(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int32_t	j;

	if (argc <= 1) {
		fprintf(stderr,"%s: Error: Nothing specified for ID\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		if (xgp->id_firsttime == 1) { /* zero out the ID string if this is the first time thru */
			*xgp->id = 0;
			xgp->id_firsttime = 0;
		}

		if (strcmp(argv[1], "commandline") == 0) { /* put in the command line */
			for (j=0; j<xgp->argc; j++) {
				strcat(xgp->id,xgp->argv[j]);
				strcat(xgp->id," ");
			}
		} else { // Just put in the agrument for the -id option
			strcat(xgp->id,argv[1]);
			strcat(xgp->id," ");
		}
	}
	return(2);	
}
/*----------------------------------------------------------------------------*/
int
xddfunc_interactive(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_INTERACTIVE;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of KBytes to transfer per pass (1K=1024 bytes)
// Arguments: -kbytes [target #] #
// This will set tdp->td_bytes to the calculated value (kbytes * 1024)
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the tdp->td_numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_kbytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int64_t kbytes;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	kbytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_bytes = kbytes * 1024;
		tdp->td_numreqs = 0;
        return(args+2);
	} else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_bytes = kbytes * 1024;
					tdp->td_numreqs = 0;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
/*  -lockstep
	-ls
	-lockstepoverlapped
    -lso
* The lockstep and lockstepoverlapped options all follow the same format:  
*   -lockstep <master_target> <slave_target> <when> <howlong> <what> <howmuch> <startup> <completion>
*     0              1              2           3       4        5       6        7           8
* Where "master_target" is the target that tells the slave when to do something.
*   "slave_target" is the target that responds to requests from the master.
*   "when" specifies when the master should tell the slave to do something.
*    The word "when" should be replaced with the word: 
*     "time" 
*     "op"
*     "percent"
*     "mbytes"
*     "kbytes" 
*   "howlong" is either the number of seconds, number of operations, ...etc.
*       - the interval time in seconds (a floating point number) between task requests from the
*      master to the slave. i.e. if this number were 2.3 then the master would request
*      the slave to perform its task every 2.3 seconds until.
*       - the operation number that defines the interval on which the master will request
*      the slave to perform its task. i.e. if the operation number is set to 8 then upon
*      completion of every 8 (master) operations, the master will request the slave to perform
*      its task.
*       - The percentage of operations that must be completed by the master before requesting
*      the slave to perform a task.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*    "what" is the type of task the slave should perform each time it is requested to perform
*     a task by the master. The word "what" should be replaced by:
*     "time" 
*     "op"
*     "mbytes"
*     "kbytes" 
*   "howmuch" is either the number of seconds, number of operations, ...etc.
*       - the amount of time in seconds (a floating point number) the slave should run before
*      pausing and waiting for further requests from the master.
*       - the number of operations the slave should perform before pausing and waiting for
*      further requests from the master.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*      the slave should transfer before pausing and waiting for
*      further requests from the master.
*    "startup" is either "wait" or "run" depending on whether the slave should start running upon
*      invocation and perform a single task or if it should simply wait for the master to 
*      request it to perform its first task. 
*    "Completion" - in the event that the master finishes before the slave, then the slave will have
*      the option to complete all of its remaining operations or to just stop at this point. 
*      This should be specified as either "complete" or "stop". I suppose more sophisticated
*      behaviours could be defined but this is it for now.
*       
*/
int
xddfunc_lockstep(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 		mt;						// Master Target number
    int			st;						// Slave Target number
    double 		tmpf;
    target_data_t 		*master_target_datap;	// Pointer to the Master Target Data Struct
    target_data_t		*slave_target_datap;	// Pointer to the Slave Target Data Struct
    int 		retval;					// Return value at any give time
    int 		lsmode;					// Lockstep mode
    char 		*when;					// Indicates WHEN to do something
    char		*what;					// Indicates WHAT to do when triggered
    char		*lockstep_startup; 		// Whether or not to start or wait at beginning
    char		*lockstep_completion; 	// What to do after completed all I/O
    lockstep_t	*master_lsp;			// Pointer to the Master Lock Step Struct
    lockstep_t	*slave_lsp;				// Pointer to the Slave Lock Step Struct


    if ((strcmp(argv[0], "-lockstep") == 0) || (strcmp(argv[0], "-ls") == 0))
		lsmode = TO_LOCKSTEP;
    else lsmode = TO_LOCKSTEPOVERLAPPED;

    mt = atoi(argv[1]); /* T1 is the master target */
    st = atoi(argv[2]); /* T2 is the slave target */
    /* Sanity checks on the target numbers */
    master_target_datap = xdd_get_target_datap(planp, mt, argv[0]);

    if (master_target_datap == NULL) return(-1);
    slave_target_datap = xdd_get_target_datap(planp, st, argv[0]);
    if (slave_target_datap == NULL) return(-1);
 
    // Make sure there is a Lockstep Structure for the MASTER
    if (master_target_datap->td_lsp == 0) {
		master_target_datap->td_lsp = (lockstep_t *)malloc(sizeof(lockstep_t));
		if (master_target_datap->td_lsp == 0) {
			fprintf(stderr,"%s: Cannot allocate %d bytes of memory for Master lockstep structure\n",
			xgp->progname, (int)sizeof(lockstep_t));
			return(0);
		}
		memset(master_target_datap->td_lsp, 0, sizeof(lockstep_t ));
    } 
	master_lsp = master_target_datap->td_lsp;

    // Make sure there is a Lockstep Structure for the SLAVE
    if (slave_target_datap->td_lsp == 0) {
		slave_target_datap->td_lsp = (lockstep_t *)malloc(sizeof(lockstep_t));
		if (slave_target_datap->td_lsp == 0) {
			fprintf(stderr,"%s: Cannot allocate %d bytes of memory for Slave lockstep structure\n",
			xgp->progname, (int)sizeof(lockstep_t));
			return(0);
		}
		memset(slave_target_datap->td_lsp, 0, sizeof(lockstep_t));
    } 
	slave_lsp = slave_target_datap->td_lsp;

	if (flags & XDD_PARSE_PHASE1) {
		if (!(xgp->global_options & GO_LOCKSTEP)) {
			xgp->global_options |= GO_LOCKSTEP;
			master_lsp->ls_state |= LS_STATE_I_AM_THE_FIRST;
		} 
	
		if (master_lsp->ls_next_tdp) {
			slave_lsp->ls_next_tdp = master_lsp->ls_next_tdp;
		} else { 
			slave_lsp->ls_next_tdp = master_target_datap;
		}
		master_lsp->ls_next_tdp = slave_target_datap;
		master_target_datap->td_target_options |= lsmode;
		slave_lsp->ls_state &= ~LS_STATE_I_AM_THE_FIRST;
		slave_target_datap->td_target_options |= lsmode; 
	}


	// Lockstep sub-options
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		master_lsp->ls_interval_type = LS_INTERVAL_TIME;
		master_lsp->ls_interval_units = "SECONDS";
		master_lsp->ls_interval_value = (nclk_t)(tmpf * BILLION);
		if (master_lsp->ls_interval_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval = 5;
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		master_lsp->ls_interval_value = atoll(argv[4]);
		master_lsp->ls_interval_type = LS_INTERVAL_OP;
		master_lsp->ls_interval_units = "OPERATIONS";
		if (master_lsp->ls_interval_value <= 0) {
			fprintf(stderr,"%s: Invalid lockstep interval op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)master_lsp->ls_interval_value);
            return(0);
		}
		retval = 5;  
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		master_lsp->ls_interval_value = (uint64_t)(atof(argv[4]) / 100.0);
		master_lsp->ls_interval_type = LS_INTERVAL_PERCENT;
		master_lsp->ls_interval_units = "PERCENT";
		if ((master_lsp->ls_interval_value < 0.0) || (master_lsp->ls_interval_value > 1.0)) {
			fprintf(stderr,"%s: Invalid lockstep interval percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, atof(argv[4]) );
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		master_lsp->ls_interval_value = (uint64_t)(tmpf * 1024*1024);
		master_lsp->ls_interval_type = LS_INTERVAL_BYTES;
		master_lsp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		master_lsp->ls_interval_value = (uint64_t)(tmpf * 1024);
		master_lsp->ls_interval_type = LS_INTERVAL_BYTES;
		master_lsp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		retval = 5;   
	} else {
		fprintf(stderr,"%s: Invalid lockstep interval qualifer: %s\n",
				xgp->progname, when);
        return(0);
	}
	/* This section looks at what the slave target is supposed to do for a "task" */
	what = argv[5];
	if (strcmp(what,"time") == 0){ /* get the number of seconds to run a task */
		tmpf = atof(argv[6]);
		slave_lsp->ls_interval_type = LS_INTERVAL_TIME;
		slave_lsp->ls_interval_units = "SECONDS";
		slave_lsp->ls_interval_value = (nclk_t)(tmpf * BILLION);
		if (slave_lsp->ls_interval_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval += 2;
	} else if (strcmp(what,"op") == 0){ /* get the number of operations to execute per task */
		slave_lsp->ls_interval_value = atoll(argv[6]);
		slave_lsp->ls_interval_type = LS_INTERVAL_OP;
		slave_lsp->ls_interval_units = "OPERATIONS";
		if (slave_lsp->ls_interval_value <= 0) {
			fprintf(stderr,"%s: Invalid lockstep task op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)slave_lsp->ls_interval_value);
            return(0);
		}
		retval += 2;       
	} else if (strcmp(what,"mbytes") == 0){ /* get the number of megabytes to transfer per task */
		tmpf = atof(argv[6]);
		slave_lsp->ls_interval_value = (uint64_t)(tmpf * 1024*1024);
		slave_lsp->ls_interval_type = LS_INTERVAL_BYTES;
		slave_lsp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
        return(0);
		}
		retval += 2;     
	} else if (strcmp(what,"kbytes") == 0){ /* get the number of kilobytes to transfer per task */
		tmpf = atof(argv[6]);
		slave_lsp->ls_interval_value = (uint64_t)(tmpf * 1024);
		slave_lsp->ls_interval_type = LS_INTERVAL_BYTES;
		slave_lsp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
        return(0);
		}
		retval += 2;    
	} else {
		fprintf(stderr,"%s: Invalid lockstep task qualifer: %s\n",
				xgp->progname, what);
        return(0);
	}
	lockstep_startup = argv[7];  
	if (strcmp(lockstep_startup,"run") == 0) { /* have the slave start running immediately */
		slave_lsp->ls_state |= LS_STATE_START_RUN;
	} else { /* Have the slave wait for the master to tell it to run */
		slave_lsp->ls_state |= LS_STATE_START_WAIT;
	}
    retval++;
	lockstep_completion = argv[8];
	if (strcmp(lockstep_completion,"complete") == 0) { /* Have slave complete all operations if master finishes first */
		slave_lsp->ls_state |= LS_STATE_END_COMPLETE;
		master_lsp->ls_state |= LS_STATE_END_COMPLETE;
	} else if (strcmp(lockstep_completion,"stop") == 0){ /* Have slave stop when master stops */
		slave_lsp->ls_state |= LS_STATE_END_STOP;
		master_lsp->ls_state |= LS_STATE_END_STOP;
	} else {
        return(0);
    }
    retval++;

    return(retval);
} // End of xddfunc_lockstep()
/*----------------------------------------------------------------------------*/
// Loose Ordering - Allow loose ordering of Worker Thread I/O
// Note that Loose Ordering for a Target is mutually exclusive with Serial Ordering
// Arguments: -looseordering [target #] 
// aka -lo 
int
xddfunc_looseordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_ORDERING_STORAGE_LOOSE;
		tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
        return(args+1);
    } else {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_ORDERING_STORAGE_LOOSE;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(1);
	}
} // End of  xddfunc_looseordering()
/*----------------------------------------------------------------------------*/
// Set the maxpri and process lock
int
xddfunc_maxall(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int status;

	if (flags & XDD_PARSE_PHASE2) {
		status = xddfunc_maxpri(planp, 1,0, flags);
		status += xddfunc_processlock(planp, 1,0, flags);
		if (status < 2)
			return(-1);
	}
	return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the maximum number of errors to tolerate before exiting
int
xddfunc_maxerrors(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	long long errors;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for max errors\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		errors = atoll(argv[1]);
		if (errors <= 0) {
			fprintf(stderr,"%s: Error: Max errors value of '%lld' cannot be negative\n", xgp->progname, errors);
			return(-1);
		} 
		xgp->max_errors = errors;
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the maximum number of errors to print when displaying miscompares on a -verify contents
int
xddfunc_max_errors_to_print(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int errors;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for max errors to print\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		errors = atoi(argv[1]);
		if (errors <= 0) {
			fprintf(stderr,"%s: Error: Max errors to print value of '%d' cannot be negative\n", xgp->progname, errors);
			return(-1);
		} 
		xgp->max_errors_to_print = errors;
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
// Set the maximum runtime priority
int
xddfunc_maxpri(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_MAXPRI;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of MBytes to transfer per pass (1M=1024*1024 bytes)
// Arguments: -mbytes [target #] #
// This will set tdp->td_bytes to the calculated value (mbytes * 1024*1024)
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the tdp->td_numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_mbytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int64_t mbytes;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	mbytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_bytes = mbytes * 1024 * 1024;
		tdp->td_numreqs = 0;
        return(args+2);
	} else { // Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_bytes = mbytes * 1024 * 1024;
					tdp->td_numreqs = 0;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_memalign(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
	int32_t align;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);


	align = atoi(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_mem_align = align;
		return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) { 
				tdp->td_mem_align = align;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
} 
/*----------------------------------------------------------------------------*/
// Set the  no mem lock and no proc lock flags 
int
xddfunc_minall(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int status;
 
	if (flags & XDD_PARSE_PHASE2) {	
		status = xddfunc_nomemlock(planp,1,0,flags);
		status += xddfunc_noproclock(planp,1,0,flags);
		if (status < 2)
			return(0);
		else return(1);
	}
	return(1);
}
/*----------------------------------------------------------------------------*/
// Set up a queue thread for the specified target to act as an alternate
// path to the main target device
int
xddfunc_multipath(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	fprintf(stderr,"multipath not implemented\n");
	return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_nobarrier(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOBARRIER;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the no memory lock flag
int
xddfunc_nomemlock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOMEMLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// No Ordering - Turn off Loose and Serial ordering of Worker Thread I/O
// aka -no 
int
xddfunc_noordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { 
		/* Unset the Loose and Serial Ordering Opetions for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
		tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
        return(args+1);
    } else {// Put this option into all Targets 
			/* Unset the Loose and Serial Ordering Opetions for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(1);
	}
} // End of  xddfunc_looseordering()
/*----------------------------------------------------------------------------*/
// Set the no process lock flag
int
xddfunc_noproclock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOPROCLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of requests to run 
// Arguments: -numreqs [target #] #
// This will set tdp->td_numreqs to the specified value 
// The -numreqs option is mutually exclusive with the -bytes/-kbytes/-mbytes options
// and will reset the tdp->td_bytes to 0 for all affected targets. The number of 
// bytes to transfer will be calculated at a later time.
// 
int
xddfunc_numreqs(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int 		args, i; 
	int 		target_number;
	target_data_t 		*tdp;
	int64_t 	numreqs;
	
	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
	if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	numreqs = atoll(argv[args+1]);
	if (numreqs <= 0) {
		fprintf(xgp->errout,"%s: numreqs of %lld is not valid. numreqs must be a number greater than 0\n",
			xgp->progname,
			(long long)numreqs);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_numreqs = numreqs;
		tdp->td_bytes = 0; // reset tdp->td_bytes
		return(args+2);

	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_numreqs = numreqs;
				tdp->td_bytes = 0; // reset tdp->td_bytes
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Seconds to delay between individual operations
int
xddfunc_operationdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
    int 	args, i; 
    int 	target_number;
    target_data_t 	*tdp;
	int 	operationdelay;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	operationdelay = atoi(argv[args+1]);
	if (operationdelay <= 0) {
			fprintf(xgp->errout,"%s: Operation delay of %d is not valid. Operation delay must be a number greater than 0\n",
				xgp->progname,
				operationdelay);
            return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_op_delay = operationdelay;
        return(args+2);

	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_op_delay = operationdelay;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the operation to perform - read or write 
// Arguments: -op [target #] read|write
int
xddfunc_operation(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 	args, i; 
    int 	target_number;
    target_data_t 	*tdp;
	char	*opname;
	double 	rwratio;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	opname = (char *)argv[args+1];

	if (strcmp(opname, "write") == 0) {
		rwratio = 0.0; /* all write operations */
	} else if (strcmp(opname, "read") == 0) {
		rwratio = 1.0; /* all read operations */
	} else if ((strcmp(opname, "noop") == 0) || (strcmp(opname, "nop") == 0)) {
		rwratio = -1.0; /* all NOOP operations */
	} else {
		fprintf(xgp->errout,"%s: xddfunc_operation: ERROR: Operation '%s' is not valid. Acceptable operations are 'read', 'write', or 'noop'.\n",
			xgp->progname,
			opname);
			return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_rwratio = rwratio;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_rwratio = rwratio;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
} // End of xddfunc_operation()
/*----------------------------------------------------------------------------*/
// Specify the ordering method to apply to I/O 
// Arguments: -ordering [target #] <storage | network | both>  <serial | loose | none>
int
xddfunc_ordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int			args, i; 
    int			target_number;
    target_data_t		*tdp;
	char		*thing;
	char		network_ordering = 0;
	char		storage_ordering = 0;
	char		serial_network_ordering = 0;
	char		serial_storage_ordering = 0;
	char		loose_network_ordering = 0;
	char		loose_storage_ordering = 0;
	char		no_network_ordering = 0;
	char		no_storage_ordering = 0;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);


	// Figure out "what" to order - storage, network, or both
	thing = (char *)argv[args+1];
	if ((strcmp(thing, "storage") == 0) || (strcmp(thing, "stor") == 0) || (strcmp(thing, "disk") == 0)) {
		storage_ordering = 1;
	} else if ((strcmp(thing, "network") == 0) || (strcmp(thing, "net") == 0)) {
		network_ordering = 1;
	} else if (strcmp(thing, "both") == 0) {
		storage_ordering = 1;
		network_ordering = 1;
	} else {
		fprintf(xgp->errout,"%s: xddfunc_ordering: ERROR: Don't know how to order '%s'. This should be either 'storage', 'network', or 'both'.\n",
			xgp->progname,
			thing);
			return(0);
	}
	// Now determine the requested ordering method: serial, loose, or none
	// Note that the ordering mechanisms are mutually exclusive so if one is enabled, the others are disabled
	thing = (char *)argv[args+2];
	if ((strcmp(thing, "serial") == 0) || (strcmp(thing, "ser") == 0) || (strcmp(thing, "so") == 0)) {
		if (storage_ordering)
			serial_storage_ordering = 1; 
		if (network_ordering)
			serial_network_ordering = 1; 
	} else if ((strcmp(thing, "loose") == 0) || (strcmp(thing, "lo") == 0)) {
		if (storage_ordering)
			loose_storage_ordering = 1; 
		if (network_ordering)
			loose_network_ordering = 1; 
	} else if ((strcmp(thing, "none") == 0) || (strcmp(thing, "no") == 0)) {
		if (storage_ordering)
			no_storage_ordering = 1; 
		if (network_ordering)
			no_network_ordering = 1; 
	} else {
		fprintf(xgp->errout,"%s: xddfunc_ordering: ERROR: Don't know what '%s' ordering is. This should be either 'serial', 'loose', or 'none'.\n",
			xgp->progname,
			thing);
			return(0);
	}
	// At this point the "order" variable has the proper flags OR'd into it so just or "order" into the target options for either a specific target or all targets
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		if (serial_storage_ordering) {
			tdp->td_target_options |= TO_ORDERING_STORAGE_SERIAL;
			tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
		} else if (loose_storage_ordering) {
			tdp->td_target_options |= TO_ORDERING_STORAGE_LOOSE;
			tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
		} else if (no_storage_ordering) {
			tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
			tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
		}
		if (serial_network_ordering) {
			tdp->td_target_options |= TO_ORDERING_NETWORK_SERIAL;
			tdp->td_target_options &= ~TO_ORDERING_NETWORK_LOOSE;
		} else if (loose_network_ordering) {
			tdp->td_target_options |= TO_ORDERING_NETWORK_LOOSE;
			tdp->td_target_options &= ~TO_ORDERING_NETWORK_SERIAL;
		} else if (no_network_ordering) {
			tdp->td_target_options &= ~TO_ORDERING_NETWORK_SERIAL;
			tdp->td_target_options &= ~TO_ORDERING_NETWORK_LOOSE;
		}
        return(args+3);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				if (serial_storage_ordering) {
					tdp->td_target_options |= TO_ORDERING_STORAGE_SERIAL;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
				} else if (loose_storage_ordering) {
					tdp->td_target_options |= TO_ORDERING_STORAGE_LOOSE;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
				} else if (no_storage_ordering) {
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_SERIAL;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
				}
				if (serial_network_ordering) {
					tdp->td_target_options |= TO_ORDERING_NETWORK_SERIAL;
					tdp->td_target_options &= ~TO_ORDERING_NETWORK_LOOSE;
				} else if (loose_network_ordering) {
					tdp->td_target_options |= TO_ORDERING_NETWORK_LOOSE;
					tdp->td_target_options &= ~TO_ORDERING_NETWORK_SERIAL;
				} else if (no_network_ordering) {
					tdp->td_target_options &= ~TO_ORDERING_NETWORK_SERIAL;
					tdp->td_target_options &= ~TO_ORDERING_NETWORK_LOOSE;
				}
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(3);
	}
} // End of xddfunc_ordering()
/*----------------------------------------------------------------------------*/
int
xddfunc_output(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	
	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No file name specified for output file\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		xgp->output_filename = argv[1];
		xgp->output = fopen(xgp->output_filename,"a");
		if (xgp->output == NULL) {
			fprintf(xgp->errout,"%s: Error: Cannot open output file %s - defaulting to standard out\n", xgp->progname,argv[1]);
			xgp->output = stdout;
			xgp->output_filename = "stdout";
		}
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_output_format(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	
	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No format string specified for '-outputformat' option\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		if (strcmp(argv[1], "add") == 0) {
			if (argc <= 2) {
				fprintf(stderr,"%s: Error: No format string specified for '-outputformat add' option\n", xgp->progname);
				return(-1);
			}
			xdd_results_format_id_add(argv[2], planp->format_string);
			return(3);
		}
		if (strcmp(argv[1], "new") == 0) {
			if (argc <= 2) {
				fprintf(stderr,"%s: Error: No format string specified for '-outputformat new' option\n", xgp->progname);
				return(-1);
			}
			planp->format_string = argv[2];
			return(3);
		}
	}
    return(3);
}
/*----------------------------------------------------------------------------*/
// Seconds to delay between passes
int
xddfunc_passdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 

	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for pass delay\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		planp->pass_delay = atof(argv[1]);
		if (planp->pass_delay <= 0.0) {
			fprintf(xgp->errout,"%s: pass delay time of %f is not valid. The pass delay time must be a number of seconds greater than 0.00 but less than the remaining life of the sun.\n",xgp->progname,planp->pass_delay);
			return(0);
		}
		planp->pass_delay_usec = (nclk_t)(planp->pass_delay * MILLION);
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_passes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int	passes;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for number of passes\n", xgp->progname);
		return(-1);
	}

	if (flags & XDD_PARSE_PHASE2) {
		passes = atoi(argv[1]);
		if (passes <= 0) {
			fprintf(stderr,"%s: Error: Pass count value of '%d' cannot be negative\n", xgp->progname, passes);
			return(-1);
		}
		planp->passes = passes;
	}
    return(2);
}

/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -passoffset [target #] #
// 
int
xddfunc_passoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int64_t pass_offset;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	pass_offset = atoll(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_pass_offset = pass_offset;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_pass_offset = pass_offset;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
    } 
}
/*----------------------------------------------------------------------------*/
// This defines the meaning of the Percent CPU values that are displayed.
int
xddfunc_percentcpu(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);


	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		/*
        if ( strcmp(argv[args+1], "absolute") == 0) 
            tdp->td_target_options = TO_PCPU_ABSOLUTE; // Make sure 'absolute' percent CPU is on
        else if (strcmp(argv[args+1], "relative") == 0)
            tdp->td_target_options &= ~TO_PCPU_ABSOLUTE; // For "relative" percent CPU, turn off 'absolute" percent CPU
        else {
            fprintf(xgp->errout,"%s: Error: The percent CPU specification of '%s' is not valid. It should be either 'absolute' or 'relative'. Defaulting to 'absolute'\n",xgp->progname, argv[args+1]);
            tdp->td_target_options = TO_PCPU_ABSOLUTE; // Make sure 'absolute' percent CPU is on
        }
		*/
		return(args+2);
    } else { 
        return(2);
    }
}
/*----------------------------------------------------------------------------*/
// Specify the number of bytes to preallocate for a target file that is 
// being created. This option is only valid when used on operating systems
// and file systems that support the Reserve Space file operation.
int
xddfunc_preallocate(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	int 		args, i; 
	int 		target_number;
	target_data_t 		*tdp;
	int64_t 	preallocate;

	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
	if (args < 0) return(-1);
	
	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);
	
	preallocate = (int64_t)atoll(argv[args+1]);
	if (preallocate <= 0) {
		fprintf(stderr,"%s: Error: Preallocate value of '%lld' is not valid - it must be greater than or equal to zero\n", xgp->progname, (long long int)preallocate);
		return(-1);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		
		tdp->td_preallocate = preallocate;
		return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_preallocate = preallocate;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the number of bytes to truncate the file size to  target file that is 
// being created. This option may zero-fill for some file systems.
int
xddfunc_pretruncate(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int         args, i;
    int         target_number;
    target_data_t       *tdp;
    int64_t     pretruncate;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
        return(0);

    pretruncate = (int64_t)atoll(argv[args+1]);
    if (pretruncate <= 0) {
        fprintf(stderr,"%s: Error: Pretruncate value of '%lld' is not valid - it must be greater than or equal to zero\n", xgp->progname, (long long int)pretruncate);
        return(-1);
    }

    if (target_number >= 0) { /* Set this option value for a specific target */
        tdp = xdd_get_target_datap(planp, target_number, argv[0]);
        if (tdp == NULL) return(-1);

        tdp->td_pretruncate= pretruncate;
        return(args+2);
    } else { // Put this option into all Targets 
        if (flags & XDD_PARSE_PHASE2) {
            tdp = planp->target_datap[0];
            i = 0;
            while (tdp) {
                tdp->td_pretruncate= pretruncate;
                i++;
                tdp = planp->target_datap[i];
            }
        }
        return(2);
    }
}
/*----------------------------------------------------------------------------*/
// Lock the process in memory
int
xddfunc_processlock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) 
		xgp->global_options |= GO_PLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// processor/target assignment
int
xddfunc_processor(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int32_t args, i; 
	int32_t cpus;
	int32_t processor_number;
	int target_number;
	target_data_t *tdp;


	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
	if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	cpus = xdd_cpu_count();
	processor_number = atoi(argv[args+1]); /* processor to run on */
	if ((processor_number < 0) || (processor_number >= cpus)) {
		fprintf(xgp->errout,"%s: Error: Processor number <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Processor number should be between 0 and %d\n",xgp->progname, cpus-1);
        return(0);
	} 

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
			tdp->td_processor = processor_number;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {	
				tdp->td_processor = processor_number;
				i++;
				tdp = planp->target_datap[i];
			}
		}
	return(2);
	} 
}
/*----------------------------------------------------------------------------*/
int
xddfunc_queuedepth(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int32_t queue_depth;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	queue_depth = atoi(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_queue_depth = queue_depth;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_queue_depth = queue_depth;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify that the seek list should be randomized between passes
// Arguments: -randomize [target #]

int
xddfunc_randomize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
    if (target_number >= 0) { /* Set this option value for a specific target */
	    tdp = xdd_get_target_datap(planp, target_number, argv[0]);
	    if (tdp == NULL) return(-1);

	    tdp->td_target_options |= TO_PASS_RANDOMIZE;
	    return(args+1);
    } else { // Put this option into all Targets 
	    if (flags & XDD_PARSE_PHASE2) {
		    tdp = planp->target_datap[0];
		    i = 0;
		    while (tdp) {
			    tdp->td_target_options |= TO_PASS_RANDOMIZE;
			    i++;
			    tdp = planp->target_datap[i];
		    }
	    }
	    return(1);
    }
}
/*----------------------------------------------------------------------------*/
// Specify the read-after-write options for either the reader or the writer
// Arguments: -readafterwrite [target #] option_name value
// Valid options are trigger [stat | mp]
//                   lag <#>
//                   reader <hostname>
//                   port <#>
// 
int
xddfunc_readafterwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int			 	i;
    int				args; 
    int				target_number;
	xint_raw_t		*rawp;
    target_data_t	*tdp;

	i = 1;
    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for read-after-write option %s\n",
					xgp->progname, target_number, argv[i+2]);
			return(0);
		}
		i += args;  /* skip past the "target <taget number>" */
	}
	/* At this point "i" points to the raw "option" argument */
	if (strcmp(argv[i], "trigger") == 0) { /* set the the trigger type */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rawp = xdd_get_rawp(tdp);
			if (rawp == NULL) return(-1);
			tdp->td_target_options |= TO_READAFTERWRITE;
			if (strcmp(argv[i+1], "stat") == 0)
					rawp->raw_trigger |= RAW_STAT;
				else if (strcmp(argv[i+1], "mp") == 0)
					rawp->raw_trigger |= RAW_MP;
				else {
					fprintf(stderr,"%s: Invalid trigger type specified for read-after-write option: %s\n",
						xgp->progname, argv[i+1]);
					return(0);
				}
		}
        return(i+2);
	} else if (strcmp(argv[i], "lag") == 0) { /* set the lag block count */
		if (target_number >= 0) {
			/* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rawp = xdd_get_rawp(tdp);
			if (rawp == NULL) return(-1);
			tdp->td_target_options |= TO_READAFTERWRITE;
			rawp->raw_lag = atoi(argv[i+1]);
		}
        return(i+2);
	} else if (strcmp(argv[i], "reader") == 0) { /* hostname of the reader for this read-after-write */
		/* This assumes that these targets are all writers and need to know who the reader is */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rawp = xdd_get_rawp(tdp);
			if (rawp == NULL) return(-1);
			tdp->td_target_options |= TO_READAFTERWRITE;
			rawp->raw_hostname = argv[i+1];
		}
        return(i+2);
	} else if (strcmp(argv[i], "port") == 0) { /* set the port number for the socket used by the writer */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rawp = xdd_get_rawp(tdp);
			if (rawp == NULL) return(-1);
			tdp->td_target_options |= TO_READAFTERWRITE;
			rawp->raw_port = atoi(argv[i+1]);
		}
        return(i+2);
	} else {
			fprintf(stderr,"%s: Invalid Read-after-write option %s\n",xgp->progname, argv[i+1]);
            return(0);
	}/* End of the -readafterwrite (raw) sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_reallyverbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) 
		xgp->global_options |= GO_REALLYVERBOSE;
    return(1);
}

/*----------------------------------------------------------------------------*/
// Re-create the target file between each pass
int
xddfunc_recreatefiles(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

	    tdp->td_target_options |= TO_RECREATE;
        return(args+1);
    } else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_RECREATE;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(1);
	}
}

/*----------------------------------------------------------------------------*/
// Re-open the target file between each pass
int
xddfunc_reopen(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

	    tdp->td_target_options |= TO_REOPEN;
        return(args+1);
    } else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_REOPEN;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the reporting threshold for I/O operations that take more than a
// certain time to complete for either a single target or all targets 
// Arguments: -reportthreshold [target #] #.#
int
xddfunc_report_threshold(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
    double threshold;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	threshold = atof(argv[args+1]);
	if (threshold < 0.0) {
		fprintf(xgp->errout,"%s: report threshold of %5.2f is not valid. rwratio must be a positive number\n",xgp->progname,threshold);
        return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_report_threshold = (nclk_t)(threshold * BILLION);
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_report_threshold = (nclk_t)(threshold * BILLION);
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the I/O request size in blocks for either a single target or all targets 
// Arguments: -reqsize [target #] #
int
xddfunc_reqsize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int32_t reqsize;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	reqsize = atoi(argv[args+1]);
	if (reqsize <= 0) {
		fprintf(xgp->errout,"%s: reqsize of %d is not valid. reqsize must be a number greater than 0\n",xgp->progname,reqsize);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_reqsize = reqsize;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_reqsize = reqsize;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
} // End of xddfunc_reqsize()
/*----------------------------------------------------------------------------*/
// Control restart operation options
int
xddfunc_restart(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 			i; 
    int 			args; 
    int 			args_index;
    int 			target_number;
    target_data_t 	*tdp;
	xint_restart_t	*rp;


	args_index = 1;
    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for seek option %s\n",
					xgp->progname, target_number, argv[args_index+args]);
            return(0);
		}
		args_index += args;  /* skip past the "target <taget number>" */
	}
	/* At this point "args_index" is an index to the "option" argument */
	if (strcmp(argv[args_index], "enable") == 0) { /* Enable the restart option for this command */
		if(planp->restart_frequency == 0) 
			planp->restart_frequency = 1;
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rp = xdd_get_restartp(tdp);
			if (rp == NULL) return(-1);
			tdp->td_target_options |= TO_RESTART_ENABLE;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					rp = xdd_get_restartp(tdp);
					if (rp == NULL) return(-1);
					tdp->td_target_options |= TO_RESTART_ENABLE;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "file") == 0) { /* Use this filename for the restart file */
		if(planp->restart_frequency == 0)  // Turn on restart 
			planp->restart_frequency = 1;
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rp = xdd_get_restartp(tdp);
			if (rp == NULL) return(-1);
			rp->restart_filename = argv[args_index+1];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					rp = xdd_get_restartp(tdp);
					if (rp == NULL) return(-1);
					rp->restart_filename = argv[args_index+1];
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+2);
	} else if ((strcmp(argv[args_index], "frequency") == 0) || 
			   (strcmp(argv[args_index], "freq") == 0)) { // The frequency in seconds to check the threads
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rp = xdd_get_restartp(tdp);
			if (rp == NULL) return(-1);
			planp->restart_frequency = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					rp = xdd_get_restartp(tdp);
					if (rp == NULL) return(-1);
					planp->restart_frequency = atoi(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+2);
	} else if ((strcmp(argv[args_index], "byteoffset") == 0) || 
			   (strcmp(argv[args_index], "offset") == 0)) { /*  Restart from a specific offset */
		if(planp->restart_frequency == 0) 
			planp->restart_frequency = 1;
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			rp = xdd_get_restartp(tdp);
			if (rp == NULL) return(-1);
			
	    	if (NULL == tdp->td_e2ep) // If there is no e2e struct, then allocate one
				tdp->td_e2ep = xdd_get_e2ep();
	    	if (tdp->td_e2ep == NULL) // If there is still no e2e struct then return -1
				return(-1);

			rp->initial_restart_offset = atoll(argv[args_index+1]);
			rp->byte_offset = rp->initial_restart_offset;
			rp->flags |= RESTART_FLAG_RESUME_COPY;
			tdp->td_e2ep->e2e_total_bytes_written=rp->byte_offset;

			/* Set the last committed location to avoid restart output of
			   0 if the target does not complete any I/O during first interval 
			 */
			tdp->td_last_committed_location = rp->byte_offset;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					rp = xdd_get_restartp(tdp);
					if (rp == NULL) return(-1);
			
	    			if (NULL == tdp->td_e2ep) // If there is no e2e struct, then allocate one
						tdp->td_e2ep = xdd_get_e2ep();
	    			if (tdp->td_e2ep == NULL) // If there is still no e2e struct then return -1
						return(-1);

					rp->initial_restart_offset = atoll(argv[args_index+1]);
					rp->byte_offset = rp->initial_restart_offset;
					rp->flags |= RESTART_FLAG_RESUME_COPY;
					tdp->td_e2ep->e2e_total_bytes_written=rp->byte_offset;

					/* Set the last committed location to avoid restart output 
					   of 0 if the target does not complete any I/O during 
					   first interval */
					tdp->td_last_committed_location = rp->byte_offset;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+2);
    } else {
			fprintf(stderr,"%s: Invalid RESTART suboption %s\n",xgp->progname, argv[args_index]);
            return(0);
    } /* End of the -restart sub options */

} // End of xddfunc_restart()
/*----------------------------------------------------------------------------*/
// Set the retry count for each target. The retry count gets inherited by any
// nsubsequent Worker Threads for the target.
int
xddfunc_retry(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	int32_t retry_count;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	retry_count = atoi(argv[args+1]);
	if (retry_count <= 0) {
		fprintf(xgp->errout,"%s: Retry count of %d is not valid. Retry count must be a number greater than 0\n",xgp->progname,retry_count);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_retry_count = retry_count;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_retry_count = retry_count;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// round robin processor target assignment
int
xddfunc_roundrobin(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int32_t cpus;
    int32_t processor_number;

	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for round robin\n", xgp->progname);
		return(-1);
	}
	cpus = xdd_cpu_count();
	processor_number = atoi(argv[1]);
	if ((processor_number < 1) || (processor_number > cpus)) {
		fprintf(xgp->errout,"%s: Error: Number of processors <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Number of processors should be between 1 and %d\n",xgp->progname, cpus);
		return(0);
	}
	/* - this needs to be fixed
	k = 0;
	for (j = 0; j < MAX_TARGETS; j++) {
		tdp = xdd_get_target_datap(planp, j, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_processor = k;
		k++;
		if (k >= processor_number)
			k = 0;
	}
	*/
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_runtime(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for run time\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		planp->run_time = atof(argv[1]);
		if (planp->run_time <= 0.0) {
			fprintf(xgp->errout,"%s: run time of %f is not valid. The run time must be a number of seconds greater than 0.00 but less than the remaining life of the sun.\n",xgp->progname,planp->run_time);
			return(0);
		}
		planp->run_time_ticks = (nclk_t)(planp->run_time * BILLION);
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the read/write ratio for either a single target or all targets 
// Arguments: -rwratio [target #] #.#
int
xddfunc_rwratio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
    double rwratio;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	rwratio = (atof(argv[args+1]) / 100.0);
	if ((rwratio < 0.0) || (rwratio > 1.0)) {
		fprintf(xgp->errout,"%s: rwratio of %5.2f is not valid. rwratio must be a number between 0.0 and 100.0\n",xgp->progname,rwratio);
        return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_rwratio = rwratio;
        return(args+2);
	} else { // Put this option into all Targets 
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_rwratio = rwratio;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -seek [target #] option_name value
// 
int
xddfunc_seek(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int     i;
    int     args, args_index; 
    int     target_number;
    target_data_t  *tdp;

	args_index = 1;
    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for seek option %s\n",
					xgp->progname, target_number, argv[args_index+args]);
            return(0);
		}
		args_index += args;  /* skip past the "target <taget number>" */
	}
	/* At this point "args_index" is an index to the seek "option" argument */
	if (strcmp(argv[args_index], "save") == 0) { /* save the seek information in a file */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_SAVE;
			tdp->td_seekhdr.seek_savefile = argv[args_index+1];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_SAVE;
					tdp->td_seekhdr.seek_savefile = argv[args_index+1];
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+2);
	} else if (strcmp(argv[args_index], "load") == 0) { /* load seek list from "filename" */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_LOAD;
			tdp->td_seekhdr.seek_loadfile = argv[args_index+1];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_LOAD;
					tdp->td_seekhdr.seek_loadfile = argv[args_index+1];
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+2);
	} else if (strcmp(argv[args_index], "disthist") == 0) { /*  Print a Distance Histogram */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_DISTHIST;
			tdp->td_seekhdr.seek_NumDistHistBuckets = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_DISTHIST;
					tdp->td_seekhdr.seek_NumDistHistBuckets = atoi(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "seekhist") == 0) { /* Print a Seek Histogram */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_SEEKHIST;
			tdp->td_seekhdr.seek_NumSeekHistBuckets = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_SEEKHIST;
					tdp->td_seekhdr.seek_NumSeekHistBuckets = atoi(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+2);
	} else if (strcmp(argv[args_index], "sequential") == 0) { /*  Sequential seek list option */     
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options &= ~SO_SEEK_RANDOM;
			tdp->td_seekhdr.seek_pattern = "sequential";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options &= ~SO_SEEK_RANDOM;
					tdp->td_seekhdr.seek_pattern = "sequential";
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+1);
	} else if (strcmp(argv[args_index], "random") == 0) { /*  Random seek list option */     
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_RANDOM;
			tdp->td_seekhdr.seek_pattern = "random";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_RANDOM;
					tdp->td_seekhdr.seek_pattern = "random";
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+1);
	} else if (strcmp(argv[args_index], "stagger") == 0) { /*  Staggered seek list option */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_STAGGER;
			tdp->td_seekhdr.seek_pattern = "staggered";
			tdp->td_seekhdr.seek_stride = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_STAGGER;
					tdp->td_seekhdr.seek_pattern = "staggered";
			                tdp->td_seekhdr.seek_stride = atoi(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "interleave") == 0) { /* set the interleave for sequential seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_interleave = atoi(argv[args_index+1]);
			tdp->td_seekhdr.seek_pattern = "interleaved";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_interleave = atoi(argv[args_index+1]);
					tdp->td_seekhdr.seek_pattern = "interleaved";
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "none") == 0) { /* no seeking at all */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_options |= SO_SEEK_NONE;
			tdp->td_seekhdr.seek_pattern = "none";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_options |= SO_SEEK_NONE;
					tdp->td_seekhdr.seek_pattern = "none";
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+1);
	} else if (strcmp(argv[args_index], "range") == 0) { /* set the range of seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_range = atoll(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_range = atoll(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "seed") == 0) { /* set the seed for random seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_seekhdr.seek_seed = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_seekhdr.seek_seed = atoi(argv[args_index+1]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		} 
		return(args_index+2);
    } else {
			fprintf(stderr,"%s: Invalid Seek option %s\n",xgp->progname, argv[args_index]);
            return(0);
    } /* End of the -seek sub options */
}
/*----------------------------------------------------------------------------*/
// Serial Ordering - Enforce Serial Ordering on Worker Thread I/O
// Note that Serial Ordering for a Target is mutually exclusive with Loose Ordering
// Arguments: -serialordering [target #] 
// aka -nso 
int
xddfunc_serialordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_ORDERING_STORAGE_SERIAL;
		tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
        return(args+1);
    } else {// Put this option into all Targets 
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_ORDERING_STORAGE_SERIAL;
					tdp->td_target_options &= ~TO_ORDERING_STORAGE_LOOSE;
					i++;
					tdp = planp->target_datap[i];
				}
			}
        return(1);
	}
} // End of  xddfunc_serialordering()
/*----------------------------------------------------------------------------*/
int
xddfunc_setup(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int status;
	
	if (flags & XDD_PARSE_PHASE1) {
		status = xdd_process_paramfile(planp, argv[1]);
		if (status == 0)
			return(0);
	}
	return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the use of SCSI Generic I/O for a single target or for all targets
// Arguments: -sgio [target #]
int
xddfunc_sgio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_SGIO;
        return(args+1);
    } else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_SGIO;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(1);
    }
}
/*----------------------------------------------------------------------------*/
int
xddfunc_sharedmemory(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_SHARED_MEMORY;
        return(args+1);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_SHARED_MEMORY;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(1);
	}
}
/*----------------------------------------------------------------------------*/	
// single processor scheduling
int
xddfunc_singleproc(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
    int32_t cpus;
    int32_t processor_number;
    int32_t args, i; 
    int target_number;
    target_data_t *tdp;


	cpus = xdd_cpu_count();
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	processor_number = atoi(argv[args+1]);
	if ((processor_number < 0) || (processor_number >= cpus)) {
		fprintf(xgp->errout,"%s: Error: Processor number <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Processor number should be between 0 and %d\n",xgp->progname, cpus-1);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_processor = processor_number;
		return(args+1);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_processor = processor_number;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// The start delay function will set the "start delay" time for all
// targets or for a specific target if specified.
// For example, assuming four targets have been specified, the option
// "-startdelay 2.1" will set the start times of each target like so:
//   Target#   StartDelay (seconds)
//     0        2.1
//     1        2.1
//     2        2.1
//     3        2.1
//
// ---or - to change the start delay of a single target accordingly:
// "-startdelay target 2 5.3" will set the start delay time of 
// target 2 to a value of 5.3 seconds while leaving the start delay 
// times of the other targets unaltered.
//
// See also: -targetstartdelay (aka xddfunc_targetstartdelay)
//
int
xddfunc_startdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    double start_delay;
    nclk_t start_delay_psec;
    target_data_t *tdp;

		
	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);
	start_delay = atof(argv[args+1]);
	start_delay_psec = (nclk_t)(start_delay * BILLION); 
	if (start_delay < 0.0) {
		fprintf(xgp->errout,"%s: Start Delay time of %f is not valid. The Start Delay time must be a number of seconds greater than or equal to 0.00 but less than the remaining life of the sun.\n",
			xgp->progname,start_delay);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_start_delay = start_delay;
		tdp->td_start_delay_psec = start_delay_psec;

		return(args+2);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_start_delay = start_delay;
				tdp->td_start_delay_psec = start_delay_psec;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -startoffset [target #] #
// 
int
xddfunc_startoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
	int64_t	start_offset;
    target_data_t *tdp;

	
	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	start_offset = atoll(argv[args+1]);
	if (start_offset < 0) {
		fprintf(xgp->errout,"%s: start offset of %lld is not valid. start offset must be a number equal to or greater than 0\n",xgp->progname,(long long)start_offset);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_start_offset = start_offset;
        return(args+2);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_start_offset = start_offset;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_starttime(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	planp->gts_time = (nclk_t) atoll(argv[1]);
	planp->gts_time *= BILLION;
    return(2);
}
/*----------------------------------------------------------------------------*/
/* The Trigger options all fillow the same format:  
*                -xxxtrigger <target1> <target2> <when #>
                    0           1          2       3   4 
* Where "target1" is the target that generates the trigger and sends it to target2.
*   "target2" is the target that responds to the trigger from trigger 1.
*   "when #" specifies when the trigger should happen. The word "when" should be replaced with
*    the word "time", "op",  "percent", "mbytes", or "kbytes" followed by "howlong" is 
*     either the number of seconds, number of operations, or a percentage from 0.1-100.0, ...etc.
*       - the time in seconds (a floating point number) as measured from the start of the pass
*      before the trigger is sent to target2
*       - the operation number that target1 should reach before sending trigger to target2
*       - the percentage of data that target1 should transfer before the trigger is sent
*      to target2.
*       - the number of megabytes (1024*1024 bytes) or the number of kilobytes (1024 bytes)
*       
* Usage notes:
* Target1 should not equal target2 - this does not make sense.
* Target1 and target2 should not try to single each other to start - this is not possible.
* Target1 can signal target2 to start and target2 can later signal target1 to stop.
*       
*/
int
xddfunc_starttrigger(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 			t1,t2;				// Target numbers
    target_data_t 			*tdp1, *tdp2;			// Target Data pointers for the two targets involved
    xint_triggers_t 	*trigp;				// Trigger Stucture pointers for the this target
    char 			*when;				// The "When" to perform a trigger
    double 			tmpf;				// temp

		  
	if (argc < 5) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}

	t1 = atoi(argv[1]); /* T1 is the target that does the triggering */
	t2 = atoi(argv[2]); /* T2 is the target that gets triggered by T1 */
	// Get the Target Data and Trigger Structures for each target
	tdp1 = xdd_get_target_datap(planp, t1, argv[0]);
	if (tdp1 == NULL) return(-1); 
	trigp = xdd_get_trigp(tdp1);
	if (trigp == NULL) return(-1); 

	tdp2 = xdd_get_target_datap(planp, t2, argv[0]);
	if (tdp2 == NULL) return(-1); 
	  
	trigp->start_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	tdp2->td_target_options |= TO_WAITFORSTART; /* The target that will be triggered has to know to wait */
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->start_trigger_time = (nclk_t)(tmpf * BILLION);
		if (trigp->start_trigger_time <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
			return(0);
		}
		trigp->trigger_types |= TRIGGER_STARTTIME;
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		trigp->start_trigger_op = atoll(argv[4]);
		if (trigp->start_trigger_op <= 0) {
			fprintf(stderr,"%s: Invalid starttrigger op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)trigp->start_trigger_op);
			return(0);
		}
		trigp->trigger_types |= TRIGGER_STARTOP;
		return(5);    
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		trigp->start_trigger_percent = (atof(argv[4]) / 100.0);
		if ((trigp->start_trigger_percent < 0.0) || (trigp->start_trigger_percent > 1.0)) {
			fprintf(stderr,"%s: Invalid starttrigger percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, trigp->start_trigger_percent * 100.0 );
			return(0);
		}
		trigp->trigger_types |= TRIGGER_STARTPERCENT;
		return(5);    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->start_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		trigp->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->start_trigger_bytes = (uint64_t)(tmpf * 1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		trigp->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else {
		fprintf(stderr,"%s: Invalid starttrigger qualifer: %s\n",
				xgp->progname, when);
            return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_stoponerror(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_STOP_ON_ERROR;
    return(1);
}
/*----------------------------------------------------------------------------*/
// See description of "starttrigger" option.
int
xddfunc_stoptrigger(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int 			t1,  t2;			// Target numbers
    target_data_t 	*tdp1;				// Target Data pointers for the target 
    xint_triggers_t	*trigp;				// Trigger Stucture pointers for the this target
    char 			*when;				// The "When" to perform a trigger
    double 			tmpf;				// temp

	  
	if (argc < 5) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	// Get the primary and secondary target numbers and the "when" to stop qualifier
	t1 = atoi(argv[1]);
	t2 = atoi(argv[2]);
    when = argv[3];

	tdp1 = xdd_get_target_datap(planp, t1, argv[0]);  
	if (tdp1 == NULL) return(-1);
	trigp = xdd_get_trigp(tdp1);
	if (trigp == NULL) return(-1); 

	trigp->stop_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->stop_trigger_time = (nclk_t)(tmpf * BILLION);
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		trigp->stop_trigger_op = atoll(argv[4]);
        return(5);
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		trigp->stop_trigger_percent = atof(argv[4]);
        return(5);
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->stop_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
        return(5);
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		trigp->stop_trigger_bytes = (uint64_t)(tmpf * 1024);
        return(5);
	} else {
		fprintf(stderr,"%s: Invalid %s qualifer: %s\n",
				xgp->progname, argv[0], when);
        return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	xgp->global_options |= GO_SYNCIO;
	planp->syncio = atoi(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_target_options |= TO_SYNCWRITE;
		return(3);
	} else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_options |= TO_SYNCWRITE;
				i++;
				tdp = planp->target_datap[i];
			}
		}
	return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify a single target name
int
xddfunc_target(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
	int target_number;
	target_data_t *tdp;

	if (flags & XDD_PARSE_PHASE1) {
		target_number = planp->number_of_targets; // This is the last target + 1 
		tdp = xdd_get_target_datap(planp, target_number, argv[0]); 
		if (tdp == NULL) return(-1);
	
		tdp->td_target_basename = argv[1];
		planp->number_of_targets++; 
	}
	return(2);
}
/*----------------------------------------------------------------------------*/
// The target target directory name
// Arguments to this function look like so:
//     -targtetdir [target #] directory_name
// Where "directory_name" is the name of the directory to use for the targets
//
int
xddfunc_targetdir(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
	    tdp = xdd_get_target_datap(planp, target_number, argv[0]); 
		if (tdp == NULL) return(-1);

	    tdp->td_target_directory = argv[args+1];
        return(args+2);
    } else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_target_directory = argv[args+1];
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
 // -targetin 
int
xddfunc_targetin(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int args=0;

	
//	args = xddfunc_target_inout(argc, argv, flags | XDD_PARSE_TARGET_IN);

    return(args);
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -targetoffset #
// 
int
xddfunc_targetoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	planp->target_offset = atoll(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
// Arguments to this function look like so:
//     -targtets N t1 t2 t3 ... TN
// Where N is the number of targets
//    and t1, t2, t3,... are the target names (i.e. /dev/sd0a)
// The "-targets" option is additive. Each time it is used in a command line
// new targets are added to the list of defined targets for this run.
// The global variable "number_of_targets" is the current number of targets
// that have been defined by previous "-targets"
// If the number of targets, N, is negative (i.e. -3) then there is a single
// target name specified and it is repeated N times (i.e. 3 times). 
// For example, "-targets -3 /dev/hd1" would be the equivalent of 
// "-targets 3 /dev/hd1 /dev/hd1 /dev/hd1". This is just shorthand. 
//
int
xddfunc_targets(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int i,j,k;
	int status;
	target_data_t *tdp;

	if (flags & XDD_PARSE_PHASE1) {
		k = planp->number_of_targets;  // Keep the current number of targets we have
		planp->number_of_targets = atoi(argv[1]); // get the number of targets to add to the list
		if (planp->number_of_targets < 0) { // Set all the target names to the single name specified
			// Check to see if the target name is actually an "option" - this may be a syntax error on the command line
			if (*argv[2] == '-') {
				status = xdd_check_option(argv[2]);
				if (status != 0) {
					fprintf(xgp->errout,"%s: syntax error in '%s' specification: target name cannot be an xdd 'option': %s\n",
						xgp->progname, argv[0], argv[2]);
					return(-1);
				}
			}
			i = 3; // Set for the return value
			planp->number_of_targets *= -1; // make this a positive number 
			planp->number_of_targets += k;  // add in the previous number of targets 
			for (j=k; j<planp->number_of_targets; j++) { // This will add targets to the end of the current list of targets 
				// Call xdd_get_target_datap() for each target and put the same target name in each Target Data
				// Make sure the Target Data for this target exists - if it does not, the xdd_get_target_datap() subroutine will create one
				tdp = xdd_get_target_datap(planp, j, argv[0]);
				if (tdp == NULL) return(-1);
				
				tdp->td_target_basename = argv[2];
				if (strcmp(tdp->td_target_basename,"null") == 0) 
					tdp->td_target_options |= TO_NULL_TARGET;
			} // end of FOR loop that places a single target name on each of the associated Target Data
		} else { // Set all target names to the appropriate name
			i = 2; // start with the third argument  
			planp->number_of_targets += k;  // add in the previous number of targets 
			for (j=k; j<planp->number_of_targets; j++) { // This will add targets to the end of the current list of targets 
				// Make sure the Target Data for this target exists - if it does not, the xdd_get_target_datap() subroutine will create one
				tdp = xdd_get_target_datap(planp, j, argv[0]);
				if (tdp == NULL) return(-1);
				tdp->td_target_basename = argv[i];
				if (strcmp(tdp->td_target_basename,"null") == 0) 
					tdp->td_target_options |= TO_NULL_TARGET;
				i++;
			}
		}
		return(i);
	} else { // Phase 2 we need to figure out how many targets are specified but not to process them
		k = atoi(argv[1]); // get the number of targets to add to the list
		if (k < 0) { // Set all the target names to the single name specified
			i = 3; // Set for the return value
		} else { // Set all target names to the appropriate name
			i = k+2; // start with the third argument
		}
		return(i);
	}
}
/*----------------------------------------------------------------------------*/
// The target start delay function will set the "start delay" time for all
// targets or for a specific target if specified.
// The amount of time to delay a target's start time is calculated by 
// multiplying the specified time delay by the target number.
// For example, assuming four targets have been specified, the option
// "-targetstartdelay 2.1" will set the start times of each target like so:
//   Target#   StartDelay (seconds)
//     0        0.0
//     1        2.1
//     2        4.2
//     3        6.3
//
// ---or - to change the start delay of a single target accordingly:
// "-targetstartdelay target 2 5.3" will set the start delay time of 
// target 2 to a value of 2*5.3=10.6 seconds while leaving the start delay 
// times of the other targets unaltered.
//
// See also: -startdelay (aka xddfunc_startdelay)
//
int
xddfunc_targetstartdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    double start_delay;
    nclk_t start_delay_psec;
    target_data_t *tdp;

	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);
	start_delay = atof(argv[args+1]);
	start_delay_psec = (nclk_t)(start_delay * BILLION); 
	if (start_delay < 0.0) {
		fprintf(xgp->errout,"%s: Target Start Delay time of %f is not valid. The Target Start Delay time must be a number of seconds greater than or equal to 0.00 but less than the remaining life of the sun.\n",
			xgp->progname,start_delay);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);
		tdp->td_start_delay = (double)(start_delay * target_number);
		tdp->td_start_delay_psec = start_delay_psec * target_number;
		return(args+2);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_start_delay = (double)(start_delay * tdp->td_target_number);
				tdp->td_start_delay_psec = start_delay_psec * tdp->td_target_number;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// -targetout 
int
xddfunc_targetout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int args=0;

	
//	args = xddfunc_target_inout(argc, argv, flags | XDD_PARSE_TARGET_OUT);

    return(args);
}
/*----------------------------------------------------------------------------*/
// Specify the throttle type and associated throttle value 
// Arguments: -throttle [target #] bw|ops|var #.#
// 
int
xddfunc_throttle(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    char *what;
    double value;
    target_data_t *tdp;
    int retval;
	xint_throttle_t	*throtp;


    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);
	if (argc < 3) {
		fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-throttle'\n",xgp->progname);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		what = argv[args+1];
		value = atof(argv[args+2]);
        retval = args+3;
    } else { /* All targets */
        tdp = 0;
        what = argv[1];
		value = atof(argv[2]);
        retval = 3;
    }

    if (strcmp(what, "ops") == 0) {/* Throttle the ops/sec */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (tdp) {
			throtp = xdd_get_throtp(tdp);
		    throtp->throttle_type = XINT_THROTTLE_OPS;
            throtp->throttle = value;
        } else  { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					throtp = xdd_get_throtp(tdp);
					throtp->throttle_type = XINT_THROTTLE_OPS;
					throtp->throttle = value;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(retval);
    } else if (strcmp(what, "bw") == 0) {/* Throttle the bandwidth */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (tdp) {
			throtp = xdd_get_throtp(tdp);
		    throtp->throttle_type = XINT_THROTTLE_BW;
            throtp->throttle = value;
        } else { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					throtp = xdd_get_throtp(tdp);
					throtp->throttle_type = XINT_THROTTLE_BW;
					throtp->throttle = value;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(retval);
    } else if (strcmp(what, "delay") == 0) {/* Introduce a delay of # seconds between ops */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle delay of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (tdp) {
			throtp = xdd_get_throtp(tdp);
		    tdp->td_throtp->throttle_type = XINT_THROTTLE_DELAY;
            tdp->td_throtp->throttle = value;
        } else { /* Set option for all targets */
	    	if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					throtp = xdd_get_throtp(tdp);
		    		tdp->td_throtp->throttle_type = XINT_THROTTLE_DELAY;
		    		tdp->td_throtp->throttle = value;
		    		i++;
		    		tdp = planp->target_datap[i];
				}
	    	}
		}
		return(retval);
    } else if (strcmp(what, "var") == 0) { /* Throttle Variance */
	if (NULL != tdp && value <= 0.0) {
		throtp = xdd_get_throtp(tdp);
	    fprintf(xgp->errout,"%s: throttle variance of %5.2f is not valid. throttle variance must be a number greater than 0.00 but less than the throttle value of %5.2f\n",xgp->progname,tdp->td_throtp->throttle_variance,tdp->td_throtp->throttle);
	    return(0);
	}
	else if (value <= 0.0)
	    fprintf(xgp->errout,"%s: Negative throttle variance of %5.2f is not valid.\n",xgp->progname, value);
        if (tdp) {
			throtp = xdd_get_throtp(tdp);
            tdp->td_throtp->throttle_variance = value;
		} else { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					throtp = xdd_get_throtp(tdp);
					tdp->td_throtp->throttle_variance = value;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(retval);
    } else {
		fprintf(xgp->errout,"%s: throttle type of of %s is not valid. throttle type must be \"ops\", \"bw\", or \"var\"\n",xgp->progname,what);
		return(0);
	}
} // End of xddfunc_throttle()
/*----------------------------------------------------------------------------*/
int
xddfunc_timelimit(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    target_data_t *tdp;
	double	time_limit;
	nclk_t	time_limit_ticks;

    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	time_limit = atof(argv[args+1]);
	if (time_limit <= 0.0) {
		fprintf(xgp->errout,"%s: time limit of %f is not valid. The time limit must be a number of seconds greater than 0.00 but less than the remaining life of the sun.\n",xgp->progname,time_limit);
		return(0);
	}
	time_limit_ticks = (nclk_t)(time_limit * BILLION);

	if (target_number >= 0) { /* Set this option value for a specific target */
		tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL) return(-1);

		tdp->td_time_limit = time_limit;
		tdp->td_time_limit_ticks = time_limit_ticks;
        return(args+2);
	} else  { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tdp = planp->target_datap[0];
			i = 0;
			while (tdp) {
				tdp->td_time_limit = time_limit;
				tdp->td_time_limit_ticks = time_limit_ticks;
				i++;
				tdp = planp->target_datap[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timerinfo(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_TIMER_INFO;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timeserver(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    int i;

	if (argc < 3) {
		fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timeserver'\n",xgp->progname);
		return(0);
	}
	i = 1;
	/* At this point "i" points to the ts "option" argument */
	if (strcmp(argv[i], "host") == 0) { /* The host name of the time server */
        i++;
		planp->gts_hostname = argv[i];
		return(i+1);
	} else if (strcmp(argv[i], "port") == 0) { /* The port number to use when talking to the time server host */
        i++;
		planp->gts_port = (in_port_t)atol(argv[i]);
		return(i+1);
	} else if (strcmp(argv[i], "bounce") == 0) { /* Specify the number of times to ping the time server */
        i++;
		planp->gts_bounce = atoi(argv[i]);
		return(i+1);
    } else {
			fprintf(stderr,"%s: Invalid timeserver option %s\n",xgp->progname, argv[i]);
            return(0);
    } /* End of the -timeserver sub options */

}
/*----------------------------------------------------------------------------*/
 // -ts on|off|detailed|summary|oneshot
 //     output filename
int
xddfunc_timestamp(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int i;
	int args, args_index; 
	int target_number;
	target_data_t *tdp;


	args_index = 1;
	args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
	if (args < 0) return(-1);

	if (argc < 2) {
		fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timestamp'\n",xgp->progname);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		args_index += args; 
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is someewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for time stamp option %s\n",
				xgp->progname, target_number, argv[args_index]);
			return(0);
		}
	}
	/* At this point "args_index" indexes to the ts "option" argument */
	if (strcmp(argv[args_index], "on") == 0) { /* set the time stamp reporting option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "off") == 0) { /* Turn off the time stamp reporting option */
		if (target_number >= 0) { 
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options &= ~TS_ON; /* Turn OFF time stamping */
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options &= ~TS_ON; /* Turn OFF time stamping */
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "wrap") == 0) { /* Turn on the TS Wrap option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= TS_WRAP;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= TS_WRAP;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "oneshot") == 0) { /* Turn on the TS Wrap option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= TS_ONESHOT;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= TS_ONESHOT;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "size") == 0) { /* Specify the number of entries in the time stamp buffer */
		if (argc < 3) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timestamp'\n",xgp->progname);
			return(0);
		}
        args_index++;
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_size = atoi(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_size = atoi(argv[args_index]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if ((strcmp(argv[args_index], "triggertime") == 0) || (strcmp(argv[args_index], "tt") == 0)) { /* Specify the time to start time stamping */
		if (argc < 3) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timestamp'\n",xgp->progname);
			return(0);
		}
        args_index++;
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= (TS_ON | TS_TRIGTIME);
			tdp->td_ts_table.ts_trigtime = atoll(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= (TS_ON | TS_TRIGTIME);
					tdp->td_ts_table.ts_trigtime = atoll(argv[args_index]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if ((strcmp(argv[args_index], "triggerop") == 0) || (strcmp(argv[args_index], "to") == 0)) { /* Specify the operation number at which to start time stamping */
		if (argc < 3) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timestamp'\n",xgp->progname);
			return(0);
		}
		args_index++;
        if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= (TS_ON | TS_TRIGOP);
			tdp->td_ts_table.ts_trigop = atoi(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= (TS_ON | TS_TRIGOP);
					tdp->td_ts_table.ts_trigop = atoi(argv[args_index]);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "normalize") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "output") == 0) { /* redirect ts output to "filename" */
		if (argc < 3) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-timestamp'\n",xgp->progname);
			return(0);
		}
		args_index++;
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL | TS_APPEND | TS_DETAILED | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL | TS_APPEND | TS_DETAILED | TS_SUMMARY);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		planp->ts_output_filename_prefix = argv[args_index];
		return(args_index+1);
	} else if (strcmp(argv[args_index], "append") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL | TS_APPEND | TS_DETAILED | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= (TS_ON | TS_ALL | TS_APPEND | TS_DETAILED | TS_SUMMARY);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "dump") == 0) { /* dump a binary stimestamp file to "filename" */
        args_index++;
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		planp->ts_binary_filename_prefix = argv[args_index];
		return(args_index+1);
	} else if (strcmp(argv[args_index], "summary") == 0) { /* set the time stamp SUMMARY reporting option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "detailed") == 0) { /* set the time stamp DETAILED reporting option */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_ts_table.ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	}
    return(args_index+1);
}
/*----------------------------------------------------------------------------*/
 // -verify location | contents
int
xddfunc_verify(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
    int i;
    int args, args_index; 
    int target_number;
    target_data_t *tdp;

    args_index = 1;
    args = xdd_parse_target_number(planp, argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
        args_index += args; 
		if (target_number >= MAX_TARGETS) { /* Make sure the target number is someewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for verify option %s\n",
					xgp->progname, target_number, argv[args_index+1]);
            return(0);
		}
	}
    if (strcmp(argv[args_index], "contents") == 0) { /*  Verify the contents of the buffer */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_target_options |= TO_VERIFY_CONTENTS;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_VERIFY_CONTENTS;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "location") == 0) { /*  Verify the buffer location */
		if (target_number >= 0) {
			tdp = xdd_get_target_datap(planp, target_number, argv[0]);
			if (tdp == NULL) return(-1);
			tdp->td_target_options |= TO_VERIFY_LOCATION;
			tdp->td_dpp->data_pattern_options |= DP_SEQUENCED_PATTERN;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				tdp = planp->target_datap[0];
				i = 0;
				while (tdp) {
					tdp->td_target_options |= TO_VERIFY_LOCATION;
					tdp->td_dpp->data_pattern_options |= DP_SEQUENCED_PATTERN;
					i++;
					tdp = planp->target_datap[i];
				}
			}
		}
		return(args_index+1);
	} else {
		fprintf(stderr,"%s: Invalid verify suboption %s\n",xgp->progname, argv[args_index]);
        return(0);
    } /* End of the -verify sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_unverbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options &= ~GO_VERBOSE;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_verbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_VERBOSE;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_version(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
    fprintf(stdout,"%s: Version %s\n",xgp->progname, PACKAGE_VERSION);
    exit(XDD_RETURN_VALUE_SUCCESS);
}

/*----------------------------------------------------------------------------*/
int
xddfunc_invalid_option(int32_t argc, char *argv[], uint32_t flags)
{
	if (argv[0]) {
			fprintf(xgp->errout, "%s: Invalid option: token number %d: '%s'\n",
				xgp->progname, argc-1, argv[0]);
		} else {
			fprintf(xgp->errout, "%s: Invalid option: option number, token number %d: <option string not available>\n",
				xgp->progname, argc-1);
		}
    return(0);
} // end of xddfunc_invalid_option() 
 
/*----------------------------------------------------------------------------*/
void
xddfunc_currently_undefined_option(char *sp) {
	fprintf(xgp->errout,"The '%s' option is currently undefined\n",sp);
} // End of xddfunc_currently_undefined_option()

/*----------------------------------------------------------------------------*/
// xddfunc_target_inout_file
// Called by xddfunc_target_inout() to process the "file" I/O Type.
// This processes the I/O Type, suboptions, and arguments for that. 
// 
int
xddfunc_target_inout_file(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int args=0;
	// Suboptions include:
	//   name:
	//   dio:
	//   ordering:   loose, serial, none
	// 
    return(args);
}

/*----------------------------------------------------------------------------*/
// xddfunc_target_inout_network
// Called by xddfunc_target_inout() to process the "network" I/O Type.
// This processes the I/O Type, suboptions, and arguments for that. 
// 
int
xddfunc_target_inout_network(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int args=0;
	// Suboptions include:
	//   hostname:   hostname|IP:port#,#ports
	//   protocol:   TCP or UDP
	//   ordering:   loose, serial, none
	// 

    return(args);
}
/*----------------------------------------------------------------------------*/
// xddfunc_target_inout
// Called by xddfunc_targetin() and xddfunc_targetout() option functions.
// This processes the I/O Type, suboptions, and arguments for that. 
// 
int
xddfunc_target_inout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags) 
{
	int args=0;
// Process the I/O Type 
// Within each I/O Type process the suboptions
// Within each suboption process the arguments
//    File
//    Network
//    
//	if (flags & XDD_PARSE_TARGET_FILE) {
//		args = xddfunc_target_inout_file(argc, argv, flags);
//	} else if (flags & XDD_PARSE_TARGET_NETWORK) {
//		args = xddfunc_target_inout_network(argc, argv, flags);
//	} else { 
//		fprintf(xgp->errout, "%s: xddfunc_target_inout: Invalid I/O Type\n",
//			xgp->progname);
//	}
    return(args);
}

int
xddfunc_xni(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int args; 
	int target_number;
	char* xni_mode_str = 0;
	xni_protocol_t* xni_proto = 0;

	args = xdd_parse_target_number(planp, argc, &argv[0],
								   flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	/* Get the XNI Mode */
	xni_mode_str = argv[args + 1];
	if (0 == strcmp(xni_mode_str, "tcp"))
		xni_proto = &xni_protocol_tcp;
	else if (0 == strcmp(xni_mode_str, "ib"))
		xni_proto = &xni_protocol_ib;
	else {
		fprintf(stderr, "Invalid XNI mode: %s\n", xni_mode_str);
		return -1;
	}
	
	/* Enable XNI in the plan */
	planp->plan_options |= PLAN_ENABLE_XNI;
	printf("XNI enabled.\n");
	/* Add the XNI mode to relevant targets */
	if (target_number >= 0) {
		/* Set this option value for a specific target */
		target_data_t *tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL)
			return(-1);
		tdp->xni_pcl = *xni_proto;
		return(args+2);
	} else {
        /* Put this option into all Targets */ 
		if (flags & XDD_PARSE_PHASE2) {
			target_data_t *tdp = planp->target_datap[0];
			int i = 0;
			while (tdp) {
				tdp->xni_pcl = *xni_proto;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
} // End of xddfunc_xni()

int
xddfunc_ibdevice(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags)
{
	int args; 
	int target_number;
	char* ibdev = NULL;

	args = xdd_parse_target_number(planp, argc, &argv[0],
								   flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	/* Get the device name */
	ibdev = argv[args + 1];
	
	/* Set the device name for the relevant targets */
	if (target_number >= 0) {
		/* Set this option value for a specific target */
		target_data_t *tdp = xdd_get_target_datap(planp, target_number, argv[0]);
		if (tdp == NULL)
			return(-1);
		tdp->xni_ibdevice = ibdev;
		return(args+2);
	} else {
        /* Put this option into all Targets */ 
		if (flags & XDD_PARSE_PHASE2) {
			target_data_t *tdp = planp->target_datap[0];
			int i = 0;
			while (tdp) {
				tdp->xni_ibdevice = ibdev;
				i++;
				tdp = planp->target_datap[i];
			}
		}
		return(2);
	}
}

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
