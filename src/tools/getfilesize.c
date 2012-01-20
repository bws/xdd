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
 * Small program to print file sizes.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sysexits.h>

const char *program;

void usage(int exit_code)
{
    fprintf(stderr, "Usage: %s FILE...\n", program);
    exit(exit_code);
}

int main(int argc, char *argv[])
{
    int i;
    int err = 0;
    
    program = argv[0];
    
    for (i = 1; i < argc ; i++ ) {
        char* filename = argv[i];
        struct stat buffer;
        int rc = stat(filename, &buffer);
        if (0 != rc) {
            /** Handle errors */
	    switch(errno) {
		case(EACCES):
		    fprintf(stderr, "Unable to access file: %s\n", filename);
		    break;
		case(ENAMETOOLONG):
		    fprintf(stderr, "Filename too long: %s\n", filename);
		    break;
		case(ENOTDIR):
		    fprintf(stderr, "Invalid path segment: %s\n", filename);
		    break;
		case(EFAULT):
		    fprintf(stderr, "Invalid buffer address.\n");
		    break;
		case(ENOENT):
		    fprintf(stderr, "File does not exist: %s\n", filename);
		    break;
		case(EOVERFLOW):
		    fprintf(stderr, "Overflow error.  Abort.\n");
		    break;
		default:
		    fprintf(stderr, "Unknown error occurred during stat %d.\n",
			    errno);
		    break;
	    }
            err = 1;
            break;
        }
        else {
            printf("%llu\n", (long long unsigned)buffer.st_size);
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
