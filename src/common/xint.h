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
#ifndef XINT_H
#define XINT_H

#include "config.h"
#include "xdd_base_version.h"
#if WIN32
#include "xint_win32.h"
#elif defined(LINUX)
#include "xint_linux.h"
#elif DARWIN
#include "xint_darwin.h"
#elif FREEBSD
#include "xint_freebsd.h"
#elif SOLARIS
#include "xint_solaris.h"
#elif AIX
#include "xint_aix.h"
#elif IRIX
#include "xint_irix.h"
#endif

// #include "xint_common.h"
#include "xint_plan.h"
#include "xint_prototypes.h"
#include "xint_global_data.h"

#endif
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

