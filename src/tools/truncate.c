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
	    switch(errno) {
		case(EACCES):
		    fprintf(stderr, "Invalid permissions to access file: %s\n",
			    filename);
		    break;
		case(EAGAIN):
		    fprintf(stderr, "EAGAIN error for unknown reason %s\n",
			    filename);
		    break;
		case(EDQUOT):
		    fprintf(stderr, "Trauncate failed due to quota %s\n",
			    filename);
		    break;
		case(EFBIG):
		    fprintf(stderr, "Truncate size too large %s\n", filename);
		    break;
		case(EISDIR):
		    fprintf(stderr, "Cannot truncate directory:  %s\n",
			    filename);
		    break;
		case(EINTR):
		    fprintf(stderr, "A signal was caught while processing %s\n",
			    filename);
		    break;
		case(EIO):
		    fprintf(stderr, "I/O error while processing %s\n",
			    filename);
		    break;
		case(ELOOP):
		    fprintf(stderr, "Too many symbolic links %s\n", filename);
		    break;
		case(EMFILE):
		    fprintf(stderr, "EMFile error %s\n", filename);
		    break;
		case(ENAMETOOLONG):
		    fprintf(stderr, "Pathname too long %s\n", filename);
		    break;
		case(ENOENT):
		    fprintf(stderr, "Invalid path %s\n", filename);
		    break;
		case(ENOSPC):
		    fprintf(stderr, "Could not allocate space for %s\n",
			    filename);
		    break;
		case(EROFS):
		    fprintf(stderr, "File is in read-only FS: %s\n", filename);
		    break;
		case(ETXTBSY):
		    fprintf(stderr, "File is busy %s\n", filename);
		    break;
		default:
		    fprintf(stderr, "Unknown error %d\n", errno);
		    break;
	    }
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
