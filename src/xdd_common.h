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
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

#define XDD_COPYRIGHT "xdd - I/O Performance Inc., US DoE/DoD Extreme Scale Systems Center <ESSC> at Oak Ridge National Labs <ORNL> - Copyright 1992-2010\n"
#define XDD_DISCLAIMER "XDD DISCLAIMER:\n *** >>>> WARNING <<<<\n *** THIS PROGRAM CAN DESTROY DATA\n *** USE AT YOUR OWN RISK\n *** IOPERFORMANCE and/or THE AUTHORS ARE NOT LIABLE FOR\n *** >>>> ANYTHING BAD <<<<\n **** THAT HAPPENS WHEN YOU RUN THIS PROGRAM\n     ...although we will take credit for anything good that happens\n     but we are not *liable* for that either.\n\n" 

#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

#define DFLOW(str) (fprintf(stderr,str))

/* misc */
#define ONEMEG 1024*1024
#define ON 1
#define OFF 0
#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILED 0
#define MAX_TARGETS 8192
#define MAX_TARGET_NAME_LENGTH 2048
#define MAXSEM 1

/* program defines */
#define COMMENT '#'
#define DIRECTIVE '@'
#ifndef SPACE
#define SPACE ' '
#endif
#define TAB '\t'

/* Parameter defaults */
#define DEFAULT_TARGET_OPTIONS 0x0000000000000000ULL
#define DEFAULT_OUTPUT "stdout"
#define DEFAULT_ERROUT "stderr"
#define DEFAULT_ID "No ID Specified"
#define DEFAULT_OP "read"
#define DEFAULT_SETUP ""
#define DEFAULT_TARGETDIR ""
#define DEFAULT_TARGET NULL
#define DEFAULT_TIMESTAMP "ts"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_REQSIZE 128
#define DEFAULT_REQINCR 32
#define DEFAULT_FLUSHWRITE 0
#define DEFAULT_PASSES 1
#define DEFAULT_PASSOFFSET 0
#define DEFAULT_PASSDELAY 0
#define DEFAULT_STARTOFFSET 0
#define DEFAULT_START_DELAY 0
#define DEFAULT_NUM_TARGETS 0
#define DEFAULT_DATA_PATTERN_FILENAME NULL
#define DEFAULT_DATA_PATTERN "\0"
#define DEFAULT_DATA_PATTERN_LENGTH 1
#define DEFAULT_DATA_PATTERN_PREFIX "\0"
#define DEFAULT_DATA_PATTERN_PREFIX_LENGTH 1
#define DEFAULT_PREALLOCATE 0
#define DEFAULT_TIME_LIMIT 0
#define DEFAULT_REPORT_THRESHOLD 0
#define DEFAULT_THROTTLE 0.0
#define DEFAULT_VARIANCE 0.0
#define DEFAULT_RANGE (1024*1024)
#define DEFAULT_SEED 72058
#define DEFAULT_INTERLEAVE 1
#define DEFAULT_PORT 2000
#define DEFAULT_E2E_PORT 5044
#define DEFAULT_QUEUEDEPTH 1
#define DEFAULT_BOUNCE 100
#define DEFAULT_NUM_SEEK_HIST_BUCKETS 100
#define DEFAULT_NUM_DIST_HIST_BUCKETS 100
#define DEFAULT_RAW_PORT 2004
#define DEFAULT_RAW_LAG 0
#define DEFAULT_MAX_PATTERN_NAME_LENGTH 32
#define DEFAULT_MAX_ERRORS_TO_PRINT 10
#define DEFAULT_XDD_RLIMIT_STACK_SIZE (8192*1024)
#define DEFAULT_OUTPUT_FORMAT_STRING "+WHAT+PASS+TARGET+QUEUE+BYTESXFERED+OPS+ELAPSEDTIMEPASS+BANDWIDTH+IOPS+LATENCY+PERCENTCPUTIME+OPTYPE+XFERSIZEBYTES "

/* ------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------ */

#define BACKLOG SOMAXCONN /* Pending request limit for listen() */

/* Default flag values */
#define DFL_FL_SERVER	false	/* Client by default */
#define DFL_FL_PORT	2000	/* Port to use */
#define DFL_FL_COUNT	10  	/* Bounce count */
#define DFL_FL_TIME	99160##000000000000LL /* Zero means don't wait */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/* XXX *//* Windows might be limiting the range from 1024 to 5000 */
#define PORT_MAX USHRT_MAX /* Max value for a port number */

extern int errno;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
