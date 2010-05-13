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


/*----------------------------------------------------------------------------*/
/* xdd_target_open() - open the target device and do all necessary 
 * sanity checks.  This routine simply calls the appropriate open routine
 * for the given operating system it is compiled for.
 * Each of the open routines will set the file descriptor in the PTDS to
 * an appropriate value. If the open routine succeeds then a successful status
 * of 0 (zero) is returned. 
 * Otherwise, a -1 returned to indicate there was an error. 
 * The OS-specific open routine will issue the appropriate error messages. 
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

	// Call the OS-specific open call
	xdd_target_open_for_os(p);

	pclk_now(&p->open_end_time); // Record the ending time of the open

	// Check the status of the OPEN operation to see if it worked
	if (p->fd < 0) {
			fprintf(xgp->errout,"%s: xdd_target_open: ERROR: Could not open target number %d name %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_full_pathname);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
	}


	return(0);

} // End of xdd_target_open()

/*----------------------------------------------------------------------------*/
/* xdd_target_reopen() - This subroutine will close and reopen a new target
 * file and then request that all qthreads do the same.
 */
void
xdd_target_reopen(ptds_t *p) {
	int32_t		status;		// Status of the open call


	// Close the existing target
#if (WIN32)
	CloseHandle(p->fd);
#else
	close(p->fd);
#endif
	if (p->target_options & TO_CREATE_NEW_FILES) { // Create a new file name for this target
		sprintf(p->target_extension,"%08d",p->my_current_pass_number+1);
	}
	// Check to see if this is the last pass - in which case do not create a new file because it will not get used
	if (p->my_current_pass_number == xgp->passes)
		return;
	// If we need to "recreate" the file for each pass then we should delete it here before we re-open it 
	if (p->target_options & TO_RECREATE)	
#ifdef WIN32
		DeleteFile(p->target_full_pathname);
#else
		unlink(p->target_full_pathname);
#endif
	/* open the old/new/recreated target file */
	status = xdd_target_open(p);
	if (status != 0) { /* error openning target */
		fprintf(xgp->errout,"%s: xdd_target_reopen: ERROR: Aborting I/O for target number %d name %s due to open failure\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
		fflush(xgp->errout);
		xgp->abort = 1;
	}
	return;
} // End of xdd_target_reopen()

/*----------------------------------------------------------------------------*/
/* xdd_target_name() - Generate the name of the target for the given PTDS
 */
void
xdd_target_name(ptds_t *p) {
	int			target_name_length;
	char 		target_full_pathname[MAX_TARGET_NAME_LENGTH]; /* target directory + target name */


	/* create the fully qualified target name */
	memset(target_full_pathname,0,sizeof(target_full_pathname));
	if (strlen(p->target_directory) > 0)
		sprintf(target_full_pathname, "%s%s", p->target_directory, p->target_basename);
	else sprintf(target_full_pathname, "%s",p->target_basename);

	if (p->target_options & TO_CREATE_NEW_FILES) { // Add the target extension to the name
		strcat(target_full_pathname, ".");
		strcat(target_full_pathname, p->target_extension);
	}
	target_name_length = strlen(target_full_pathname);
	if (p->target_full_pathname == NULL) {
		p->target_full_pathname = (char *)malloc(target_name_length+32);
	}
	sprintf(p->target_full_pathname,"%s",target_full_pathname);

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
	status = stat64(p->target_full_pathname,&p->statbuf);
#else // All other OSs use stat
	status = stat(p->target_full_pathname,&p->statbuf);
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
			fprintf(xgp->errout,"%s: xdd_target_existence_check: NOTICE: target number %d name %s does not exist so it will be created.\n",
				xgp->progname,
				p->my_target_number,
				p->target_full_pathname);
		} else { // Something is wrong with the file
			fprintf(xgp->errout,"%s: xdd_target_existence_check: ERROR: Could not stat target number %d name %s\n",
				xgp->progname,
				p->my_target_number,
				p->target_full_pathname);
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
		fprintf(xgp->errout,"%s: xdd_target_existence_check: WARNING: Target number %d name %s is a DIRECTORY\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname);
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
		fprintf(xgp->errout,"%s: xdd_target_existence_check: WARNING: Target number %d name %s <%lld bytes> is shorter than the the total requested transfer size <%lld bytes>\n",
			xgp->progname,
			p->my_target_number,
			p->target_full_pathname,
			(long long)p->statbuf.st_size, 
			(long long)p->target_bytes_to_xfer_per_pass);
		fflush(xgp->errout);
		}
	}
	/* Perform sanity checks */
	if (p->target_options & TO_DIO) {
		/* make sure it is a regular file, otherwise fail */
		if ( (p->statbuf.st_mode & S_IFMT) != S_IFREG) {
			fprintf(xgp->errout,"%s: xdd_target_existence_check: WARNING: Target number %d name %s must be a regular file when used with the -dio flag\n",
				xgp->progname,
				p->my_target_number,
				p->target_full_pathname);
			fflush(xgp->errout);
		}
	}
	return(0);
} // End of xdd_target_existence_check()

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                             OS-specific Open Routines                                                    */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#if  LINUX
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_linux() - Open routine for linux
 * This routine currently always returns 0. 
 * The "fd" member of the PTDS is the actual return value from the open() 
 * system call and will indicate if there was an error.
 * The errno should also be valid upon return from this subroutine.
 */
int32_t
xdd_target_open_for_os(ptds_t *p) {



	// Set the Open Flags to indicate DirectIO if requested
	if (p->target_options & TO_DIO) 
		p->target_open_flags |= O_DIRECT;
	else p->target_open_flags &= ~O_DIRECT;

	// Check to see if this is a SCSI Generic (/dev/sg) device - SPECIFIC to LINUX
	if (p->target_options & TO_DEVICEFILE) { 
		if (strncmp(p->target_full_pathname, "/dev/sg", 7) == 0 ) // is this an "sg" device? 
			p->target_options |= TO_SGIO; // Yes - turn on SGIO Mode
	}

	/* open the target */
	if (p->rwratio == 0.0) {
		if (p->target_options & TO_SGIO) {
			p->fd = open(p->target_full_pathname,p->target_open_flags|O_RDWR, 0777); /* Must open RDWR for SGIO */
			xdd_sg_set_reserved_size(p,p->fd);
			xdd_sg_get_version(p,p->fd);
		} else p->fd = open(p->target_full_pathname,p->target_open_flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		p->target_open_flags &= ~O_CREAT;
		if (p->target_options & TO_SGIO) {
			p->fd = open(p->target_full_pathname,p->target_open_flags|O_RDWR, 0777); /* Must open RDWR for SGIO  */
			xdd_sg_set_reserved_size(p,p->fd);
			xdd_sg_get_version(p,p->fd);
		} else p->fd = open(p->target_full_pathname,p->target_open_flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open(p->target_full_pathname,p->target_open_flags|O_RDWR, 0666);
	}

	return(0);

} // End of xdd_target_open_for_linux()
#endif 

#if AIX
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_aix() - Open routine for aix
 */
int32_t
xdd_target_open_for_os(ptds_t *p) {

	p->target_open_flags |= O_LARGEFILE;
	// Set the Open Flags to indicate DirectIO if requested
	if (p->target_options & TO_DIO) {
		p->target_open_flags |= O_DIRECT;
	}

	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		p->fd = open64(p->target_full_pathname,p->target_open_flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open64(p->target_full_pathname,p->target_open_flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		p->target_open_flags &= ~O_CREAT;
		p->fd = open64(p->target_full_pathname,p->target_open_flags|O_RDWR, 0666);
	}

	return(0);
} // End of xdd_target_open_for_aix()
#endif


#if SOLARIS
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_solaris() - Open routine for solaris
 */
int32_t
xdd_target_open_for_os(ptds_t *p) {
	struct stat64 statbuf; /* buffer for file statistics */

	flags |= O_LARGEFILE;
//////////////////////////////tbd //////////////////////////////////////////////
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
			/* make sure it is a regular file, otherwise fail */
			if ( (statbuf.st_mode & S_IFMT) != S_IFREG) {
				fprintf(xgp->errout,"%s: xdd_open_target<solaris>: ERROR: Target number %d name %s must be a regular file when used with the -dio flag\n",
					xgp->progname,
					p->my_target_number,
					target_full_pathname);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"%s: xdd_open_target<solaris>: ERROR: Could not set DIRECTIO flag for target number %d name %s\n",
					xgp->progname,
					p->my_target_number,
					target_full_pathname);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		fd = open64(target_full_pathname,flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		fd = open64(target_full_pathname,flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		fd = open64(target_full_pathname,flags|O_RDWR, 0666);
	}
	/* setup for DIRECTIO & perform sanity checks */
	if (p->target_options & TO_DIO) {
		status = directio(p->fd,DIRECTIO_ON);
		if (status < 0) {
			fprintf(xgp->errout,"%s: xdd_open_target<solaris>: ERROR: Could not set DIRECTIO flag for target number %d name %s\n",
					xgp->progname,
					p->my_target_number,
					p->target_full_pathname);
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
xdd_target_open_for_os(ptds_t *p) {
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
					target_full_pathname);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,target_full_pathname);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		fd = open64(target_full_pathname,flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		fd = open64(target_full_pathname,flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		fd = open64(target_full_pathname,flags|O_RDWR, 0666);
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
xdd_target_open_for_os(ptds_t *p) {
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
					target_full_pathname);
				fflush(xgp->errout);
				return(-1);
			}
		i = directio(fd,DIRECTIO_ON);
		if (i < 0) {
			fprintf(xgp->errout,"(%d) %s: xdd_open_target: could not set DIRECTIO flag for: %s\n",
					p->my_target_number,xgp->progname,p->target_full_pathname);
			fflush(xgp->errout);
			perror("reason");
			return(-1);
		}
	} /* end of IF stmnt that opens with DIO */
	// Generic 64-bit UNIX open stuff - for Solaris, AIX, FREEBSD, and MacOSX
	if (p->rwratio == 0.0) {
		p->fd = open64(p->target_full_pathname,p->flags|O_WRONLY, 0666); /* write only */
	} else if (p->rwratio == 1.0) { /* read only */
		flags &= ~O_CREAT;
		p->fd = open64(p->target_full_pathname,p->flags|O_RDONLY, 0777); /* Read only */
	} else if ((p->rwratio > 0.0) && (p->rwratio < 1.0)) { /* read/write mix */
		flags &= ~O_CREAT;
		p->fd = open64(p->target_full_pathname,p->flags|O_RDWR, 0666);
	}
	return(0);
} // End of xdd_target_open_for_freebsd()
#endif

#ifdef WIN32
/*----------------------------------------------------------------------------*/
/* xdd_target_open_for_windows() - Open routine for Windows
 */
int32_t
xdd_target_open_for_os(ptds_t *p) {
	HANDLE hfile;  /* The file handle */
	LPVOID lpMsgBuf; /* Used for the error messages */
	unsigned long flags; /* Open flags */
	DWORD disp;

	// Check to see if this is a "device" target - must have \\.\ before the name
	// Otherwise, it is assumed that this is a regular file
	if (strncmp(target_full_pathname,"\\\\.\\",4) == 0) { // This is a device file *not* a regular file
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
		strcat(target_full_pathname, ".");
		strcat(target_full_pathname, p->target_extension);
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
		hfile = CreateFile(target_full_pathname, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if (p->rwratio == 0.0) { /* write only */
		hfile = CreateFile(target_full_pathname, 
			GENERIC_WRITE,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else if ((p->rwratio > 0.0) && p->rwratio < 1.0) { /* read/write mix */
		hfile = CreateFile(target_full_pathname, 
			GENERIC_WRITE | GENERIC_READ,   
			(FILE_SHARE_WRITE | FILE_SHARE_READ), 
			(LPSECURITY_ATTRIBUTES)NULL, 
			disp, 
			flags,
			(HANDLE)NULL);
	} else { /* open for read operations */
		hfile = CreateFile(target_full_pathname, 
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
			xgp->progname,(p->rwratio < 1.0)?"write":"read",target_full_pathname);
			fprintf(xgp->errout,"reason:%s",lpMsgBuf);
			fflush(xgp->errout);
			return((void *)-1);
	}
	
	pclk_now(&p->open_end_time); // Record the ending time of the open
	return(hfile);
} // End of xdd_target_open_for_windows()
#endif
