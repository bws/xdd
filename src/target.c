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
 * This file contains the subroutines that perform an "open" operation 
 * on the target device/file.
 */
#include "xdd.h"

/* xdd_target_start() - Will start all the target threads and qthreads.
 * The basic idea here is that there are N targets and each target can have X instances ( or queue threads as they are referred to)
 * The "ptds" array contains pointers to the main ptds's for the primary targets.
 * Each target then has a pointer to a linked-list of ptds's that constitute the targets that are used to implement the queue depth.
 * The thread creation process is serialized such that when a thread is created, it will perform its initialization tasks and then
 * enter the "serialization" barrier. Upon entering this barrier, the *while* loop that creates these threads will be released
 * to create the next thread. In the meantime, the thread that just finished its initialization will enter the main thread barrier 
 * waiting for all threads to become initialized before they are all released. Make sense? I didn't think so.
 */
int32_t
xdd_target_start() {
	int32_t		target_number;	// Target number to work on
	int32_t		status;			// Status of a subroutine call
	ptds_t		*p;				// pointer to the PTS for this QThread
	int32_t		q;				// The qthread number


	for (target_number = 0; target_number < xgp->number_of_targets; target_number++) {
		p = xgp->ptdsp[target_number]; /* Get the ptds for this target */
		q = 0;
		while (p) { /* create a thread for each ptds for this target - the queue depth */
			/* Create the new thread */
			p->run_start_time = xgp->base_time;
			status = pthread_create(&p->thread, NULL, xdd_io_thread, p);
			if (status) {
				fprintf(xgp->errout,"%s: Error creating thread %d",
					xgp->progname, 
					target_number);
				fflush(xgp->errout);
				perror("Reason");
				xdd_destroy_all_barriers();
				return(-1);
			}
			p->my_target_number = target_number;
			p->my_qthread_number = q;
			p->total_threads = xgp->number_of_iothreads;
			/* Wait for the previous thread to complete initialization and then create the next one */
			xdd_barrier(&xgp->serializer_barrier[p->mythreadnum%2]);
			// If the previous target *open* fails, then everything needs to stop right now.
			if (xgp->abort_io == 1) { 
				fprintf(xgp->errout,"%s: xdd_main: xdd thread %d aborting due to previous initialization failure\n",
					xgp->progname,
					p->my_target_number);
				fflush(xgp->errout);
				xdd_destroy_all_barriers();
				return(-1);
			}
			q++;
			p = p->nextp; /* get next ptds in the queue for this target */
		}

	} /* end of FOR loop that starts all procs */
	return(0);
} // End of xdd_target_start()


#ifdef WIN32
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_windows() - Open routine for Windows
 */
int32_t
xdd_target_open_for_windows(ptds_t *p) {
	HANDLE hfile;  /* The file handle */
	LPVOID lpMsgBuf; /* Used for the error messages */
	unsigned long flags; /* Open flags */
	DWORD disp;

	// Check to see if this is a "device" target - must have \\.\ before the name
	// Otherwise, it is assumed that this is a regular file
	if (strncmp(target_name,"\\\\.\\",4) == 0) { // This is a device file *not* a regular file
		p->target_options |= TO_DEVICEFILE;
		p->target_options &= ~TO_REGULARFILE;
	}
	else { // This is a regular file *not* a device file
		p->target_options &= ~TO_DEVICEFILE;
		p->target_options |= TO_REGULARFILE;
	}

	// We can only create new files if the target is a regular file (not a device file)
	if ((p->target_options & TO_CREATE_NEW_FILES) && (p->target_options & TO_REGULARFILE)) { 
		// Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}

	/* open the target */
	if (p->target_options & TO_DIO) 
		flags = FILE_FLAG_NO_BUFFERING;
	else flags = 0;
	/* Device files and files that are being read MUST exist
	 * in order to be opened. For files that are being written,
	 * they can be created if they do not exist or use the existing one.
	 */
	if ((p->target_options & TO_DEVICEFILE) || (p->rwratio == 1.0))
		disp = OPEN_EXISTING; 
	else if (p->rwratio < 1.0)
		disp = OPEN_ALWAYS;
	else disp = OPEN_EXISTING;

	pclk_now(&p->open_start_time); // Record the starting time of the open

	if (p->rwratio < 1.0)  { /* open for write operations */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if (p->rwratio == 0.0) { /* write only */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if ((p->rwratio > 0.0) && p->rwratio < 1.0) { /* read/write mix */
		hfile = CreateFile(target_name, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else { /* open for read operations */
		hfile = CreateFile(target_name, 
			GENERIC_READ, 
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	}
	if (hfile == INVALID_HANDLE_VALUE) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(xgp->errout,"%s: xdd_open_target: Could not open target for %s: %s\n",
			xgp->progname,(p->rwratio < 1.0)?"write":"read",target_name);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			return((void *)-1);
	}
	
	pclk_now(&p->open_end_time); // Record the ending time of the open
	return(hfile);
} // End of xdd_target_open_for_windows()
#endif

#if  LINUX
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_linux() - Open routine for linux
 */
int32_t
xdd_target_open_for_linux(ptds_t *p) {


	// Set the Open Flags to indicate DirectIO if requested
	if (p->target_options & TO_DIO) 
		p->target_open_flags |= O_DIRECT;

	// Check to see if this is a SCSI Generic (/dev/sg) device - SPECIFIC to LINUX
	if (p->target_options & TO_DEVICEFILE) { 
		if (strncmp(p->target_name, "/dev/sg", 7) == 0 ) // is this an "sg" device? 
			p->target_options |= TO_SGIO; // Yes - turn on SGIO Mode
	}

	/* open the target */
	if (p->rwratio == 0.0) {
		if (p->target_options & TO_SGIO) {
			p->fd = open(p->target_name,p->target_open_flags|O_RDWR, 0777); /* Must open RDWR for SGIO */
			xdd_sg_set_reserved_size(p,p->fd);
			xdd_sg_get_version(p,p->fd);
		} else p->fd = open(p->target_name,p->target_open_flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		p->target_open_flags &= ~O_CREAT;
		if (p->target_options & TO_SGIO) {
			p->fd = open(p->target_name,p->target_open_flags|O_RDWR, 0777); /* Must open RDWR for SGIO  */
			xdd_sg_set_reserved_size(p,p->fd);
			xdd_sg_get_version(p,p->fd);
		} else p->fd = open(p->target_name,p->target_open_flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open(p->target_name,p->target_open_flags|O_RDWR, 0666);
	}

	return(0);

} // End of xdd_target_open_for_linux()
#endif 

#if AIX
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_aix() - Open routine for aix
 */
int32_t
xdd_target_open_for_aix(ptds_t *p) {

	p->target_open_flags |= O_LARGEFILE;
	// Set the Open Flags to indicate DirectIO if requested
	if (p->target_options & TO_DIO) {
		p->target_open_flags |= O_DIRECT;
	}

	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		p->fd = open64(p->target_name,p->target_open_flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open64(p->target_name,p->target_open_flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open64(p->target_name,p->target_open_flags|O_RDWR, 0666);
	}

	return(0);
} // End of xdd_target_open_for_aix()
#endif


#if SOLARIS
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_solaris() - Open routine for solaris
 */
int32_t
xdd_target_open_for_solaris(ptds_t *p) {
	struct stat64 statbuf; /* buffer for file statistics */

	flags |= O_LARGEFILE;
//////////////////////////////tbd //////////////////////////////////////////////
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: target %s must be a regular file when used with the -dio flag\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		fd = open64(target_name,flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		fd = open64(target_name,flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		fd = open64(target_name,flags|O_RDWR, 0666);
	}
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
		status = directio(p->fd,DIRECTIO_ON);
		if (status < 0) {
			fprintf(xgp->errout,"(%s) (%d): xdd_open_target: could not set DIRECTIO flag for: %s\n",
					xgp->progname,
					p->my_target_number,
					p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	}
	return(0);
} // End of xdd_target_open_for_solaris()
#endif

#if OSX
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_osx() - Open routine for osx
 */
int32_t
xdd_target_open_for_osx(ptds_t *p) {
	struct stat64 statbuf; /* buffer for file statistics */

//////////////////////////////tbd //////////////////////////////////////////////

	flags |= O_LARGEFILE;
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: target %s must be a regular file when used with the -dio flag\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		fd = open64(target_name,flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		fd = open64(target_name,flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		fd = open64(target_name,flags|O_RDWR, 0666);
	}
	i = fstat64(fd,&statbuf);
	return(0);
} // End of xdd_target_open_for_osx()
#endif

#if FREEBSD
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_freebsd() - Open routine for freebsd
 */
int32_t
xdd_target_open_for_freebsd(ptds_t *p) {
	struct stat64 statbuf; /* buffer for file statistics */
//////////////////////////////tbd //////////////////////////////////////////////
	flags |= O_LARGEFILE;
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"%s (%d): xdd_open_target: target %s must be a regular file when used with the -dio flag\n",
					xgp->progname,
					p->my_target_number,
					target_name);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		p->fd = open64(p->target_name,p->flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		p->fd = open64(p->target_name,p->flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		p->fd = open64(p->target_name,p->flags|O_RDWR, 0666);
	}
	return(0);
} // End of xdd_target_open_for_freebsd()
#endif

/*----------------------------------------------------------------------------*/
/* xdd_target_name() - Generate the name of the target for the given PTDS
 */
void
xdd_target_name(ptds_t *p) {
	int			target_name_length;
	char 		target_name[MAX_TARGET_NAME_LENGTH]; /* target directory + target name */

	/* create the fully qualified target name */
	memset(target_name,0,sizeof(target_name));
	if (strlen(p->targetdir) > 0)
		sprintf(target_name, "%s%s", p->targetdir, p->target);
	else sprintf(target_name, "%s",p->target);

	if (p->target_options & TO_CREATE_NEW_FILES) { // Add the target extension to the name
		strcat(target_name, ".");
		strcat(target_name, p->targetext);
	}
	target_name_length = strlen(target_name);
	if (p->target_name == NULL) {
		p->target_name = (char *)malloc(target_name_length+32);
	}
	sprintf(p->target_name,"%s",target_name);

} // End of xdd_target_name()

/*----------------------------------------------------------------------------*/
/* xdd_target_existence_check() - Check to see if the target exists. 
 * Return -1 is it does not exist and this is a read or 0 if it does exist.
 */
int32_t
xdd_target_existence_check(ptds_t *p) {
	int32_t  	status; /* Status of various system function calls */

	/* Stat the file before it is opened */
#if (AIX || SOLARIS)
	status = stat64(p->target_name,&p->statbuf);
#else // All other OSs use stat
	status = stat(p->target_name,&p->statbuf);
#endif

	// If stat did not work then it could be that the file does not exist.
	// For write operations this is ok because the file will just be created.
	// Hence, if the errno is ENOENT then set the target file type to
	// be a "regular" file, issue an information message, and continue.
	// If this is a read or a mixed read/write operation then issue an
	// error message and return a -1 indicating an error on open.
	//
	if (status < 0) { 
		if ((ENOENT == errno) && (p->rwratio == 0.0)) { // File does not yet exist
			p->target_options |= TO_REGULARFILE;
			fprintf(xgp->errout,"%s: (%d): xdd_target_existence_check: NOTICE: target %s does not exist so it will be created.\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
		} else { // Something is wrong with the file
			fprintf(xgp->errout,"%s: (%d): xdd_target_existence_check: ERROR: Could not stat target: %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	}
	// Check to see what kind of target file this really is
	if (S_ISREG(p->statbuf.st_mode)) {  // Is this a Regular File?
		p->target_options |= TO_REGULARFILE;
	} else if (S_ISDIR(p->statbuf.st_mode)) { // Is this a Directory?
		p->target_options |= TO_REGULARFILE;
		fprintf(xgp->errout,"%s: (%d): xdd_target_existence_check: WARNING: Target %s is a DIRECTORY\n",
			xgp->progname,
			p->my_target_number,
			p->target_name);
		fflush(xgp->errout);
	} else if (S_ISCHR(p->statbuf.st_mode)) { // Is this a Character Device?
		p->target_options |= TO_DEVICEFILE;
	} else if (S_ISBLK(p->statbuf.st_mode)) {  // Is this a Block Device?
		p->target_options |= TO_DEVICEFILE;
	} else if (S_ISSOCK(p->statbuf.st_mode)) {  // Is this a Socket?
		p->target_options |= TO_SOCKET;
	}
	/* If this is a regular file, and we are trying to read it, then
 	* check to see that we are not going to read off the end of the
 	* file. Send out a WARNING if this is a possibility
 	*/
	if ( (p->target_options & TO_REGULARFILE) && !(p->rwratio < 1.0)) { // This is a purely read operation
		if (p->target_bytes_to_xfer_per_pass > p->statbuf.st_size) { // Check to make sure that the we won't read off the end of the file
		fprintf(xgp->errout,"%s (%d): xdd_target_existence_check: WARNING! The target file <%lld bytes> is shorter than the the total requested transfer size <%lld bytes>\n",
			xgp->progname,
			p->my_target_number,
			(long long)p->statbuf.st_size, 
			(long long)p->target_bytes_to_xfer_per_pass);
		fflush(xgp->errout);
		}
	}
	/* Perform sanity checks */
	if (p->target_options & TO_DIO) {
		/* make sure it is a regular file, otherwise fail */
		if ( (p->statbuf.st_mode & S_IFMT) != S_IFREG) {
			fprintf(xgp->errout,"%s (%d): xdd_target_existence_check: WARNING: target %s must be a regular file when used with the -dio flag\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
			fflush(xgp->errout);
		}
	}
	return(0);
} // End of xdd_target_existence_check()
/*----------------------------------------------------------------------------*/
/* xdd_target_open() - open the target device and do all necessary 
 * sanity checks.  This routine simply calls the appropriate open routine
 * for the given operating system it is compiled for.
 * Each of the open routines will set the file descriptor in the PTDS to
 * an appropriate value. If the open routine succeeds then a successful status
 * is returned. Otherwise, an unsuccessful status is returned. The OS-specific
 * open routine will issue the appropriate error messages. 
 */
int32_t
xdd_target_open(ptds_t *p) {
	int32_t		status;		// Status of the open call


	/* create the fully qualified target name */
	xdd_target_name(p);

	// Check to see if this target really exists and record what kind of target it is
	status = xdd_target_existence_check(p);
	if (status < 0)
		return(-1);

	pclk_now(&p->open_start_time); // Record the starting time of the open
	// Set the default and optional open flags
	p->target_open_flags |= O_CREAT;
#if LINUX 
	status = xdd_target_open_for_linux(p);
#elif WIN32
	status = xdd_target_open_for_windows(p);
#elif AIX
	status = xdd_target_open_for_aix(p);
#elif SOLARIS
	status = xdd_target_open_for_solaris(p);
#elif OSX
	status = xdd_target_open_for_osx(p);
#elif FREEBSD
	status = xdd_target_open_for_freebsd(p);
#endif

	pclk_now(&p->open_end_time); // Record the ending time of the open

	// Check the status of the OPEN operation to see if it worked
	if (p->fd < 0) {
			fprintf(xgp->errout,"%s (%d): xdd_target_open_for_linux: could not open target: %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_name);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}
	return(0);

} // End of xdd_target_open()

