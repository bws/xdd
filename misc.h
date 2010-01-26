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

#ifndef MISC_H
#define MISC_H
#ifdef WIN32
#define __INTEL__
#endif

/* -------- */
/* Includes */
/* -------- */
#if (LINUX || AIX || IRIX || SOLARIS || OSX || FREEBSD )
#ifndef NDEBUG /* These are only needed if Assert() expands to something */
#include <stdio.h> /* fprintf(), stderr */
#include <unistd.h> /* pause() */
#endif /* NDEBUG */
#endif
#include <limits.h> /* ULONG_MAX */
#if ( IRIX )
#include <sys/types.h>
#include <sys/param.h> /* NBPSCTR */
#endif /* IRIX */
/* ------- */
/* Renames */
/* ------- */
#define private static  /* Differentiate scope from visibility */
#define reg register /* Just an abbreviation */
/* ----- */
/* Types */
/* ----- */
typedef enum { false = 0, true } bool;
#if !(IRIX || SOLARIS )
#endif /* ! IRIX */
/* --------- */
/* Constants */
/* --------- */
#define MILLION  1000000LL /**< 10^6, as opposed to 2^20 */
#define BILLION  1000000000LL /**< 10^9, as opposed to 2^30 */
#define TRILLION 1000000000000LL /**< 10^12, as opposed to 2^40 */
#define FLOAT_MILLION 1000000.0 /**< 10^6 as floating point */
#define FLOAT_BILLION 1000000000.0 /**< 10^9 as floating point */
#define FLOAT_TRILLION 1000000000000.0 /**< 10^12 as floating point */
#ifndef DOUBLE_MAX
#define DOUBLE_MAX 1.7976931348623158e+308 /* max value of a double float*/
#define NATIONAL_DEBT DOUBLE_MAX
#endif

//-----------------------------------------------------------------
/* Windows defines these values a different way... */
// Signed 64-bit integer
#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1LL)
#endif
#if (LINUX || AIX || IRIX || SOLARIS || OSX || FREEBSD )
#ifndef LONGLONG_MIN
#define LONGLONG_MIN LLONG_MIN
#endif
#ifndef LONGLONG_MAX
#define LONGLONG_MAX LLONG_MAX
#endif
#else
#define LONGLONG_MIN _I64_MIN
#define LONGLONG_MAX _I64_MAX
#endif
// Unsigned 64-bit integer
#ifndef ULLONG_MAX
#define ULLONG_MAX 18446744073709551615LL
#endif
#ifndef ULONGLONG_MAX
#define ULONGLONG_MAX ULLONG_MAX
#endif
/* ------ */
/* Macros */
/* ------ */
/*
 * ntohll(n), htonll(h)
 *
 * Convert between network and host byte order for 64-bit long long
 * values.
 */
#ifdef __INTEL__
/*
 * Defining them in terms of each other is technically not
 * safe, but it works for the normal big/little endian cases.
 */

#if (SOLARIS || LINUX)
/*  These definitions are for SOLARIS running on an Intel platform */
#ifndef __int64
#define __int64 long long
#endif
#ifndef __int32
#define __int32 int
#endif
#endif /* SOLARIS definitions */
//---------------------------------
#ifndef ntohll
#define ntohll(n) \
	    (((__int64) ntohl((unsigned __int32) ((n) & 0xffffffff))) << 32 \
		| (__int64) ntohl((unsigned __int32) ((n) >> 32)))
#endif /* end of ntohll definition */
//---------------------------------
#ifndef htonll
#define htonll(h)   ntohll(h)
#endif
//---------------------------------
#else /* ! __INTEL__ */
#ifndef ntohll
#define ntohll(n)   (n)  /* It's a no-op for most everyone else */
#endif
#ifndef htonll
#define htonll(h)   (h)  /* It's a no-op for most everyone else */
#endif
#endif /* ! __INTEL__ */
//---------------------------------
#if WIN32 
/*
 * Windows defines a LONGLONG type to hold long data; problem is
 * it's a floating point, rather than an integral type.  So this
 * kludge is used to convert a LONGLONG to its long long form.
 */

#define LONG_LONG(LLvar) (*(long long *) &(LLvar).QuadPart)
#define pause()    /* Not defined in Windows env */
#endif /* WIN32 long long redefinition */
#endif /* ! MISC_H */

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
