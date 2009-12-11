/* Copyright (C) 1992-2009 I/O Performance, Inc.
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
/* Author:
 *  Tom Ruwart (tmruwart@ioperformance.com)
 *   I/O Performance, Inc. and
 *
 */
#include "xdd_base_version.h"
#if WIN32
#include "xdd_win32.h"
#elif LINUX
#include "xdd_linux.h"
#elif OSX
#include "xdd_osx.h"
#elif FREEBSD
#include "xdd_freebsd.h"
#elif SOLARIS
#include "xdd_solaris.h"
#elif AIX
#include "xdd_aix.h"
#elif IRIX
#include "xdd_irix.h"
#endif

#include "xdd_common.h"
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
