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
#define XDD_CMDLINE_LENGTH	512		// Length of the command line

/*----------------------------------------------------------------------------*/
/* xdd_interactive() - This thread conttrols "interactive mode" 
 * This thread will read input from stdin and execute commands as specified.
 *
 */
void *
xdd_interactive(void *data) {
	char			*fstatus;						// Status of fgets() 
	FILE			*cmdin;							// Where to read commands from (stdin normally)
	char			cmdline[XDD_CMDLINE_LENGTH];	// The command line buffer
	char			*cp;							// character pointer to the next token in the command line
	int				tokens;							// Number of tokens in the command line
	int				len;							// Length of the command line
	int				cmd_status;						// Status from execution of the last command
	char			*identity;						// Identity is either "icp" or "debug"
	xdd_plan_t		*planp;



	planp = (xdd_plan_t *)data;
	cmdin = stdin;
	if (1) { // This means that we have been called by the signal handler 
		// In debugger mode, we do not initialize or enter barriers....
		identity = "DEBUG";
		planp->heartbeat_flags |= HEARTBEAT_HOLDOFF; // Turn off the heartbeat just in case it is running...
	} else { // Otherwise, this is just the interactive option
		// Init the interactive flow control barriers
		identity = "ICP";
		xdd_init_barrier(planp, &planp->interactive_barrier, planp->number_of_targets+1, "interactive_barrier");
	
		// Initialize our "occupant" structure
		xdd_init_barrier_occupant(&planp->interactive_occupant, "INTERACTIVE", (XDD_OCCUPANT_TYPE_SUPPORT), NULL);
	
		// Enter this barrier to tell xdd that the interactive thread is running
		xdd_barrier(&planp->main_general_init_barrier,&planp->interactive_occupant,0);
	}

	// The command line loop
	while (1) {
		fprintf(stdout,"%s READY> ",identity);
		memset(cmdline,0,XDD_CMDLINE_LENGTH);
		fstatus = fgets(cmdline,XDD_CMDLINE_LENGTH, cmdin);
		if (fstatus == NULL) {
			fprintf(stdout,"\n%s exiting\n",identity);
			planp->plan_options &= ~PLAN_INTERACTIVE;
			break;
		} 
		len = strlen(cmdline);
		if (len == 0) {
			fprintf(stdout, "\nHuh?\n");
			continue;
		}
fprintf(stderr,"received command: '%s' %d bytes\n",cmdline,len);
		cp = cmdline;
		// Skip over any leading white space in the command line
		while ((*cp == TAB) || (*cp == SPACE)) cp++;

		// Find out how many words/arguments are in the command line
		tokens = xdd_tokenize(cp);

fprintf(stderr,"parsing command: '%s' %d tokens\n",cmdline,tokens);
		// Call xdd_interactive_parse_command() to execute this command
		cmd_status = xdd_interactive_parse_command(tokens, cp, planp);

		if (cmd_status) {
			fprintf(stdout, "\n %s Exiting\n",identity);
			break;
		}
	}
	return(0);
} // End of xdd_interactive()

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
 */
int
xdd_interactive_parse_command(int tokens, char *cmd, xdd_plan_t *planp) {
	int	flags;
    int funci;      // Index into xdd_interactive_func[] table
    int status;     // Status from the command-specific subroutine
    int not_found;  // Indicates that an command was not found

	funci = 0;
	flags = 0;
	not_found = 1;
	while (xdd_interactive_func[funci].func_name) { // The last entry in the xddfunc table is 0 which will kick us out of this loop.
		if ((strcmp(xdd_interactive_func[funci].func_name,cmd) == 0) || 
			(strcmp(xdd_interactive_func[funci].func_alt, cmd) == 0)) {
			status = (int)xdd_interactive_func[funci].interactive_func_ptr(tokens, cmd, flags, planp);
			not_found = 0;
			break;
		}
		funci++;
	}
	// Check to see if things worked...
	if (not_found) { // Indicates that the specified option was not found in the xddfunc table. 
		fprintf(xgp->output,"\nERROR: Command '%s' not recognized\n",cmd);
		return(0);
	}      
	return(status);
} /* End of xdd_interactive_parse_command() */
