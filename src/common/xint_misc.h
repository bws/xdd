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

#ifndef XINT_MISC_H
#define XINT_MISC_H

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/param.h>

/* Some reckless renames */
//#define private static
//#define reg register

/* Define Base10 numeric constants */
#define ONE   		1LL 			/**< 10^0, as opposed to 2^0 */
#define THOUSAND 	1000LL 			/**< 10^3, as opposed to 2^10 */
#define MILLION  	1000000LL 		/**< 10^6, as opposed to 2^20 */
#define BILLION  	1000000000LL 	/**< 10^9, as opposed to 2^30 */
#define TRILLION 	1000000000000LL /**< 10^12, as opposed to 2^40 */
#define FLOAT_MILLION 	1000000.0 		/**< 10^6 as floating point */
#define FLOAT_BILLION 	1000000000.0 	/**< 10^9 as floating point */
#define FLOAT_TRILLION 	1000000000000.0 /**< 10^12 as floating point */

/* Define Base2 numeric constants */
#define LL_KILOBYTE 	1024LL 			/**< 2^10 as Long long Int */
#define LL_MEGABYTE 	1048576LL 		/**< 2^20 as Long Long Int */
#define LL_GIGABYTE 	1073741824LL 	/**< 2^30 as Long Long Int */
#define LL_TERABYTE 	1099511627776LL /**< 2^40 as Long Long Int */
#define FLOAT_KILOBYTE 	1024.0 			/**< 2^10 as floating point */
#define FLOAT_MEGABYTE 	1048576.0 		/**< 2^20 as floating point */
#define FLOAT_GIGABYTE 	1073741824.0 	/**< 2^30 as floating point */
#define FLOAT_TERABYTE 	1099511627776.0 /**< 2^40 as floating point */

/* Define some maximum values if needed */
#ifndef DOUBLE_MAX
#define DOUBLE_MAX 	1.7976931348623158e+308 /* max value of a double float*/
#endif
#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1LL)
#endif
#ifndef LONGLONG_MIN
#define LONGLONG_MIN LLONG_MIN
#endif
#ifndef LONGLONG_MAX
#define LONGLONG_MAX LLONG_MAX
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX 18446744073709551615LL
#endif
#ifndef ULONGLONG_MIN
#define ULONGLONG_MIN 0x0ULL
#endif
#ifndef ULONGLONG_MAX
#define ULONGLONG_MAX ULLONG_MAX
#endif

/*
 * Windows defines a LONGLONG type to hold long data; problem is
 * it's a floating point, rather than an integral type.  So this
 * kludge is used to convert a LONGLONG to its long long form.
 */
#ifdef WIN32 

/* Enable Intel on Windows? */
#define __INTEL__

#define LONG_LONG(LLvar) (*(long long *) &(LLvar).QuadPart)
#define pause()    /* Not defined in Windows env */
#endif /* WIN32 long long redefinition */
#endif /* ! MISC_H */

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
