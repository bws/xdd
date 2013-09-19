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

/* The format of the entries in the xdd interactive command function table */
typedef int (*interactive_func_ptr)(int32_t tokens, char *cmd, uint32_t flags, xdd_plan_t *planp);

#define XDD_INTERACTIVE_EXTRA_HELP_LINES 5
#define XDD_INTERACTIVE_FUNC_INVISIBLE 0x00000001
struct xdd_interactive_func {
	char    *func_name;     /* name of the function */
	char    *func_alt;      /* Alternate name of the function */
    int     (*interactive_func_ptr)(int32_t tokens, char *cmd, uint32_t flags, xdd_plan_t *planp);      /* pointer to the function */
    int     argc;           /* number of arguments */
    char    *help;          /* help string */
    char    *ext_help[XDD_INTERACTIVE_EXTRA_HELP_LINES];   /* Extented help strings */
	uint32_t flags;			/* Flags for various parsing functions */
}; 
typedef struct xdd_interactive_func xdd_interactive_func_t;

