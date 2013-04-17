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
 * This file contains the table of subroutines that implement the various 
 * interactive commands.
 */

#include "xint.h"
#include "interactive.h"
/* -------------------------------------------------------------------------------------------------- */
// The xdd Interaction Command Function Table
//   This table contains entries of the following structure as defined in xdd.h:
// 		struct xdd_interactive_func {
//			char    *func_name;     /* name of the function */
//			char    *func_alt;      /* Alternate name of the function */
//   		int     (*interactive_func_ptr)(int32_t tokens, char *cmd, uint32_t flags);      /* pointer to the function */
//  		int     argc;           /* number of arguments */
// 			char    *help;          /* help string */
//			char    *ext_help[XDD_EXT_HELP_LINES];   /* Extented help strings */
//			uint32_t flags;			/* Flags for various parsing functions */
//      }

xdd_interactive_func_t  xdd_interactive_func[] = {
    {"exit", "quit",
            xdd_interactive_exit,  
            1,  
            "  exit \n",  
            {"    Will cause the run to exit at this point\n", 
            0,0,0,0},
			0},
    {"goto", "go",
            xdd_interactive_goto,  
            1,  
            "  goto <operation number>\n",  
            {"    Will cause the run to resume at the specified operation number\n", 
            0,0,0,0},
			0},
    {"help", "h",
            xdd_interactive_help,  
            1,  
            "  help <command_name>\n",  
            {"    Will print out extended help for a specific interactive command\n", 
            0,0,0,0},
			0},
    {"run", "run",
            xdd_interactive_run,  
            1,  
            "  run \n",  
            {"    Will continue the run from the current operation number\n", 
            0,0,0,0},
			0},
    {"show",   "sh",
            xdd_interactive_show, 
            1,  
            "  show <item>\n",  
            {"    will show different items\n", 
            0,0,0,0},
			0},
    {"step", "step",
            xdd_interactive_step,  
            1,  
            "  step <N>\n",  
            {"    Will continue the run from the current operation number for N operations then pause\n", 
            0,0,0,0},
			0},
    {"stop", "stop",
            xdd_interactive_stop,
            1,  
            "  stop\n",  
            {"    Will cause the run to stop at the current operation. Use the 'run' or 'step' command to resume the run\n", 
            0,0,0,0},
			0},
    {"timestampreport",   "tsreport",
            xdd_interactive_ts_report,
            1,  
            "  timestampreport\n",  
            {"    will generate the time stamp report file\n", 
            0,0,0,0},
			0},
    {0,0,0,0,0,{0,0,0,0,0},0}
}; // This is the end of the interactive command definitions

 
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
