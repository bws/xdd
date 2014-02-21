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
 * getfilesize.c
 *
 * Small program to print a file's size.
 */
#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sysexits.h>

/* Include available device IOCTLs */
#ifdef HAVE_SYS_DISK_H
#include <sys/disk.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

const char *program;

void usage(int exit_code)
{
    fprintf(stderr, "Usage: %s FILE...\n", program);
    exit(exit_code);
}

int get_block_device_size(const char* filename, size_t* bytes) {
	int rc = 0;
	int fd = open(filename, O_RDONLY);
	if (-1 == fd) {
		rc = 1;
		switch(errno) {
			case(EACCES):
				fprintf(stderr, "Root privileges required to access device: %s\n", filename);
				break;
			case(EEXIST):
				fprintf(stderr, "Device does not exist: %s\n", filename);
				break;
			case(EINTR):
				fprintf(stderr, "Signal caught during open: %s\n", filename);
				break;
			case(EINVAL):
				fprintf(stderr, "Invalid permissions to access block device: %s\n",
						filename);
				break;
			case(EIO):
				fprintf(stderr, "Invalid block device: %s\n", filename);
				break;
			case(EISDIR):
				fprintf(stderr, "Block device is a directory: %s\n", filename);
				break;
			default:
				fprintf(stderr, "Unknown error occurred during open: %s.\n",
						strerror(errno));
				break;
		}
	}
	else {
#if HAVE_DECL_DKIOCGETBLOCKSIZE && HAVE_DECL_DKIOCGETBLOCKSIZE
		size_t block_sz, block_ct;
		rc = ioctl(fd, DKIOCGETBLOCKSIZE, &block_sz);
		rc += ioctl(fd, DKIOCGETBLOCKCOUNT, &block_ct);
		if (0 == rc) {
			*bytes = block_sz * block_ct;
		}
		else {
			fprintf(stderr, "Unknown error occurred during ioctl: %d.\n",
						errno);
		}
#elif HAVE_DECL_BLKGETSIZE64
		rc = ioctl(fd, BLKGETSIZE64, bytes);
#else
		fprintf(stderr, "No method for getting device size.\n");
		rc = -1;
#endif
	}
	return rc;
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
			goto error_out;
        }
		else {
			size_t bytes = 0;
			if (S_ISREG(buffer.st_mode))
				bytes = buffer.st_size;
			else if (S_ISBLK(buffer.st_mode)) {
				int rc = get_block_device_size(filename, &bytes);
				if (0 != rc) {
					fprintf(stderr, "Unable to get size of block device: %s\n",
							filename);
					err = 2;
					goto error_out;
				}
			}
			else {
				fprintf(stderr,
						"Filename is not a regular file or block device: %s\n", 
						filename);
				err = 3;
				goto error_out;
			}
			printf("%zu\n", bytes);
		}
	}
error_out:    
    return err;
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
