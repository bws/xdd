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
 * This file contains the subroutines necessary to parse the command line
 * arguments  set up all the global and target-specific variables.
 */

#include "xdd.h"

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
// that the PTDSs (per target data structures) are allocated in memory, there is a multi-phase
// approach to applying command line options to each target. 
// Phase 1 is the initial pass through all the command line options in order to determine how many targets there will be
//     and to allocate one PTDS for each of those targets during this phase.
// Phase 2 is when all the command line options and their arguments are actually processed and the associated values are
//     put into the proper PTDS's. This is because some options may be for a specific target whereas other options will
//     be for all the targets. Since Phase 1 allocated all the PTDS's then the options for any target can be set without
//     worrying about whether or not a PTDS has been allocated.
//
// Precedance: The target-specific options have precedance over the global options which have precedance over the default options. 
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
xddfunc_blocksize(int32_t argc, char *argv[], uint32_t flags)
{
    int args,i; 
    int target_number;
	int32_t block_size;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
	
		p->block_size = block_size;
		if (p->block_size <= 0) {
			fprintf(xgp->errout,"%s: blocksize of %d is not valid. blocksize must be a number greater than 0\n",xgp->progname,p->block_size);
			return(0);
		}
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) { 
				p->block_size = block_size;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
} // End of xddfunc_blocksize()
/*----------------------------------------------------------------------------*/
// Specify the number of Bytes to transfer per pass
// Arguments: -bytes [target #] #
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the p->numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_bytes(int32_t argc, char *argv[], uint32_t flags)
{
	int args, i; 
	int target_number;
	ptds_t *p;
	int64_t bytes;

	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
	if (args < 0)
		return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	bytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL)
			return(-1);

		p->bytes = bytes;
		p->numreqs = 0;
		return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->bytes = bytes;
				p->numreqs = 0;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
} // End of xddfunc_bytes()
/*----------------------------------------------------------------------------*/
int
xddfunc_combinedout(int32_t argc, char *argv[], uint32_t flags)
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
// Create new target files for each pass.
int
xddfunc_createnewfiles(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_CREATE_NEW_FILES;
        return(args+1);
    } else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) { 
				p->target_options |= TO_CREATE_NEW_FILES;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(1);
	}
} // End of xddfunc_createnewfiles()
/*----------------------------------------------------------------------------*/
int
xddfunc_csvout(int32_t argc, char *argv[], uint32_t flags)
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
// xdd_get_data_pattern_pointer() - This is a helper function for the
//     xddfunc_datapattern() parsing function. 
//     This function checks to see if there is a valid pointer to the data_pattern
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the data_pattern structure and 
//     set the "ddp" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid data_pattern pointer as well. 
// 
data_pattern_t *
xdd_get_data_pattern_pointer(ptds_t *p)
{
	if (p->dpp == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->dpp = (data_pattern_t *)malloc(sizeof(data_pattern_t));
		if (p->dpp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the data_pattern PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->dpp);
} 
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -datapattern [target #] # <option> 
// 
int
xddfunc_datapattern(int32_t argc, char *argv[], uint32_t flags)
{
	int           args, i; 
	int           target_number;
	ptds_t        *p;
	char          *pattern_type; // The pattern type of ascii, hex, random, ...etc
	unsigned char *pattern; // The ACSII representation of the specified data pattern
	unsigned char *pattern_value; // The ACSII representation of the specified data pattern
	uint64_t      pattern_binary; // The 64-bit value shifted all the way to the left
	size_t        pattern_length; // The length of the pattern string from the command line
	unsigned char *tmpp;
	int           retval;
  
    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	pattern_type = (char *)argv[args+1];
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

        retval = args+2;
    } else {
        p = (ptds_t *)NULL;
        args = 0;
        retval = 2;
    }
	if (strcmp(pattern_type, "random") == 0) {  /* make it random data  */
		if (p)  /* set option for the specific target */
            p->target_options |= TO_RANDOM_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) { 
					p->target_options |= TO_RANDOM_PATTERN;
					i++;
					p = xgp->ptdsp[i];
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
		if (p) { /* set option for specific target */
			xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
			p->target_options |= TO_ASCII_PATTERN;
			p->dpp->data_pattern = (unsigned char *)argv[args+2];
			p->dpp->data_pattern_length = strlen((char *)p->dpp->data_pattern);
		}
		else  {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) { 
					xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
					p->target_options |= TO_ASCII_PATTERN;
					p->dpp->data_pattern = pattern;
					p->dpp->data_pattern_length = pattern_length;
					i++;
					p = xgp->ptdsp[i];
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
		if (pattern_length <= 0) {
			fprintf(xgp->errout, "%s: WARNING: 0-length hex data pattern specified - using 0x00 instead.\n",xgp->progname);
			pattern = (unsigned char *)"\0";
		}
		pattern_value = malloc(((pattern_length+1)/2));
		if (pattern_value == 0) {
			fprintf(xgp->errout, "%s: WARNING: cannot allocate %d bytes for hex data pattern - defaulting to no pattern\n",
				xgp->progname,
				((pattern_length+1)/2));
		}
		memset(pattern_value,0,((pattern_length+1)/2));
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

		if (p) { /* set option for specific target */
			xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
			p->target_options |= TO_HEX_PATTERN;
			p->dpp->data_pattern = pattern_value; // The actual 64-bit value left-justtified
			p->dpp->data_pattern_length = pattern_length; // length in nibbles 
		}
		else  {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) { 
					xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
					p->target_options |= TO_HEX_PATTERN;
					p->dpp->data_pattern = pattern_value; // The actual 64-bit value right-justtified
					p->dpp->data_pattern_length = pattern_length; // length in bytes
					i++;
					p = xgp->ptdsp[i];
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
			fprintf(xgp->errout, "%s: WARNING: Length of data pattern is too long <%d> - truncating to 8 bytes.\n",xgp->progname, pattern_length);
			pattern[16] = '\0';
		}
		pattern_value = malloc(((pattern_length+1)/2));
		if (pattern_value == 0) {
			fprintf(xgp->errout, "%s: WARNING: cannot allocate %d bytes for prefix data pattern - defaulting to no prefix\n",
				xgp->progname,
				((pattern_length+1)/2));
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
		if (p) { /* set option for specific target */
			xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
			p->target_options |= TO_PATTERN_PREFIX;
			p->dpp->data_pattern_prefix = pattern;
			p->dpp->data_pattern_prefix_value = pattern_value; // Pointer to the  N-bit value in BIG endian (left justified)
			p->dpp->data_pattern_prefix_binary = pattern_binary; // The actual 64-bit binary value left-justtified
			p->dpp->data_pattern_prefix_length = pattern_length; // Length in nibbles
		} else  {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) { 
					xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
					p->target_options |= TO_PATTERN_PREFIX;
					p->dpp->data_pattern_prefix = pattern;
					p->dpp->data_pattern_prefix_value = pattern_value; // Pointer to the  N-bit value in BIG endian (left justified)
					p->dpp->data_pattern_prefix_binary = pattern_binary; // The actual 64-bit binary value left-justtified
					p->dpp->data_pattern_prefix_length = pattern_length; // Length in nibbles
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "file") == 0) {
		retval++;
		if (argc <= args+2) {
			fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-datapattern'\n",xgp->progname);
			return(0);
		}
		if (p) {/* set option for specific target */
			xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
			p->target_options |= TO_FILE_PATTERN;
			p->dpp->data_pattern_filename = (char *)argv[args+2];
		} else {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
					p->target_options |= TO_FILE_PATTERN;
					p->dpp->data_pattern_filename = (char *)argv[args+2];
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "sequenced") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_SEQUENCED_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_SEQUENCED_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "inverse") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_INVERSE_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_INVERSE_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strncmp(pattern_type, "replicate", 9) == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_REPLICATE_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_REPLICATE_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "lfpat") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_LFPAT_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_LFPAT_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "ltpat") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_LTPAT_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_LTPAT_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "cjtpat") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_CJTPAT_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_CJTPAT_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "crpat") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_CRPAT_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_CRPAT_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else if (strcmp(pattern_type, "cspat") == 0) {
		if (p) /* set option for specific target */
			p->target_options |= TO_CSPAT_PATTERN;
		else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_CSPAT_PATTERN;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	} else {
		if (p) { /* set option for a specific target */ 
			xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
			p->target_options |= TO_SINGLECHAR_PATTERN;
			p->dpp->data_pattern = (unsigned char *)pattern_type;
		} else {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_data_pattern_pointer(p); // make sure p->dpp exists
					p->target_options |= TO_SINGLECHAR_PATTERN;
					p->dpp->data_pattern = (unsigned char *)pattern_type;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
	}
    return(retval);
} // End of xddfunc_datapattern()

/*----------------------------------------------------------------------------*/
// Set the DEBUG global option
int
xddfunc_debug(int32_t argc, char *argv[], uint32_t flags)
{
		xgp->global_options |= (GO_DEBUG | GO_DEBUG_INIT);
	return(1);
} // End of xddfunc_debug()

/*----------------------------------------------------------------------------*/
// Delete the target file when complete
int
xddfunc_deletefile(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_DELETEFILE;
        return(args+1);
    } else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_DELETEFILE;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(1);
	}
} // End of xddfunc_deletefile()
/*----------------------------------------------------------------------------*/
// Deskew the data rates after a run
int
xddfunc_deskew(int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) {
		xgp->global_options |= GO_DESKEW;
		xgp->deskew_total_bytes = 0;
		xgp->deskew_total_time = 0;
		xgp->deskew_total_rates = 0;
	}
    return(1);
} // End of xddfunc_deskew()
/*----------------------------------------------------------------------------*/
// Arguments: -devicefile [target #] - OBSOLETE
int
xddfunc_devicefile(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_DEVICEFILE;
        return(args+1);
    } else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_DEVICEFILE;
					i++;
					p = xgp->ptdsp[i];
				}
			}
        return(1);
	}
} 
/*----------------------------------------------------------------------------*/
// Specify the use of direct I/O for a single target or for all targets
// Arguments: -dio [target #]
int
xddfunc_dio(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_DIO;
        return(args+1);
    } else {// Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_DIO;
					i++;
					p = xgp->ptdsp[i];
				}
			}
        return(1);
	}
}
/*----------------------------------------------------------------------------*/
// xdd_get_e2e_pointer() - This is a helper function for the
//     xddfunc_endtoend() parsing function. 
//     This function checks to see if there is a valid pointer to the e2e
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the  e2e structure and 
//     set the "e2ep" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid e2e pointer as well. 
// 
e2e_t *
xdd_get_e2e_pointer(ptds_t *p)
{
	if (p->e2ep == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->e2ep = (e2e_t *)malloc(sizeof(e2e_t));
		if (p->e2ep == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the EndToEnd PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->e2ep);
} 
/*----------------------------------------------------------------------------*/
// Specify the end-to-end options for either the source or the destination
// Arguments: -endtoend [target #] 
//				destination <hostname> 
//				port <number>
//				issource
//				isdestination
// 
/*----------------------------------------------------------------------------*/
int
xddfunc_endtoend(int32_t argc, char *argv[], uint32_t flags)
{	
	int i;
	int args, args_index; 
	int target_number;
	ptds_t *p;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for EndToEnd option\n", xgp->progname);
		return(-1);
	}
	args_index = 1;
	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
	if (args < 0) return(-1);

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

	// Check to see if the e2e struct is present in the PTDS and if not, allocate one
	/* At this point "args_index" indexes to the -e2e "option" argument */
	if ((strcmp(argv[args_index], "destination") == 0) ||
	    (strcmp(argv[args_index], "dest") == 0) ||
	    (strcmp(argv[args_index], "host") == 0)) { 
		/* Record the name of the "destination" host name for the -e2e option */
		args_index++;
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			xdd_get_e2e_pointer(p);
			p->target_options |= TO_ENDTOEND;
			p->e2ep->e2e_dest_hostname = argv[args_index];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_e2e_pointer(p);
					p->target_options |= TO_ENDTOEND;
					p->e2ep->e2e_dest_hostname = argv[args_index];
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->global_options |= GO_ENDTOEND;
		return(args_index+1);
	} else if ((strcmp(argv[args_index], "issource") == 0) ||
	    (strcmp(argv[args_index], "issrc") == 0)) { 
		// Set the target option flags that indicates this is the 
		// "source" host name for the -e2e option
		args_index++;
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->target_options |= TO_E2E_SOURCE;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_E2E_SOURCE;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->global_options |= GO_ENDTOEND;
		return(args_index);
	} else if ((strcmp(argv[args_index], "isdestination") == 0) ||
	    (strcmp(argv[args_index], "isdest") == 0)) { 
		// Set the target option flags that indicates this is the 
		// "destination" host name for the -e2e option
		args_index++;
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->target_options |= TO_E2E_DESTINATION;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_E2E_DESTINATION;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->global_options |= GO_ENDTOEND;
		return(args_index);
	} else if (strcmp(argv[args_index], "port") == 0) { /* set the port number to use for -e2e */
		args_index++;
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			xdd_get_e2e_pointer(p);
			p->target_options |= TO_ENDTOEND;
			p->e2ep->e2e_dest_base_port = atoi(argv[args_index]);
			p->e2ep->e2e_dest_port = p->e2ep->e2e_dest_base_port;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_e2e_pointer(p);
					p->target_options |= TO_ENDTOEND;
					p->e2ep->e2e_dest_base_port = atoi(argv[args_index]);
					p->e2ep->e2e_dest_port = p->e2ep->e2e_dest_base_port;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->global_options |= GO_ENDTOEND;
		return(args_index+1);
	} else if (strcmp(argv[args_index], "useUDP") == 0) { /* indicate that -e2e should use UDP instead of the default */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			xdd_get_e2e_pointer(p);
			p->target_options |= TO_ENDTOEND;
			p->e2ep->e2e_useUDP = 1;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_e2e_pointer(p);
					p->target_options |= TO_ENDTOEND;
					p->e2ep->e2e_useUDP = 1;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->global_options |= GO_ENDTOEND;
		return(args_index+1);
	} else {
		fprintf(stderr,"%s: Invalid End-to-End option %s\n",xgp->progname, argv[args_index]);
		return(0);
    }/* End of the -endtoend (e2e or war) sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_errout(int32_t argc, char *argv[], uint32_t flags)
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
xddfunc_extended_stats(int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) 
		xgp->global_options |= GO_EXTENDED_STATS;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Performance a flush (sync) operation every so many write operations
// Arguments: -flushwrite [target #] #
// 
int
xddfunc_flushwrite(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int64_t flushwrite;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	flushwrite = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->flushwrite = flushwrite;

        return(args+2);
	} else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->flushwrite = flushwrite;
					i++;
					p = xgp->ptdsp[i];
				}
			}
        return(2);
	}
} // End of flushwrite()
/*----------------------------------------------------------------------------*/
int
xddfunc_fullhelp(int32_t argc, char *argv[], uint32_t flags)
{
    xdd_usage(1);
    return(-1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_heartbeat(int32_t argc, char *argv[], uint32_t flags)
{
	int heartbeat;

 
	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for heartbeat\n", xgp->progname);
		return(-1);
	}

	if (flags & XDD_PARSE_PHASE2) {
		heartbeat = atoi(argv[1]);
		if (heartbeat <= 0) {
			fprintf(stderr,"%s: Error: Heartbeat value of '%d' cannot be negative\n", xgp->progname, heartbeat);
			return(-1);
		}
		xgp->heartbeat = heartbeat;
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_help(int32_t argc, char *argv[], uint32_t flags)
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
xddfunc_id(int32_t argc, char *argv[], uint32_t flags)
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
// Specify the number of KBytes to transfer per pass (1K=1024 bytes)
// Arguments: -kbytes [target #] #
// This will set p->bytes to the calculated value (kbytes * 1024)
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the p->numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_kbytes(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int64_t kbytes;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	kbytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
		p->bytes = kbytes * 1024;
		p->numreqs = 0;
        return(args+2);
	} else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->bytes = kbytes * 1024;
					p->numreqs = 0;
					i++;
					p = xgp->ptdsp[i];
				}
			}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// xdd_get_lockstep_pointer() - This is a helper function for the
//     xddfunc_lockstep() parsing function. 
//     This function checks to see if there is a valid pointer to the lockstep
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the  lockstep structure and 
//     set the "lsp" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid lockstep pointer as well. 
// 
lockstep_t *
xdd_get_lockstep_pointer(ptds_t *p)
{
	if (p->lsp == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->lsp = (lockstep_t *)malloc(sizeof(lockstep_t));
		if (p->lsp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the EndToEnd PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->lsp);
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
xddfunc_lockstep(int32_t argc, char *argv[], uint32_t flags)
{
    int mt,st;
    double tmpf;
    ptds_t *masterp, *slavep;
    int retval;
    int lsmode;
    char *when, *what, *lockstep_startup, *lockstep_completion;

	if ((strcmp(argv[0], "-lockstep") == 0) ||
		(strcmp(argv[0], "-ls") == 0))
		lsmode = TO_LOCKSTEP;
	else lsmode = TO_LOCKSTEPOVERLAPPED;

	mt = atoi(argv[1]); /* T1 is the master target */
	st = atoi(argv[2]); /* T2 is the slave target */
	/* Sanity checks on the target numbers */
	masterp = xdd_get_ptdsp(mt, argv[0]);
	if (masterp == NULL) return(-1);
	slavep = xdd_get_ptdsp(st, argv[0]);
	if (slavep == NULL) return(-1);
	
	/* Both the master and the slave must know that they are in lockstep. */
	masterp->target_options |= lsmode;
	slavep->target_options |= lsmode; 

	xdd_get_lockstep_pointer(masterp);
	xdd_get_lockstep_pointer(slavep);
	masterp->lsp->ls_slave = st; /* The master has to know the number of the slave target */
	slavep->lsp->ls_master = mt; /* The slave has to know the target number of its master */
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->lsp->ls_interval_type = LS_INTERVAL_TIME;
		masterp->lsp->ls_interval_units = "SECONDS";
		masterp->lsp->ls_interval_value = (pclk_t)(tmpf * TRILLION);
		if (masterp->lsp->ls_interval_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval = 5;
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		masterp->lsp->ls_interval_value = atoll(argv[4]);
		masterp->lsp->ls_interval_type = LS_INTERVAL_OP;
		masterp->lsp->ls_interval_units = "OPERATIONS";
		if (masterp->lsp->ls_interval_value <= 0) {
			fprintf(stderr,"%s: Invalid lockstep interval op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)masterp->lsp->ls_interval_value);
            return(0);
		}
		retval = 5;  
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		masterp->lsp->ls_interval_value = (uint64_t)(atof(argv[4]) / 100.0);
		masterp->lsp->ls_interval_type = LS_INTERVAL_PERCENT;
		masterp->lsp->ls_interval_units = "PERCENT";
		if ((masterp->lsp->ls_interval_value < 0.0) || (masterp->lsp->ls_interval_value > 1.0)) {
			fprintf(stderr,"%s: Invalid lockstep interval percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, atof(argv[4]) );
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->lsp->ls_interval_value = (uint64_t)(tmpf * 1024*1024);
		masterp->lsp->ls_interval_type = LS_INTERVAL_BYTES;
		masterp->lsp->ls_interval_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep interval mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		retval = 5;    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		masterp->lsp->ls_interval_value = (uint64_t)(tmpf * 1024);
		masterp->lsp->ls_interval_type = LS_INTERVAL_BYTES;
		masterp->lsp->ls_interval_units = "BYTES";
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
		slavep->lsp->ls_task_type = LS_TASK_TIME;
		slavep->lsp->ls_task_units = "SECONDS";
		slavep->lsp->ls_task_value = (pclk_t)(tmpf * TRILLION);
		if (slavep->lsp->ls_task_value <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
            return(0);
		};
		retval += 2;
	} else if (strcmp(what,"op") == 0){ /* get the number of operations to execute per task */
		slavep->lsp->ls_task_value = atoll(argv[6]);
		slavep->lsp->ls_task_type = LS_TASK_OP;
		slavep->lsp->ls_task_units = "OPERATIONS";
		if (slavep->lsp->ls_task_value <= 0) {
			fprintf(stderr,"%s: Invalid lockstep task op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)slavep->lsp->ls_task_value);
            return(0);
		}
		retval += 2;       
	} else if (strcmp(what,"mbytes") == 0){ /* get the number of megabytes to transfer per task */
		tmpf = atof(argv[6]);
		slavep->lsp->ls_task_value = (uint64_t)(tmpf * 1024*1024);
		slavep->lsp->ls_task_type = LS_TASK_BYTES;
		slavep->lsp->ls_task_units = "BYTES";
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid lockstep task mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
        return(0);
		}
		retval += 2;     
	} else if (strcmp(what,"kbytes") == 0){ /* get the number of kilobytes to transfer per task */
		tmpf = atof(argv[6]);
		slavep->lsp->ls_task_value = (uint64_t)(tmpf * 1024);
		slavep->lsp->ls_task_type = LS_TASK_BYTES;
		slavep->lsp->ls_task_units = "BYTES";
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
		slavep->lsp->ls_ms_state |= LS_SLAVE_RUN_IMMEDIATELY;
		slavep->lsp->ls_ms_state &= ~LS_SLAVE_WAITING;
		slavep->lsp->ls_task_counter = 1;
	} else { /* Have the slave wait for the master to tell it to run */
		slavep->lsp->ls_ms_state &= ~LS_SLAVE_RUN_IMMEDIATELY;
		slavep->lsp->ls_ms_state |= LS_SLAVE_WAITING;
		slavep->lsp->ls_task_counter = 0;
	}
    retval++;
	lockstep_completion = argv[8];
	if (strcmp(lockstep_completion,"complete") == 0) { /* Have slave complete all operations if master finishes first */
		slavep->lsp->ls_ms_state |= LS_SLAVE_COMPLETE;
	} else if (strcmp(lockstep_completion,"stop") == 0){ /* Have slave stop when master stops */
		slavep->lsp->ls_ms_state |= LS_SLAVE_STOP;
	} else {
		fprintf(stderr,"%s: Invalid lockstep slave completion directive: %s. This value must be either 'complete' or 'stop'\n",
				xgp->progname,lockstep_completion);
        return(0);
    }
    retval++;

    return(retval);
 }
/*----------------------------------------------------------------------------*/
// Set the maxpri and process lock
int
xddfunc_maxall(int32_t argc, char *argv[], uint32_t flags)
{
    int status;

	if (flags & XDD_PARSE_PHASE2) {
		status = xddfunc_maxpri(1,0, flags);
		status += xddfunc_processlock(1,0, flags);
		if (status < 2)
			return(-1);
	}
	return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the maximum number of errors to tolerate before exiting
int
xddfunc_maxerrors(int32_t argc, char *argv[], uint32_t flags)
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
xddfunc_max_errors_to_print(int32_t argc, char *argv[], uint32_t flags)
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
xddfunc_maxpri(int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_MAXPRI;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of MBytes to transfer per pass (1M=1024*1024 bytes)
// Arguments: -mbytes [target #] #
// This will set p->bytes to the calculated value (mbytes * 1024*1024)
// The -bytes/-kbytes/-mbytes option is mutually exclusive with the -numreqs option 
// and will reset the p->numreqs to 0 for all affected targets. The number of 
// requests will be calculated at a later time.
// 
int
xddfunc_mbytes(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int64_t mbytes;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	mbytes = atoll(argv[args+1]);
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->bytes = mbytes * 1024 * 1024;
		p->numreqs = 0;
        return(args+2);
	} else { // Put this option into all PTDSs 
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->bytes = mbytes * 1024 * 1024;
					p->numreqs = 0;
					i++;
					p = xgp->ptdsp[i];
				}
			}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_memalign(int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
	int32_t align;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);


	align = atoi(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->mem_align = align;
		return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) { 
				p->mem_align = align;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
} 
/*----------------------------------------------------------------------------*/
// Set the  no mem lock and no proc lock flags 
int
xddfunc_minall(int32_t argc, char *argv[], uint32_t flags)
{
    int status;
 
	if (flags & XDD_PARSE_PHASE2) {	
		status = xddfunc_nomemlock(1,0,flags);
		status += xddfunc_noproclock(1,0,flags);
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
xddfunc_multipath(int32_t argc, char *argv[], uint32_t flags)
{
	fprintf(stderr,"multipath not implemented\n");
	return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_nobarrier(int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOBARRIER;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the no memory lock flag
int
xddfunc_nomemlock(int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOMEMLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Set the no process lock flag
int
xddfunc_noproclock(int32_t argc, char *argv[], uint32_t flags)
{ 
	if (flags & XDD_PARSE_PHASE2)
		xgp->global_options |= GO_NOPROCLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// Specify the number of requests to run 
// Arguments: -numreqs [target #] #
// This will set p->numreqs to the specified value 
// The -numreqs option is mutually exclusive with the -bytes/-kbytes/-mbytes options
// and will reset the p->bytes to 0 for all affected targets. The number of 
// bytes to transfer will be calculated at a later time.
// 
int
xddfunc_numreqs(int32_t argc, char *argv[], uint32_t flags)
{
	int 		args, i; 
	int 		target_number;
	ptds_t 		*p;
	int64_t 	numreqs;
	
	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
		p->numreqs = numreqs;
		p->bytes = 0; // reset p->bytes
		return(args+2);

	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->numreqs = numreqs;
				p->bytes = 0; // reset p->bytes
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Seconds to delay between individual operations
int
xddfunc_operationdelay(int32_t argc, char *argv[], uint32_t flags)
{ 
    int 	args, i; 
    int 	target_number;
    ptds_t 	*p;
	int 	operationdelay;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
		p->op_delay = operationdelay;
        return(args+2);

	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->op_delay = operationdelay;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the operation to perform - read or write 
// Arguments: -op [target #] read|write
int
xddfunc_operation(int32_t argc, char *argv[], uint32_t flags)
{
    int 	args, i; 
    int 	target_number;
    ptds_t 	*p;
	double 	rwratio;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);


	if ((strcmp(argv[args+1], "write") == 0) || 
		(strcmp(argv[args+1], "writeverify") == 0) || 
		(strcmp(argv[args+1], "writev") == 0))
		 rwratio = 0.0; /* all write operations */
	else rwratio = 1.0; /* all read operations */

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->rwratio = rwratio;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->rwratio = rwratio;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_output(int32_t argc, char *argv[], uint32_t flags)
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
xddfunc_output_format(int32_t argc, char *argv[], uint32_t flags)
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
			xdd_results_format_id_add(argv[2]);
			return(3);
		}
		if (strcmp(argv[1], "new") == 0) {
			if (argc <= 2) {
				fprintf(stderr,"%s: Error: No format string specified for '-outputformat new' option\n", xgp->progname);
				return(-1);
			}
			xgp->format_string = argv[2];
			return(3);
		}
	}
    return(3);
}
/*----------------------------------------------------------------------------*/
// Seconds to delay between passes
int
xddfunc_passdelay(int32_t argc, char *argv[], uint32_t flags)
{ 
	int passdelay;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for pass delay\n", xgp->progname);
		return(-1);
	}
	if (flags & XDD_PARSE_PHASE2) {
		passdelay = atoi(argv[1]);
		if (passdelay <= 0) {
			fprintf(stderr,"%s: Error: Pass delay value of '%d' cannot be negative\n", xgp->progname, passdelay);
			return(-1);
		}
		xgp->pass_delay = passdelay;
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_passes(int32_t argc, char *argv[], uint32_t flags)
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
		xgp->passes = passes;
	}
    return(2);
}

/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks between passes
// Arguments: -passoffset [target #] #
// 
int
xddfunc_passoffset(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int64_t pass_offset;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	pass_offset = atoll(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->pass_offset = pass_offset;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->pass_offset = pass_offset;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
    } 
}
/*----------------------------------------------------------------------------*/
// This defines the meaning of the Percent CPU values that are displayed.
int
xddfunc_percentcpu(int32_t argc, char *argv[], uint32_t flags)
{
    int args; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);


	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		/*
        if ( strcmp(argv[args+1], "absolute") == 0) 
            p->target_options = TO_PCPU_ABSOLUTE; // Make sure 'absolute' percent CPU is on
        else if (strcmp(argv[args+1], "relative") == 0)
            p->target_options &= ~TO_PCPU_ABSOLUTE; // For "relative" percent CPU, turn off 'absolute" percent CPU
        else {
            fprintf(xgp->errout,"%s: Error: The percent CPU specification of '%s' is not valid. It should be either 'absolute' or 'relative'. Defaulting to 'absolute'\n",xgp->progname, argv[args+1]);
            p->target_options = TO_PCPU_ABSOLUTE; // Make sure 'absolute' percent CPU is on
        }
		*/
		return(args+2);
    } else { 
        return(2);
    }
}
/*----------------------------------------------------------------------------*/
int
xddfunc_preallocate(int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    ptds_t *p;
	int32_t preallocate;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	preallocate = atoi(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->preallocate = preallocate;
		return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->preallocate = preallocate;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Lock the process in memory
int
xddfunc_processlock(int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) 
		xgp->global_options |= GO_PLOCK;
    return(1);
}
/*----------------------------------------------------------------------------*/
// processor/target assignment
int
xddfunc_processor(int32_t argc, char *argv[], uint32_t flags)
{
	int32_t args, i; 
	int32_t cpus;
	int32_t processor_number;
	int target_number;
	ptds_t *p;


	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
			p->processor = processor_number;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {	
				p->processor = processor_number;
				i++;
				p = xgp->ptdsp[i];
			}
		}
	return(2);
	} 
}
/*----------------------------------------------------------------------------*/
int
xddfunc_queuedepth(int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    ptds_t *p;
	int32_t queue_depth;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	queue_depth = atoi(argv[args+1]);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->queue_depth = queue_depth;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->queue_depth = queue_depth;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_qthreadinfo(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_QTHREAD_INFO;
        return(args+1);
    }else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_QTHREAD_INFO;
				i++;
				p = xgp->ptdsp[i];
			}
		}
	return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify that the seek list should be randomized between passes
// Arguments: -randomize [target #]

int
xddfunc_randomize(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_PASS_RANDOMIZE;
        return(args+1);
    } else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_PASS_RANDOMIZE;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(1);
	}
}
/*----------------------------------------------------------------------------*/
// xdd_get_raw_pointer() - This is a helper function for the
//     xddfunc_readafterwrite() parsing function. 
//     This function checks to see if there is a valid pointer to the raw
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the  raw structure and 
//     set the "rawp" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid raw pointer as well. 
// 
raw_t *
xdd_get_raw_pointer(ptds_t *p)
{
	if (p->rawp == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->rawp = (raw_t *)malloc(sizeof(raw_t));
		if (p->rawp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the EndToEnd PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->rawp);
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
xddfunc_readafterwrite(int32_t argc, char *argv[], uint32_t flags)
{
    int     i;
    int     args; 
    int     target_number;
    ptds_t  *p;

	i = 1;
    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		if (target_number >= MAX_TARGETS) { /* Make sure the target number is somewhat valid */
			fprintf(stderr,"%s: Invalid Target Number %d specified for read-after-write option %s\n",
					xgp->progname, target_number, argv[i+2]);
			return(0);
		}
		i += args, i;  /* skip past the "target <taget number>" */
	}
	/* At this point "i" points to the raw "option" argument */
	if (strcmp(argv[i], "trigger") == 0) { /* set the the trigger type */
		if (target_number < 0) {  /* set option for all targets */
//			xgp->global_ptds.target_options |= TO_READAFTERWRITE;
//			if (strcmp(argv[i+1], "stat") == 0)
//				xgp->global_ptds.raw_trigger |= PTDS_RAW_STAT;
//			else if (strcmp(argv[i+1], "mp") == 0)
//				xgp->global_ptds.raw_trigger |= PTDS_RAW_MP;
//			else {
//				fprintf(stderr,"%s: Invalid trigger type specified for read-after-write option: %s\n",
//					xgp->progname, argv[i+1]);
				return(0);
//			}
		} else {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_raw_pointer(p); // Make sure there is a valid p->rawp pointer
			p->target_options |= TO_READAFTERWRITE;
			if (strcmp(argv[i+1], "stat") == 0)
					p->rawp->raw_trigger |= PTDS_RAW_STAT;
				else if (strcmp(argv[i+1], "mp") == 0)
					p->rawp->raw_trigger |= PTDS_RAW_MP;
				else {
					fprintf(stderr,"%s: Invalid trigger type specified for read-after-write option: %s\n",
						xgp->progname, argv[i+1]);
					return(0);
				}
		}
        return(i+2);
	} else if (strcmp(argv[i], "lag") == 0) { /* set the lag block count */
		if (target_number < 0) {
			for (target_number=0; target_number<MAX_TARGETS; target_number++) {  /* set option for all targets */
			}
		} else {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_raw_pointer(p); // Make sure there is a valid p->rawp pointer
			p->target_options |= TO_READAFTERWRITE;
			p->rawp->raw_lag = atoi(argv[i+1]);
		}
        return(i+2);
	} else if (strcmp(argv[i], "reader") == 0) { /* hostname of the reader for this read-after-write */
		/* This assumes that these targets are all writers and need to know who the reader is */
		if (target_number < 0) {
		} else {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_raw_pointer(p); // Make sure there is a valid p->rawp pointer
			p->target_options |= TO_READAFTERWRITE;
			p->rawp->raw_hostname = argv[i+1];
		}
        return(i+2);
	} else if (strcmp(argv[i], "port") == 0) { /* set the port number for the socket used by the writer */
		if (target_number < 0) {
		} else {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_raw_pointer(p); // Make sure there is a valid p->rawp pointer
			p->target_options |= TO_READAFTERWRITE;
			p->rawp->raw_port = atoi(argv[i+1]);
		}
        return(i+2);
	} else {
			fprintf(stderr,"%s: Invalid Read-after-write option %s\n",xgp->progname, argv[i+1]);
            return(0);
	}/* End of the -readafterwrite (raw) sub options */
}
/*----------------------------------------------------------------------------*/
int
xddfunc_reallyverbose(int32_t argc, char *argv[], uint32_t flags)
{
	if (flags & XDD_PARSE_PHASE2) 
		xgp->global_options |= GO_REALLYVERBOSE;
    return(1);
}

/*----------------------------------------------------------------------------*/
// Re-create the target file between each pass
int
xddfunc_recreatefiles(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

	    p->target_options |= TO_RECREATE;
        return(args+1);
    } else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_RECREATE;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(1);
	}
}

/*----------------------------------------------------------------------------*/
// Re-open the target file between each pass
int
xddfunc_reopen(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

	    p->target_options |= TO_REOPEN;
        return(args+1);
    } else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_REOPEN;
				i++;
				p = xgp->ptdsp[i];
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
xddfunc_report_threshold(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
    double threshold;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	threshold = atof(argv[args+1]);
	if (threshold < 0.0) {
		fprintf(xgp->errout,"%s: report threshold of %5.2f is not valid. rwratio must be a positive number\n",xgp->progname,threshold);
        return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->report_threshold = (pclk_t)(threshold * TRILLION);
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->report_threshold = (pclk_t)(threshold * TRILLION);
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the I/O request size in blocks for either a single target or all targets 
// Arguments: -reqsize [target #] #
int
xddfunc_reqsize(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int32_t reqsize;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	reqsize = atoi(argv[args+1]);
	if (reqsize <= 0) {
		fprintf(xgp->errout,"%s: reqsize of %d is not valid. reqsize must be a number greater than 0\n",xgp->progname,reqsize);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->reqsize = reqsize;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->reqsize = reqsize;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Set the retry count for each target. The retry count gets inherited by any
// nsubsequent queue threads for the target.
int
xddfunc_retry(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
	int32_t retry_count;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	retry_count = atoi(argv[args+1]);
	if (retry_count <= 0) {
		fprintf(xgp->errout,"%s: Retry count of %d is not valid. Retry count must be a number greater than 0\n",xgp->progname,retry_count);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->retry_count = retry_count;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->retry_count = retry_count;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
// round robin processor target assignment
int
xddfunc_roundrobin(int32_t argc, char *argv[], uint32_t flags)
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
		p = xdd_get_ptdsp(j, argv[0]);
		if (p == NULL) return(-1);

		p->processor = k;
		k++;
		if (k >= processor_number)
			k = 0;
	}
	*/
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_runtime(int32_t argc, char *argv[], uint32_t flags)
{
	int	runtime;


	if (argc <= 1) {
		fprintf(stderr,"%s: Error: No value specified for run time\n", xgp->progname);
		return(-1);
	}

	if (flags & XDD_PARSE_PHASE2) {
		runtime = atoi(argv[1]);
		if (runtime <= 0) {
			fprintf(stderr,"%s: Error: Runtime value of '%d' cannot be negative\n", xgp->progname, runtime);
			return(-1);
		}
		xgp->runtime = runtime;
	}
    return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the read/write ratio for either a single target or all targets 
// Arguments: -rwratio [target #] #.#
int
xddfunc_rwratio(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;
    double rwratio;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	rwratio = (atof(argv[args+1]) / 100.0);
	if ((rwratio < 0.0) || (rwratio > 1.0)) {
		fprintf(xgp->errout,"%s: rwratio of %5.2f is not valid. rwratio must be a number between 0.0 and 100.0\n",xgp->progname,rwratio);
        return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->rwratio = rwratio;
        return(args+2);
	} else { // Put this option into all PTDSs 
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->rwratio = rwratio;
				i++;
				p = xgp->ptdsp[i];
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
xddfunc_seek(int32_t argc, char *argv[], uint32_t flags)
{
    int     i;
    int     args, args_index; 
    int     target_number;
    ptds_t  *p;

	args_index = 1;
    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_SAVE;
			p->seekhdr.seek_savefile = argv[args_index+1];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_SAVE;
					p->seekhdr.seek_savefile = argv[args_index+1];
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+2);
	} else if (strcmp(argv[args_index], "load") == 0) { /* load seek list from "filename" */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_LOAD;
			p->seekhdr.seek_loadfile = argv[args_index+1];
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_LOAD;
					p->seekhdr.seek_loadfile = argv[args_index+1];
					i++;
					p = xgp->ptdsp[i];
				}
			}
		} 
		return(args_index+2);
	} else if (strcmp(argv[args_index], "disthist") == 0) { /*  Print a Distance Histogram */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_DISTHIST;
			p->seekhdr.seek_NumDistHistBuckets = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_DISTHIST;
					p->seekhdr.seek_NumDistHistBuckets = atoi(argv[args_index+1]);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "seekhist") == 0) { /* Print a Seek Histogram */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_SEEKHIST;
			p->seekhdr.seek_NumSeekHistBuckets = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_SEEKHIST;
					p->seekhdr.seek_NumSeekHistBuckets = atoi(argv[args_index+1]);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		} 
		return(args_index+2);
	} else if (strcmp(argv[args_index], "sequential") == 0) { /*  Sequential seek list option */     
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options &= ~SO_SEEK_RANDOM;
			p->seekhdr.seek_pattern = "sequential";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options &= ~SO_SEEK_RANDOM;
					p->seekhdr.seek_pattern = "sequential";
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+1);
	} else if (strcmp(argv[args_index], "random") == 0) { /*  Random seek list option */     
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_RANDOM;
			p->seekhdr.seek_pattern = "random";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_RANDOM;
					p->seekhdr.seek_pattern = "random";
					i++;
					p = xgp->ptdsp[i];
				}
			}
		} 
		return(args_index+1);
	} else if (strcmp(argv[args_index], "stagger") == 0) { /*  Staggered seek list option */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_STAGGER;
			p->seekhdr.seek_pattern = "staggered";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_STAGGER;
					p->seekhdr.seek_pattern = "staggered";
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+1);
	} else if (strcmp(argv[args_index], "interleave") == 0) { /* set the interleave for sequential seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_interleave = atoi(argv[args_index+1]);
			p->seekhdr.seek_pattern = "interleaved";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_interleave = atoi(argv[args_index+1]);
					p->seekhdr.seek_pattern = "interleaved";
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "none") == 0) { /* no seeking at all */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_options |= SO_SEEK_NONE;
			p->seekhdr.seek_pattern = "none";
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_options |= SO_SEEK_NONE;
					p->seekhdr.seek_pattern = "none";
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+1);
	} else if (strcmp(argv[args_index], "range") == 0) { /* set the range of seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_range = atoll(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_range = atoll(argv[args_index+1]);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}  
		return(args_index+2);
	} else if (strcmp(argv[args_index], "seed") == 0) { /* set the seed for random seek locations */
		if (target_number >= 0) {  /* set option for specific target */
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->seekhdr.seek_seed = atoi(argv[args_index+1]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->seekhdr.seek_seed = atoi(argv[args_index+1]);
					i++;
					p = xgp->ptdsp[i];
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
int
xddfunc_setup(int32_t argc, char *argv[], uint32_t flags)
{
	int status;
	
	if (flags & XDD_PARSE_PHASE1) {
		status = xdd_process_paramfile(argv[1]);
		if (status == 0)
			return(0);
	}
	return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the use of SCSI Generic I/O for a single target or for all targets
// Arguments: -sgio [target #]
int
xddfunc_sgio(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_SGIO;
        return(args+1);
    } else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_SGIO;
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(1);
    }
}
/*----------------------------------------------------------------------------*/
int
xddfunc_sharedmemory(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;


    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

    // At this point the "target_number" is valid
	if (target_number >= 0) { /* Request size for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_SHARED_MEMORY;
        return(args+1);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_SHARED_MEMORY;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(1);
	}
}
/*----------------------------------------------------------------------------*/	
// single processor scheduling
int
xddfunc_singleproc(int32_t argc, char *argv[], uint32_t flags) 
{
    int32_t cpus;
    int32_t processor_number;
    int32_t args, i; 
    int target_number;
    ptds_t *p;


	cpus = xdd_cpu_count();
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	processor_number = atoi(argv[args+1]);
	if ((processor_number < 0) || (processor_number >= cpus)) {
		fprintf(xgp->errout,"%s: Error: Processor number <%d> is out of range\n",xgp->progname, processor_number);
		fprintf(xgp->errout,"%s:     Processor number should be between 0 and %d\n",xgp->progname, cpus-1);
		return(0);
	}

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);
		p->processor = processor_number;
		return(args+1);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->processor = processor_number;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_startdelay(int32_t argc, char *argv[], uint32_t flags)
{ 
    int args, i; 
    int target_number;
    double tmpf;
    ptds_t *p;

		
	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		tmpf = atof(argv[args+1]);
		p->start_delay = (pclk_t)(tmpf * TRILLION);
		return(args+2);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			tmpf = atof(argv[args+1]);
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->start_delay = (pclk_t)(tmpf * TRILLION);
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(args+1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -startoffset [target #] #
// 
int
xddfunc_startoffset(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
	int64_t	start_offset;
    ptds_t *p;

	
	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	start_offset = atoll(argv[args+1]);
	if (start_offset < 0) {
		fprintf(xgp->errout,"%s: start offset of %d is not valid. start offset must be a number equal to or greater than 0\n",xgp->progname,start_offset);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->start_offset = start_offset;
        return(args+2);
	} else {/* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->start_offset = start_offset;
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_starttime(int32_t argc, char *argv[], uint32_t flags)
{ 
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	xgp->gts_time = (pclk_t) atoll(argv[1]);
	xgp->gts_time *= TRILLION;
    return(2);
}
/*----------------------------------------------------------------------------*/
// xdd_get_trigger_pointer() - This is a helper function for the
//     xddfunc_starttrigger() and xddfunc_stoptrigger parsing functions.
//     This function checks to see if there is a valid pointer to the trigger
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the  trigger structure and 
//     set the "tgp" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid trigger pointer as well. 
// 
trigger_t *
xdd_get_trigger_pointer(ptds_t *p)
{
	if (p->tgp == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->tgp = (trigger_t *)malloc(sizeof(trigger_t));
		if (p->tgp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the EndToEnd PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->tgp);
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
xddfunc_starttrigger(int32_t argc, char *argv[], uint32_t flags)
{
    int t1,t2;
    double tmpf;
    ptds_t *p1, *p2;
    char *when;

		  
	if (argc < 5) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}

	t1 = atoi(argv[1]); /* T1 is the target that does the triggering */
	t2 = atoi(argv[2]); /* T2 is the target that gets triggered by T1 */
	/* Sanity checks on the target numbers */
	p1 = xdd_get_ptdsp(t1, argv[0]);
	if (p1 == NULL) return(-1); 
	p2 = xdd_get_ptdsp(t2, argv[0]);
	if (p2 == NULL) return(-1); 
	  
	// Make sure the PTDS for p1 and p2 have valid trigger structure pointers
	xdd_get_trigger_pointer(p1);
	xdd_get_trigger_pointer(p2);
	p1->tgp->start_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	p2->target_options |= TO_WAITFORSTART; /* The target that will be triggered has to know to wait */
	when = argv[3];

	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->start_trigger_time = (pclk_t)(tmpf * TRILLION);
		if (p1->tgp->start_trigger_time <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger time: %f. This value must be greater than 0.0\n",
				xgp->progname, tmpf);
			return(0);
		}
		p1->tgp->trigger_types |= TRIGGER_STARTTIME;
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		p1->tgp->start_trigger_op = atoll(argv[4]);
		if (p1->tgp->start_trigger_op <= 0) {
			fprintf(stderr,"%s: Invalid starttrigger op: %lld. This value must be greater than 0\n",
				xgp->progname, 
				(long long)p1->tgp->start_trigger_op);
			return(0);
		}
		p1->tgp->trigger_types |= TRIGGER_STARTOP;
		return(5);    
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		p1->tgp->start_trigger_percent = (atof(argv[4]) / 100.0);
		if ((p1->tgp->start_trigger_percent < 0.0) || (p1->tgp->start_trigger_percent > 1.0)) {
			fprintf(stderr,"%s: Invalid starttrigger percent: %f. This value must be between 0.0 and 100.0\n",
				xgp->progname, p1->tgp->start_trigger_percent * 100.0 );
			return(0);
		}
		p1->tgp->trigger_types |= TRIGGER_STARTPERCENT;
		return(5);    
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->start_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger mbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		p1->tgp->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->start_trigger_bytes = (uint64_t)(tmpf * 1024);
		if (tmpf <= 0.0) {
			fprintf(stderr,"%s: Invalid starttrigger kbytes: %f. This value must be greater than 0\n",
				xgp->progname,tmpf);
            return(0);
		}
		p1->tgp->trigger_types |= TRIGGER_STARTBYTES;
		return(5);    
	} else {
		fprintf(stderr,"%s: Invalid starttrigger qualifer: %s\n",
				xgp->progname, when);
            return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_stoponerror(int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_STOP_ON_ERROR;
    return(1);
}
/*----------------------------------------------------------------------------*/
// See description of "starttrigger" option.
int
xddfunc_stoptrigger(int32_t argc, char *argv[], uint32_t flags)
{
    int t1,t2;
    double tmpf;
    ptds_t *p1;
    char *when;

	  
	if (argc < 5) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	// Get the primary and secondary target numbers and the "when" to stop qualifier
	t1 = atoi(argv[1]);
	t2 = atoi(argv[2]);
    when = argv[3];

	p1 = xdd_get_ptdsp(t1, argv[0]);  
	if (p1 == NULL) return(-1);
	// Make sure the PTDS for p1 has a valid trigger structure pointer
	xdd_get_trigger_pointer(p1);

	p1->tgp->stop_trigger_target = t2; /* The target that does the triggering has to 
									* know the target number to trigger */
	if (strcmp(when,"time") == 0){ /* get the number of seconds to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->stop_trigger_time = (pclk_t)(tmpf * TRILLION);
        return(5);
	} else if (strcmp(when,"op") == 0){ /* get the number of operations to wait before triggering the other target */
		p1->tgp->stop_trigger_op = atoll(argv[4]);
        return(5);
	} else if (strcmp(when,"percent") == 0){ /* get the percentage of operations to wait before triggering the other target */
		p1->tgp->stop_trigger_percent = atof(argv[4]);
        return(5);
	} else if (strcmp(when,"mbytes") == 0){ /* get the number of megabytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->stop_trigger_bytes = (uint64_t)(tmpf * 1024*1024);
        return(5);
	} else if (strcmp(when,"kbytes") == 0){ /* get the number of kilobytes to wait before triggering the other target */
		tmpf = atof(argv[4]);
		p1->tgp->stop_trigger_bytes = (uint64_t)(tmpf * 1024);
        return(5);
	} else {
		fprintf(stderr,"%s: Invalid %s qualifer: %s\n",
				xgp->progname, argv[0], when);
        return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncio(int32_t argc, char *argv[], uint32_t flags)
{
	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}
	xgp->global_options |= GO_SYNCIO;
	xgp->syncio = atoi(argv[1]);
    return(2);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_syncwrite(int32_t argc, char *argv[], uint32_t flags)
{
	int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->target_options |= TO_SYNCWRITE;
		return(3);
	} else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->target_options |= TO_SYNCWRITE;
				i++;
				p = xgp->ptdsp[i];
			}
		}
	return(1);
	}
}
/*----------------------------------------------------------------------------*/
// Specify a single target name
int
xddfunc_target(int32_t argc, char *argv[], uint32_t flags)
{ 
	int target_number;
	ptds_t *p;

	if (flags & XDD_PARSE_PHASE1) {
		target_number = xgp->number_of_targets; // This is the last target + 1 
		p = xdd_get_ptdsp(target_number, argv[0]); 
		if (p == NULL) return(-1);
	
		p->target = argv[1];
		xgp->number_of_targets++; 
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
xddfunc_targetdir(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
	    p = xdd_get_ptdsp(target_number, argv[0]); 
		if (p == NULL) return(-1);

	    p->targetdir = argv[args+1];
        return(args+2);
    } else { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->targetdir = argv[args+1];
				i++;
				p = xgp->ptdsp[i];
			}
		}
		return(2);
	}
}
/*----------------------------------------------------------------------------*/
// Specify the starting offset into the device in blocks
// Arguments: -targetoffset #
// 
int
xddfunc_targetoffset(int32_t argc, char *argv[], uint32_t flags)
{
	xgp->target_offset = atoll(argv[1]);
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
xddfunc_targets(int32_t argc, char *argv[], uint32_t flags)
{
    int i,j,k;
	int status;
	ptds_t *p;

	if (flags & XDD_PARSE_PHASE1) {
		k = xgp->number_of_targets;  // Keep the current number of targets we have
		xgp->number_of_targets = atoi(argv[1]); // get the number of targets to add to the list
		if (xgp->number_of_targets < 0) { // Set all the target names to the single name specified
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
			xgp->number_of_targets *= -1; // make this a positive number 
			xgp->number_of_targets += k;  // add in the previous number of targets 
			for (j=k; j<xgp->number_of_targets; j++) { // This will add targets to the end of the current list of targets 
				// Call xdd_get_ptds() for each target and put the same target name in each PTDS
				// Make sure the PTDS for this target exists - if it does not, the xdd_get_ptds() subroutine will create one
				p = xdd_get_ptdsp(j, argv[0]);
				if (p == NULL) return(-1);
				
				p->target = argv[2];
			} // end of FOR loop that places a single target name on each of the associated PTDSs
		} else { // Set all target names to the appropriate name
			i = 2; // start with the third argument  
			xgp->number_of_targets += k;  // add in the previous number of targets 
			for (j=k; j<xgp->number_of_targets; j++) { // This will add targets to the end of the current list of targets 
				// Make sure the PTDS for this target exists - if it does not, the xdd_get_ptds() subroutine will create one
				p = xdd_get_ptdsp(j, argv[0]);
				if (p == NULL) return(-1);
				p->target = argv[i];
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
// The target start delay function will add a delay to each target's
// starting time. The amount of time to delay is calculated simply by 
// multiplying the specified time delay multiplier by the target number.
// For example, "-targetstartdelay 2.1" will add 0 seconds to target 0's
// start time, 2.1 seconds to target 1's start time, 4.2 seconds to target 2's
// start time (i.e. 2*2.1) and so on. 
//
int
xddfunc_targetstartdelay(int32_t argc, char *argv[], uint32_t flags)
{ 
    double tmpf;

	if (argc < 2) { // Not enough arguments in this line
		fprintf(xgp->errout,"%s: ERROR: Not enough arguments to fully qualify this option: %s\n",
			   	xgp->progname, argv[0]);
		return(-1);
	}

	tmpf = atof(argv[1]);
	xgp->target_start_delay = (pclk_t)(tmpf * TRILLION);
	return(2);
}
/*----------------------------------------------------------------------------*/
// Specify the throttle type and associated throttle value 
// Arguments: -throttle [target #] bw|ops|var #.#
// 
int
xddfunc_throttle(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    char *what;
    double value;
    ptds_t *p;
    int retval;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);
	if (argc < 3) {
		fprintf(xgp->errout,"%s: ERROR: not enough arguments specified for the option '-throttle'\n",xgp->progname);
		return(0);
	}
	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		what = argv[args+1];
		value = atof(argv[args+2]);
        retval = args+3;
    } else { /* All targets */
        p = 0;
        what = argv[1];
		value = atof(argv[2]);
        retval = 3;
    }

    if (strcmp(what, "ops") == 0) {/* Throttle the ops/sec */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (p) {
		    p->throttle_type = PTDS_THROTTLE_OPS;
            p->throttle = value;
        } else  { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->throttle_type = PTDS_THROTTLE_OPS;
					p->throttle = value;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(retval);
    } else if (strcmp(what, "bw") == 0) {/* Throttle the bandwidth */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (p) {
		    p->throttle_type = PTDS_THROTTLE_BW;
            p->throttle = value;
        } else { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->throttle_type = PTDS_THROTTLE_BW;
					p->throttle = value;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(retval);
    } else if (strcmp(what, "delay") == 0) {/* Introduce a delay of # seconds between ops */
        if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle delay of %5.2f is not valid. throttle must be a number greater than 0.00\n",xgp->progname,value);
            return(0);
        }
        if (p) {
		    p->throttle_type = PTDS_THROTTLE_DELAY;
            p->throttle = value;
        } else { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->throttle_type = PTDS_THROTTLE_DELAY;
					p->throttle = value;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(retval);
    } else if (strcmp(what, "var") == 0) { /* Throttle Variance */
		if (value <= 0.0) {
			fprintf(xgp->errout,"%s: throttle variance of %5.2f is not valid. throttle variance must be a number greater than 0.00 but less than the throttle value of %5.2f\n",xgp->progname,p->throttle_variance,p->throttle);
			return(0);
		}
        if (p)
            p->throttle_variance = value;
		else { /* Set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->throttle_variance = value;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(retval);
    } else {
		fprintf(xgp->errout,"%s: throttle type of of %s is not valid. throttle type must be \"ops\", \"bw\", or \"var\"\n",xgp->progname,what);
		return(0);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timelimit(int32_t argc, char *argv[], uint32_t flags)
{
    int args, i; 
    int target_number;
    ptds_t *p;

    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
    if (args < 0) return(-1);

	if (xdd_parse_arg_count_check(args,argc, argv[0]) == 0)
		return(0);

	if (target_number >= 0) { /* Set this option value for a specific target */
		p = xdd_get_ptdsp(target_number, argv[0]);
		if (p == NULL) return(-1);

		p->time_limit = atoi(argv[args+1]);
        return(args+2);
	} else  { /* Set option for all targets */
		if (flags & XDD_PARSE_PHASE2) {
			p = xgp->ptdsp[0];
			i = 0;
			while (p) {
				p->time_limit = atoi(argv[args+1]);
				i++;
				p = xgp->ptdsp[i];
			}
		}
        return(2);
	}
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timerinfo(int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_TIMER_INFO;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_timeserver(int32_t argc, char *argv[], uint32_t flags)
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
		xgp->gts_hostname = argv[i];
		return(i+1);
	} else if (strcmp(argv[i], "port") == 0) { /* The port number to use when talking to the time server host */
        i++;
		xgp->gts_port = (in_port_t)atol(argv[i]);
		return(i+1);
	} else if (strcmp(argv[i], "bounce") == 0) { /* Specify the number of times to ping the time server */
        i++;
		xgp->gts_bounce = atoi(argv[i]);
		return(i+1);
    } else {
			fprintf(stderr,"%s: Invalid timeserver option %s\n",xgp->progname, argv[i]);
            return(0);
    } /* End of the -timeserver sub options */

}
/*----------------------------------------------------------------------------*/
// xdd_get_timestamp_pointer() - This is a helper function for the
//     xddfunc_timestamp() parsing function. 
//     This function checks to see if there is a valid pointer to the timestamp
//     information structure in the specified PTDS. If there is no pointer then
//     this function will allocate memory for the  timestamp structure and 
//     set the "tsp" member of the specified PTDS accordingly. 
//     If for some strange reason the malloc() fails this routine will return 0.
// Arguments: Pointer to a PTDS
// Return value: This function normally returns a valid timestamp pointer as well. 
// 
timestamp_t *
xdd_get_timestamp_pointer(ptds_t *p)
{
	if (p->tsp == NULL) { // Allocate a new data pattern struct for this target PTDS
		p->tsp = (timestamp_t *)malloc(sizeof(timestamp_t));
		if (p->tsp == NULL) {
			fprintf(xgp->errout,"%s: ERROR: Cannot allocate memory for the EndToEnd PTDS structure.\n",xgp->progname);
			return(0);
		}
	}
	return(p->tsp);
} 
/*----------------------------------------------------------------------------*/
 // -ts on|off|detailed|summary|oneshot
 //     output filename
int
xddfunc_timestamp(int32_t argc, char *argv[], uint32_t flags) 
{
	int i;
	int args, args_index; 
	int target_number;
	ptds_t *p;


	args_index = 1;
	args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= (TS_ON | TS_ALL);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= (TS_ON | TS_ALL);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "off") == 0) { /* Turn off the time stamp reporting option */
		if (target_number >= 0) { 
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options &= ~TS_ON; /* Turn OFF time stamping */
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options &= ~TS_ON; /* Turn OFF time stamping */
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "wrap") == 0) { /* Turn on the TS Wrap option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= TS_WRAP;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= TS_WRAP;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "oneshot") == 0) { /* Turn on the TS Wrap option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= TS_ONESHOT;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= TS_ONESHOT;
					i++;
					p = xgp->ptdsp[i];
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_size = atoi(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_size = atoi(argv[args_index]);
					i++;
					p = xgp->ptdsp[i];
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= (TS_ON | TS_TRIGTIME);
			p->tsp->ts_trigtime = atoll(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= (TS_ON | TS_TRIGTIME);
					p->tsp->ts_trigtime = atoll(argv[args_index]);
					i++;
					p = xgp->ptdsp[i];
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= (TS_ON | TS_TRIGOP);
			p->tsp->ts_trigop = atoi(argv[args_index]);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= (TS_ON | TS_TRIGOP);
					p->tsp->ts_trigop = atoi(argv[args_index]);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "normalize") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_NORMALIZE);
					i++;
					p = xgp->ptdsp[i];
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= (TS_ON | TS_ALL);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= (TS_ON | TS_ALL);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->tsoutput_filename = argv[args_index];
		return(args_index+1);
	} else if (strcmp(argv[args_index], "append") == 0) { /* set the time stamp Append Output File  reporting option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_APPEND);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_APPEND);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "dump") == 0) { /* dump a binary stimestamp file to "filename" */
        args_index++;
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_DUMP);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		xgp->tsbinary_filename = argv[args_index];
		return(args_index+1);
	} else if (strcmp(argv[args_index], "summary") == 0) { /* set the time stamp SUMMARY reporting option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_SUMMARY);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "detailed") == 0) { /* set the time stamp DETAILED reporting option */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);

			xdd_get_timestamp_pointer(p);
			p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					xdd_get_timestamp_pointer(p);
					p->tsp->ts_options |= ((TS_ON | TS_ALL) | TS_DETAILED | TS_SUMMARY);
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	}
    return(args_index+1);
} // End of xddfunc_timestamp()
/*----------------------------------------------------------------------------*/
 // -verify location | contents
int
xddfunc_verify(int32_t argc, char *argv[], uint32_t flags) 
{
    int i;
    int args, args_index; 
    int target_number;
    ptds_t *p;

    args_index = 1;
    args = xdd_parse_target_number(argc, &argv[0], flags, &target_number);
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
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->target_options |= TO_VERIFY_CONTENTS;
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= TO_VERIFY_CONTENTS;
					i++;
					p = xgp->ptdsp[i];
				}
			}
		}
		return(args_index+1);
	} else if (strcmp(argv[args_index], "location") == 0) { /*  Verify the buffer location */
		if (target_number >= 0) {
			p = xdd_get_ptdsp(target_number, argv[0]);
			if (p == NULL) return(-1);
			p->target_options |= (TO_VERIFY_LOCATION | TO_SEQUENCED_PATTERN);
		} else {  /* set option for all targets */
			if (flags & XDD_PARSE_PHASE2) {
				p = xgp->ptdsp[0];
				i = 0;
				while (p) {
					p->target_options |= (TO_VERIFY_LOCATION | TO_SEQUENCED_PATTERN);
					i++;
					p = xgp->ptdsp[i];
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
xddfunc_verbose(int32_t argc, char *argv[], uint32_t flags)
{
	xgp->global_options |= GO_VERBOSE;
    return(1);
}
/*----------------------------------------------------------------------------*/
int
xddfunc_version(int32_t argc, char *argv[], uint32_t flags)
{
    fprintf(stdout,"%s: Version %s  based on version %s - %s\n",xgp->progname, XDD_VERSION, XDD_BASE_VERSION, XDD_COPYRIGHT);
    return(-1);
}

/*----------------------------------------------------------------------------*/
int
xddfunc_invalid_option(int32_t argc, char *argv[], uint32_t flags)
{
	fprintf(xgp->errout, "%s: Invalid option: %s (%d)\n",
		xgp->progname, argv[0], argc);
    return(0);
} // end of xddfunc_invalid_option() 
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
