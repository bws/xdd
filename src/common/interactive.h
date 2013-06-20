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
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

/* The format of the entries in the xdd interactive command function table */
typedef int (*interactive_func_ptr)(int32_t tokens, char *cmd, uint32_t flags);

#define XDD_EXT_HELP_LINES 5
struct xdd_interactive_func {
	char    *func_name;     /* name of the function */
	char    *func_alt;      /* Alternate name of the function */
    int     (*interactive_func_ptr)(int32_t tokens, char *cmd, uint32_t flags);      /* pointer to the function */
    int     argc;           /* number of arguments */
    char    *help;          /* help string */
    char    *ext_help[XDD_EXT_HELP_LINES];   /* Extented help strings */
	uint32_t flags;			/* Flags for various parsing functions */
}; 
typedef struct xdd_interactive_func xdd_interactive_func_t;

