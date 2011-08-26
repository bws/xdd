/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2001-2008 H. Peter Anvin - All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * truncate.c
 *
 * Small program to truncate a file to any size.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sysexits.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

const char *program;

void usage(int exit_code)
{
    fprintf(stderr, "Usage: %s [-s size]... FILE...\n", program);
    exit(exit_code);
}

int main(int argc, char *argv[])
{
    int opt;
    size_t tsize = 0;
    int i;
    int err = 0;
    
    program = argv[0];
    
    while ( (opt = getopt(argc, argv, "s:")) != -1 ) {
        switch ( opt ) {
            case 's':
                tsize = atoi(optarg);
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(EX_USAGE);
                break;
        }
    }

    if ( optind == argc )
        usage(EX_USAGE);
  
    for ( i = optind ; i < argc ; i++ ) {
        char* filename = argv[optind];
        int rc = truncate(filename, tsize);
        if (0 != rc) {
            /** Handle errors */
            err = 1;
            break;
        }
    }
    
    return err;
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
