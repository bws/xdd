dnl
dnl This program is free software; you can redistribute it and/or modify 
dnl it under the terms of the GNU General Public License as published by 
dnl the Free Software Foundation; either version 2 of the License, or 
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful, but 
dnl WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
dnl or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
dnl for more details.
dnl
dnl You should have received a copy of the GNU General Public License along 
dnl with this program; if not, write to the Free Software Foundation, Inc., 
dnl 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
dnl
dnl
dnl This file is part of XDD
dnl
dnl Copyright (C) 2010, Brad Settlemyer
dnl

dnl
dnl Process this file with autoconf to produce a configure script
dnl
AC_INIT(XDD,7.0.1.0)

dnl
dnl Determine source and build directories
dnl
CONFIG_BUILD_DIR=`pwd`
CONFIG_PROJECT_RELATIVE_DIR="$srcdir"
CONFIG_PROJECT_DIR=`cd $srcdir; pwd`
AC_SUBST(CONFIG_BUILD_DIR)
AC_SUBST(CONFIG_PROJECT_DIR)

dnl
dnl Check for programs
dnl
AC_PROG_CC
AC_LANG_C

dnl 
dnl Check for Standard C headers
dnl
AC_HEADER_STDC
AC_CHECK_HEADER([stdio.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([stdlib.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([limits.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([signal.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([errno.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([string.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))
AC_CHECK_HEADER([math.h], 
                [], 
                AC_MSG_ERROR(Must include headers for libc))

dnl
dnl Headers common to all supported platforms
dnl
AC_CHECK_HEADER([fcntl.h], 
                [], 
                AC_MSG_ERROR(Must include headers for fcntl.h))
AC_CHECK_HEADER([unistd.h], 
                [], 
                AC_MSG_ERROR(Must include headers for unistd.h))
AC_CHECK_HEADER([sys/types.h], 
                [], 
                AC_MSG_ERROR(Must include headers for sys/types.h))
AC_CHECK_HEADER([sys/stat.h], 
                [], 
                AC_MSG_ERROR(Must include headers for sys/stat.h))


dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_SIZE_T

dnl
dnl Check if XFS support should be enabled
dnl
xfs_header_support="no"
xfs_package=""
AC_CHECK_HEADER([xfs/xfs.h],
                [xfs_header_support="yes"; xfs_package="XFSPROGS_DEVEL"],
                [xfs_header_support="no"])
if test "x$xfs_header_support" = "xyes" ; then
    AC_CHECK_HEADER([xfs/libxfs.h],
		    [xfs_package="LIBXFS_DEVEL"],
                    [])
fi

xfs_magic_support="no"
if test "x$xfs_header_support" = "xyes" ; then
    AC_CHECK_HEADER([linux/magic.h],
                    [AC_CHECK_DECL(XFS_SUPER_MAGIC,
                                   [xfs_magic_support="yes"], [], [],
				   [[#include <linux/magic.h>]])],
                    [])
    
fi
dnl if test "x$xfs_header_support" = "xyes" ; then
dnl     AC_CHECK_HEADER([xfs/xfs_fs.h],
dnl                     [xfs_header_support="yes"],
dnl                     [xfs_header_support="no"])
dnl fi

xfs_lib_support="yes"
#AC_SEARCH_LIBS([xfsctl], [handle], xfs_lib_support=yes)

dnl By default, try to add XFS support, only show errors if user requests XFS
xfs_enabled="yes"
xfs_override="no"
AC_ARG_ENABLE([xfs],
              [AS_HELP_STRING(--enable-xfs,Enable XFS preallocation support)],
              [xfs_override="$enableval"
               xfs_enabled="$enableval"], [])

dnl echo $xfs_enabled $xfs_override
if test "x$xfs_override" = "xyes" ; then
    if test "x$xfs_header_support" = "xno" ; then
        AC_MSG_FAILURE("XFS headers not found.")
    elif test "x$xfs_lib_support" = "xno" ; then
        AC_MSG_ERROR("XFS libraries not found.")
    fi
    AC_SUBST(CONFIG_XFS_ENABLED, "-DXFS_ENABLED")
    AC_SUBST(CONFIG_XFS_PACKAGE, "-D$xfs_package")
elif test "x$xfs_enabled" = "xyes" ; then
    if test $xfs_header_support = yes -a $xfs_lib_support = yes ; then
        AC_SUBST(CONFIG_XFS_ENABLED, "-DXFS_ENABLED")
        AC_SUBST(CONFIG_XFS_PACKAGE, "-D$xfs_package")
    fi
fi


dnl
dnl Check for Confidence availability
dnl
confidence_headers="no"
AC_CHECK_HEADERS([libconfidence.h],
		[confidence_headers="yes"],
		[confidence_headers="no"],
		[#include "types.h"
		 #include "globalvars.h"])
confidence_libs="no"
AC_CHECK_LIB([confidence], [confidence_initialize], [confidence_libs="yes"])
confidence_enable="no"
AC_ARG_ENABLE([confidence],
	      [AS_HELP_STRING(--enable-confidence,Enable Confidence formatted output files)],
	      [confidence_enable="yes"])
if test "x$confidence_enable" = "xyes" ; then
    if test "x$confidence_headers" = "xno" ; then
    	AC_MSG_WARN("Confidence headers not found.
		Add '-I<confidence_dir>' to CFLAGS.
		Continuing without Confidence enabled...")
    fi
    if test "x$confidence_libs" = "xno" ; then
    	AC_MSG_WARN("Confidence library 'libconfidence.a' not found.
		Add '-L<confidence_dir> -lm -lconfidence' to LDFLAGS.
		Continuing without Confidence enabled...")
    fi
    if test "x$confidence_headers" = "xyes" ; then
        if test "x$confidence_libs" = "xyes" ; then
    	    AC_SUBST(CONFIG_CONFIDENCE_ENABLED, "-DCONFIDENCE_ENABLED")
	fi
    fi
fi


dnl
dnl Check for Linux Magic numbers
dnl
xfs_magic_support="no"
if test "x$xfs_header_support" = "xyes" ; then
    AC_CHECK_HEADER([linux/magic.h],
                    [AC_CHECK_DECL(XFS_SUPER_MAGIC,
                                   [xfs_magic_support="yes"], [], [],
                                   [[#include <linux/magic.h>]])],
                    [])

fi
if test "x$magic_header_support" = "xyes" ; then
    AC_SUBST(CONFIG_LINUX_MAGIC, "-DMAGIC_XFS_SUPPORT")
fi

dnl
dnl Checks for library functions.
dnl
AC_CHECK_FUNCS([memset])

dnl
dnl Sets the compilation options
dnl
CONFIG_CFLAGS="$CFLAGS"
CONFIG_LDFLAGS="$LDFLAGS $LIBS"
AC_SUBST(CONFIG_CFLAGS, "$CONFIG_CFLAGS")
AC_SUBST(CONFIG_LDFLAGS, "$CONFIG_LDFLAGS")

dnl
dnl Set the default installation prefix
dnl
install_prefix='/opt/xdd'
AC_PREFIX_DEFAULT("$install_prefix")

dnl
dnl Generate output
dnl
AC_OUTPUT(Makefile)

