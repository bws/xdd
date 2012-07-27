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
AC_INIT(XDD,7.0.0.rc23-pre)

dnl
dnl Include useful macros
dnl
m4_include(m4/ax_cflags_aix_option.m4)
m4_include(m4/ax_cflags_gcc_option.m4)
m4_include(m4/ax_check_gnu_make.m4)
m4_include(m4/ax_mpi.m4)

dnl
dnl Check for GNU Make
dnl
AX_CHECK_GNU_MAKE()
if test x$_cv_gnu_make_command = x ; then
    AC_MSG_ERROR(No working GNU Make found.)
fi

dnl
dnl Determine source and build directories
dnl
CONFIG_BUILD_DIR=`pwd`
CONFIG_PROJECT_RELATIVE_DIR="$srcdir"
CONFIG_PROJECT_DIR=`cd $srcdir; pwd`
AC_SUBST(CONFIG_BUILD_DIR)
AC_SUBST(CONFIG_PROJECT_DIR)

dnl
dnl Check operating system
dnl
PLATFORM=`uname`
if test "$PLATFORM" = "AIX"; then
   ARFLAGS="-rcs -X 64"
   CPPFLAGS="-DAIX -D_THREAD_SAFE"
   LIBS="-lnsl -lxnet -v"
   CFLAGS=""
   dnl On IBM systems we prefer the IBM compiler
elif test "$PLATFORM" = "Linux"; then
   ARFLAGS="-rcs"
   CPPFLAGS="-DLINUX -DSG_IO -D__INTEL__ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE"
   CFLAGS=""
elif test "$PLATFORM" = "Darwin"; then
   ARFLAGS="-rcs"
   CPPFLAGS="-DDARWIN -D__INTEL__ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE"
   CFLAGS="-Werror"
else
   AC_MSG_ERROR(Unsupported build platform: $PLATFORM)
fi

dnl
dnl Ensure compiler provides C
dnl
AC_PROG_CC([xlc_r gcc])
AC_LANG_C
AC_SUBST(CC)

dnl
dnl Adjust compiler flags
dnl
if test "$CC" = "xlc_r"; then
   dnl AX_CFLAGS_AIX_OPTION([-q64])
   dnl AX_CFLAGS_AIX_OPTION([-qcpluscmt])
   dnl AX_CFLAGS_AIX_OPTION([-qthreaded])
   CFLAGS="-q64 -qcpluscmt -qthreaded"
   LDFLAGS="-q64 -qthreaded"
elif test "$CC" = "bws_clang"; then
   CFLAGS="-g"
   LDFLAGS="-g"
elif test "$CC" = "gcc"; then
   AX_CFLAGS_GCC_OPTION([-std=c99])
   AX_CFLAGS_GCC_OPTION([-ggdb])
   AX_CFLAGS_GCC_OPTION([-fno-strict-aliasing])
   AX_CFLAGS_GCC_OPTION([-fPIC])
   AX_CFLAGS_GCC_OPTION([-Wall])
else
   AC_MSG_ERROR(Unsupported compiler type: $CC)
fi

dnl
dnl Enable static linking for executables if requested
dnl
AC_ARG_ENABLE([static], [Enable static linking for executables (Default: no)],
			enable_static="$enableval", enable_static="no")
if test "x$enable_static" = "xyes"; then

   dnl
   dnl This may need to be changed for each compiler we support
   dnl
   if test "$CC" = "gcc"; then
      LDFLAGS="-static"
   elif test "$CC" = "xlc_r"; then
      LDFLAGS="-bstatic"
   fi		
fi


dnl
dnl Check for C dependency generator
dnl
depc_exists="no"
AC_PATH_PROG([depc_exists],
             [gcc], 
             AC_MSG_ERROR(GCC is required to generate C code dependencies.))
DEPC_PATH=$depc_exists
AC_SUBST(DEPC_PATH)

dnl 
dnl Check for Standard C headers
dnl
AC_HEADER_STDC

dnl
dnl Ensure required POSIX headers are supplied
dnl
AC_CHECK_HEADER([fcntl.h], 
                [], 
                AC_MSG_ERROR(Required header fcntl.h not found))
AC_CHECK_HEADER([limits.h], 
                [], 
                AC_MSG_ERROR(Required header limits.h not found))
AC_CHECK_HEADER([time.h], 
                [], 
                AC_MSG_ERROR(Required header time.h not found))
AC_CHECK_HEADER([unistd.h], 
                [], 
                AC_MSG_ERROR(Required header unistd.h not found))
AC_CHECK_HEADER([sys/param.h], 
                [], 
                AC_MSG_ERROR(Required header sys/param.h not found))
AC_CHECK_HEADER([sys/stat.h], 
                [], 
                AC_MSG_ERROR(Required header sys/stat.h not found))
AC_CHECK_HEADER([sys/time.h], 
                [], 
                AC_MSG_ERROR(Required header sys/time.h not found))
AC_CHECK_HEADER([sys/types.h], 
                [], 
                AC_MSG_ERROR(Required header sys/types.h not found))
AC_CHECK_HEADER([sys/utsname.h], 
                [], 
                AC_MSG_ERROR(Required header sys/utsname.h not found))

dnl
dnl Check for optional POSIX headers
dnl
AC_CHECK_HEADERS([sched.h], [], []) 
AC_CHECK_HEADERS([utmpx.h], [], [])

dnl
dnl Check for C library function
dnl
AC_TRY_LINK([#include <stdlib.h>],
	    [atoi("0");],
	    [],
	    AC_MSG_ERROR([No suitable C library to link against]))

dnl
dnl Check for pthreads libraries -- this depends on the compiler
dnl
if test "$CC" = "gcc"; then
   CFLAGS="$CFLAGS -pthread"
   LDFLAGS="$LDFLAGS -pthread"
elif test "$CC" = "xlc_r"; then
   LIBS="$LIBS -lpthread"
fi
AC_TRY_LINK([#include <pthread.h>],
	    [pthread_attr_init(NULL);],
	    [],
	    AC_MSG_ERROR([No suitable pthreads library to link against]))

dnl
dnl Check for pthread's pthread_barrier_t
dnl 
AC_TRY_COMPILE([#include <pthread.h>],
            [pthread_barrier_t pb;],
            CPPFLAGS="$CPPFLAGS -DHAVE_PTHREAD_BARRIER_T",
            [])

dnl
dnl Check for pthread's pthread_attr_setaffinity_np
dnl 
AC_TRY_COMPILE([#include <pthread.h>],
            [pthread_attr_setaffinity_np(0,0,0);],
            CPPFLAGS="$CPPFLAGS -DHAVE_PTHREAD_ATTR_SETAFFINITY_NP",
            [])

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_SIZE_T

dnl
dnl Checks for library functions.
dnl
AC_CHECK_FUNCS([memset])

dnl
dnl Checks for POSIX functions
dnl
AC_SEARCH_LIBS([clock_gettime], [rt],
	       [], 
	       AC_MSG_WARN(Posix function clock_gettime not found.))

AC_CHECK_FUNCS([sched_getcpu])
AC_CHECK_FUNCS([sched_setscheduler])

dnl
dnl Check for Linux CPU_SET(3)
dnl
AC_TRY_COMPILE([#include <sched.h>],
            [cpu_set_t pb;],
            CPPFLAGS="$CPPFLAGS -DHAVE_CPU_SET_T",
            [])


dnl
dnl Check for MPI compiler
dnl
AX_MPI([],,AC_MSG_WARN(MPI C compiler not found.))

dnl
dnl Check if XFS support should be enabled
dnl
xfs_header_support="no"
AC_CHECK_HEADERS([xfs/xfs.h] [xfs/libxfs.h],
                [xfs_header_support="yes"])

dnl
dnl By default, try to add XFS support, only show errors if user requests XFS
dnl
xfs_lib_support="yes"
xfs_enabled="yes"
xfs_override="no"
AC_ARG_ENABLE([xfs],
              [AS_HELP_STRING(--enable-xfs,Enable XFS preallocation support)],
              [xfs_override="$enableval"
               xfs_enabled="$enableval"], [])

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
dnl Check for Linux Magic numbers
dnl
AC_CHECK_HEADERS([linux/magic.h],
		 [AC_CHECK_DECLS(XFS_SUPER_MAGIC,
                                 [], [], [],
                                 [[#include <linux/magic.h>]])],
                 [])


dnl
dnl Check whether to enable assertions
dnl
AC_HEADER_ASSERT

dnl
dnl Ensure that XDDCP required utilities are present
dnl
xddcp_disable="no"
dirname_exists="no"
AC_PATH_PROG([dirname_exists], [dirname], [no])
if test "x$dirname_exists" = "xno" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working dirname utility.)
    xddcp_disable="yes"
fi

ssh_exists="no"
AC_PATH_PROG([ssh_exists], [ssh], [no])
if test "x$ssh_exists" = "xno" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working ssh utility.)
    xddcp_disable="yes"
fi

stat_exists="no"
AC_PATH_PROG([stat_exists], [stat], [no])
if test "x$stat_exists" = "xno" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working stat utility.)
    xddcp_disable="yes"
fi

tr_exists="no"
AC_PATH_PROG([tr_exists], [tr], [no])
if test "x$tr_exists" = "xno" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working tr utility.)
    xddcp_disable="yes"
fi

if  ! test "x$xddcp_disable" = "xyes" ; then
   AC_SUBST(XDDCP_ENABLED, "true")
else
   AC_SUBST(XDDCP_ENABLED, "false")
fi

netstat_exists="no"
AC_PATH_PROG([netstat_exists], [netstat], [no])
if test "x$netstat_exists" = "xno" ; then
dnl   AC_MSG_WARN(Some features of xddcp unavailable.  No working netstat utility installed.)
    AC_MSG_WARN(Support for xddcp disabled.  No working netstat utility.)
    xddcp_disable="yes"
fi

dnl
dnl Set the default installation prefix
dnl
install_prefix='/opt/xdd'
AC_PREFIX_DEFAULT("$install_prefix")

dnl
dnl Sets the compilation options
dnl
AC_SUBST(CONFIG_ARFLAGS, "$ARFLAGS")
AC_SUBST(CONFIG_CFLAGS, "$CFLAGS")
AC_SUBST(CONFIG_CPPFLAGS, "$CPPFLAGS")
AC_SUBST(CONFIG_LDFLAGS, "$LDFLAGS")
AC_SUBST(CONFIG_LIBS, "$LIBS")

dnl
dnl Generate output
dnl
AC_OUTPUT(Makefile)

dnl
dnl Work around platforms that only have GNU make as some name other than make
dnl
if ! test x$_cv_gnu_make_command = xmake ; then
    mv Makefile GNUmakefile
    echo [.DEFAULT:] > Makefile ;
    echo [\	$_cv_gnu_make_command \$@] >> Makefile;
else
    rm -f GNUmakefile
fi
