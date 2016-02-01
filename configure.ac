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
AC_PREREQ(2.59)
AC_INIT([xdd], [7.0.0.rc-ramses3], bug@xdd.org)
AC_CONFIG_HEADER([src/compat/config.h])


dnl
dnl Include useful macros
dnl
m4_include(m4/ax_cflags_aix_option.m4)
m4_include(m4/ax_cflags_gcc_option.m4)
m4_include(m4/ax_check_gnu_make.m4)
m4_include(m4/ax_mpi.m4)
m4_include(m4/ax_python.m4)
m4_include(m4/ax_python_module.m4)

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
   dnl On IBM systems we prefer the IBM compiler
elif test "$PLATFORM" = "Linux"; then
   ARFLAGS="-rcs"
   CPPFLAGS="-DLINUX -DSG_IO -D__INTEL__ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE"
elif test "$PLATFORM" = "Darwin"; then
   ARFLAGS="-rcs"
   CPPFLAGS="-DDARWIN -D__INTEL__ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE"
else
   AC_MSG_ERROR(Unsupported build platform: $PLATFORM)
fi

dnl
dnl Ensure compiler provides C
dnl
AC_PROG_CC([xlc_r clang gcc])
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
elif test "$CC" = "clang"; then
   CFLAGS="-g -pedantic -Wall -Wextra -Wno-unused-parameter"
   LDFLAGS="-g"
elif test "$CC" = "gcc"; then
   AX_CFLAGS_GCC_OPTION([-std=c99])
   AX_CFLAGS_GCC_OPTION([-ggdb])
   AX_CFLAGS_GCC_OPTION([-fno-strict-aliasing])
   AX_CFLAGS_GCC_OPTION([-fPIC])
   AX_CFLAGS_GCC_OPTION([-Wall])
   AX_CFLAGS_GCC_OPTION([-Wextra])
   AX_CFLAGS_GCC_OPTION([-Wno-unused-parameter])
else
   AC_MSG_ERROR(Unsupported compiler type: $CC)
fi

dnl
dnl Enable static linking for executables if requested
dnl
AC_ARG_ENABLE([static], [Enable static linking for executables (Default: no)],
			enable_static="$enableval", enable_static="no")
if test "$enable_static" = "yes"; then

   dnl
   dnl This may need to be changed for each compiler we support
   dnl
   if test "$CC" = "gcc"; then
      LDFLAGS="-static"
   elif test "$CC" = "clang"; then
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
             AC_MSG_ERROR(gcc is required to generate C code dependencies.))
DEPC_PATH=$depc_exists
AC_SUBST(DEPC_PATH)

dnl
dnl Check for a GNU compatible tar program
dnl
tar_cmd=""
AC_PATH_PROGS([tar_cmd], [gnutar tar])
if test "x$tar_cmd" == "x"; then
    AC_MSG_ERROR(no suitable tar command in path.)
fi
TAR=$tar_cmd
AC_SUBST(TAR)

dnl
dnl Check for a 'patch' command
dnl
patch_cmd=""
AC_PATH_PROGS([patch_cmd], [patch])
if test "x$patch_cmd" == "x"; then
    AC_MSG_ERROR(no suitable patch command in path.)
fi
PATCH="$patch_cmd"
AC_SUBST(PATCH)

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
AC_CHECK_HEADERS([libgen.h], [], []) 
AC_CHECK_HEADERS([sched.h], [], []) 
AC_CHECK_HEADERS([utmpx.h], [], [])
AC_CHECK_HEADERS([numa.h], [], [])
AC_CHECK_HEADERS([sys/disk.h], [], [])
AC_CHECK_HEADERS([sys/ioctl.h], [], [])
AC_CHECK_HEADERS([sys/mount.h], [], [])

dnl
dnl Check for C standard library 
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
            AC_DEFINE(HAVE_PTHREAD_BARRIER_T,1,[Define if you have pthread_barrier_t type.]),
            [])

dnl
dnl Check for pthread's pthread_attr_setaffinity_np
dnl
AC_TRY_COMPILE([#include <pthread.h>],
            [pthread_attr_setaffinity_np(0,0,0);],
            AC_DEFINE(HAVE_PTHREAD_ATTR_SETAFFINITY_NP,1,[Define if you have pthread_]),
            [])

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_SIZE_T

dnl
dnl Checks for library functions.
dnl
AC_CHECK_FUNCS([memset],, AC_MSG_ERROR([Function memset not found.]))
AC_CHECK_FUNCS([random rand], break, 
               AC_MSG_ERROR([Neither random() nor rand() is defined.]))
AC_CHECK_FUNCS([initstate], break, AC_MSG_ERROR([Function initstate not found.]))
AC_CHECK_FUNCS([nanosleep],, AC_MSG_ERROR([Function nanosleep not found.]))
AC_CHECK_FUNCS([pread],, AC_MSG_ERROR([Function pread not found.]))
AC_CHECK_FUNCS([pwrite],, AC_MSG_ERROR([Function pwrite not found.]))

dnl
dnl Checks for POSIX functions
dnl
AC_SEARCH_LIBS([clock_gettime], [rt],
	       [], 
	       AC_MSG_WARN(Posix function clock_gettime not found.))
AC_CHECK_FUNCS([posix_memalign], [], 
               AC_MSG_ERROR([Function posix_memalign not found.]))
AC_CHECK_FUNCS([ioctl], [], AC_MSG_ERROR([Function ioctl not found.]))

dnl
dnl Check for useful non-standard stuff
dnl
AC_CHECK_FUNCS([valloc])
AC_CHECK_FUNCS([sched_getcpu])
AC_CHECK_FUNCS([sched_setscheduler])

dnl
dnl Search for the NUMA function numa_node_to_cpus
dnl
AC_ARG_ENABLE([numa],
              AS_HELP_STRING([--disable-numa], [Disable LibNUMA support]),
              [enable_numa="$enableval"],
              [enable_numa=yes])
if test "$enable_numa" = "yes" ; then
    AC_SEARCH_LIBS([numa_node_to_cpus], [numa], 
                   AC_DEFINE(HAVE_NUMA_NODE_TO_CPUS),
                   AC_MSG_WARN([Function numa_node_to_cpus not found.  Use --disable-numa]))

    AC_SEARCH_LIBS([numa_allocate_cpumask], [numa], 
                   AC_DEFINE(HAVE_NUMA_ALLOCATE_CPUMASK),
                   AC_MSG_WARN([Function numa_allocate_cpumask not found.  Use --disable-numa.]))
fi

dnl
dnl Check for Linux CPU_SET(3)
dnl
AC_TRY_COMPILE([#include <sched.h>],
            [cpu_set_t pb;],
            AC_DEFINE(HAVE_CPU_SET_T,1,[Define if you have the cpu_set_t type.]),
            [])

dnl
dnl Check for block device IOCTLs
dnl
AC_CHECK_DECLS(BLKGETSIZE64, [], [], [[#include <sys/mount.h>]])
AC_CHECK_DECLS(DKIOCGETBLOCKCOUNT, [], [], [[#include <sys/disk.h>]])
AC_CHECK_DECLS(DKIOCGETBLOCKSIZE, [], [], [[#include <sys/disk.h>]])

dnl
dnl Check for Infiniband headers
dnl
AC_ARG_ENABLE([ib],
              AS_HELP_STRING([--disable-ib], [Disable Infiniband support]),
              [enable_ib="$enableval"], 
              [enable_ib=yes])
if test "x$enable_ib" = "xyes" ; then
    AC_CHECK_HEADERS([infiniband/verbs.h], [], [])
    AC_SEARCH_LIBS([ibv_get_device_list], [ibverbs],
                   AC_DEFINE(HAVE_IBV_GET_DEVICE_LIST), 
                   AC_MSG_WARN([Function ibv_get_device_list not found.  Use --disable-ib.]))
fi

dnl
dnl Check for MPI compiler
dnl
AX_MPI([],,AC_MSG_WARN(MPI C compiler not found.))

dnl
dnl Enable XFS support by default
dnl
AC_ARG_ENABLE([xfs],
              AS_HELP_STRING([--enable-xfs], [Enable XFS preallocation support]),
              [enable_xfs="$enableval"],
              [enable_xfs=yes])

if test "$enable_xfs" = "yes" ; then
    AC_DEFINE(HAVE_ENABLE_XFS)
    xfs_header="no"
    AC_CHECK_HEADERS([xfs/xfs.h] [xfs/libxfs.h],
                     xfs_header="yes"; break, [])
    if test "$xfs_header" = "no" ; then
        AC_MSG_ERROR([No valid XFS header found (sometimes this is caused by missing libuuid headers).  Use --disable-xfs.])
    fi 
    AC_CHECK_DECLS([xfsctl], 
                   AC_DEFINE(HAVE_XFSCTL), 
                   AC_MSG_ERROR([Function xfsctl not found.  Use --disable-xfs.]),
                   [#if HAVE_XFS_XFS_H
                    #include <xfs/xfs.h>
                    #elif HAVE_XFS_LIBXFS_H
                    #include <xfs/libxfs.h>
                    #endif])
fi

AC_CHECK_HEADERS([xfs/libxfs.h],
		 AC_CHECK_DECLS([LIBXFS], [], [],
                                [[#include <xfs/libxfs]]),
                 [])

AC_CHECK_HEADERS([xfs/xfs.h],
		 AC_CHECK_DECLS([XFS], [], [],
                                [[#include <xfs/xfs]]),
                 [])

dnl
dnl Check for Linux Magic numbers
dnl
AC_CHECK_HEADERS([linux/magic.h],
		 AC_CHECK_DECLS(XFS_SUPER_MAGIC, [], [],
                                [[#include <linux/magic.h>]]),
                 [])


dnl
dnl Ensure that XDDCP required utilities are present
dnl
xddcp_disable="no"
dirname_exists="no"
AC_PATH_PROG([dirname_exists], [dirname], [no])
if test "$dirname_exists" = "no" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working dirname utility.)
    xddcp_disable="yes"
fi

ssh_exists="no"
AC_PATH_PROG([ssh_exists], [ssh], [no])
if test "$ssh_exists" = "no" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working ssh utility.)
    xddcp_disable="yes"
fi

stat_exists="no"
AC_PATH_PROG([stat_exists], [stat], [no])
if test "$stat_exists" = "no" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working stat utility.)
    xddcp_disable="yes"
fi

tr_exists="no"
AC_PATH_PROG([tr_exists], [tr], [no])
if test "$tr_exists" = "no" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working tr utility.)
    xddcp_disable="yes"
fi

netstat_exists="no"
AC_PATH_PROG([netstat_exists], [netstat], [no])
if test "$netstat_exists" = "no" ; then
    AC_MSG_WARN(Support for xddcp disabled.  No working netstat utility.)
    xddcp_disable="yes"
fi

if  ! test "$xddcp_disable" = "yes" ; then
   AC_SUBST(XDDCP_ENABLED, "true")
else
   AC_SUBST(XDDCP_ENABLED, "false")
fi

dnl
dnl Search for the Google test framework to see if make check should be enabled
dnl
AC_CHECK_HEADERS([gtest/gtest.h], [], [], [/opt/local/includes])

dnl
dnl Search for the SCSI Generic I/O interface
dnl
AC_CHECK_HEADERS([scsi/sg.h], [], [], [/usr/include])

dnl
dnl Check if the TCP_CONGESTION setsockopt is available
dnl
AC_CHECK_DECLS([TCP_CONGESTION], [], [], [[#include <netinet/tcp.h>]])

dnl
dnl Check for python
dnl
AX_PYTHON()

dnl
dnl Enable XFS support by default
dnl
AC_ARG_ENABLE([xddmcp],
              AS_HELP_STRING([--enable-xfs], [Enable XFS preallocation support]),
              [enable_xddmcp="$enableval"],
              [enable_xddmcp=yes])
              
dnl
dnl Check for export-controlled packages if xddmcp is enabled
dnl
if test "$enable_xddmcp" = "yes" ; then
    AC_CHECK_FILES([contrib/pycrypto-2.6.1.tar.gz], 
                   AC_SUBST(PYCRYPTO_DIST, [contrib/pycrypto-2.6.1.tar.gz]),
                   [AX_PYTHON_MODULE([Crypto], [])])
    if test "$HAVE_PYMOD_CRYPTO" = "no" ; then
        AC_MSG_WARN([XDDMCP requires PyCrypto.  Due to U.S. Export Controls, PyCrypto must be installed separately from XDD.  See README.crypto])
        AC_MSG_ERROR([Use --disable-xddmcp to build without xddmcp support.])
    fi

    AC_CHECK_FILES([contrib/ecdsa-0.11.tar.gz], 
                   AC_SUBST(ECDSA_DIST, [contrib/ecdsa-0.11.tar.gz]),
                   [AX_PYTHON_MODULE([ecdsa], [])]) 
    if test "$HAVE_PYMOD_ECDSA" = "no" ; then
        AC_MSG_WARN([XDDMCP requires ECDSA.  Due to U.S. Export Controls, PyCrypto must be installed separately from XDD.  See README.crypto])
        AC_MSG_ERROR([Use --disable-xddmcp to build without xddmcp support.])
    fi
fi

dnl
dnl If export-controlled packages don't exist, disable xddmcp
dnl
if  test "$enable_xddmcp" = "yes" ; then
   AC_SUBST(XDDMCP_ENABLED, "true")
else
   AC_SUBST(XDDMCP_ENABLED, "false")
fi

dnl
dnl Check for packages XDD requires directly
dnl
AX_PYTHON_MODULE([datetime], [missing Python module: datetime])
AX_PYTHON_MODULE([fcntl], [missing Python module: fcntl])
AX_PYTHON_MODULE([getpass], [missing Python module: getpass])
AX_PYTHON_MODULE([optparse], [missing Python module: optparse])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([platform], [missing Python module: platform])
AX_PYTHON_MODULE([SocketServer], [missing Python module: SocketServer])
AX_PYTHON_MODULE([select], [missing Python module: select])
AX_PYTHON_MODULE([socket], [missing Python module: socket])
AX_PYTHON_MODULE([subprocess], [missing Python module: subprocess])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([threading], [missing Python module: threading])
AX_PYTHON_MODULE([time], [missing Python module: time])
AX_PYTHON_MODULE([unittest], [missing Python module: unittest])

dnl
dnl Check for packages Crypto requires
dnl
AX_PYTHON_MODULE([base64], [missing Python module: base64])
AX_PYTHON_MODULE([binascii], [missing Python module: binascii])
AX_PYTHON_MODULE([distutils], [missing Python module: distutils])
AX_PYTHON_MODULE([errno], [missing Python module: errno])
AX_PYTHON_MODULE([inspect], [missing Python module: inspect])
AX_PYTHON_MODULE([math], [missing Python module: math])
AX_PYTHON_MODULE([operator], [missing Python module: operator])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([pprint], [missing Python module: pprint])
AX_PYTHON_MODULE([stat], [missing Python module: stat])
AX_PYTHON_MODULE([struct], [missing Python module: struct])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([threading], [missing Python module: threading])
AX_PYTHON_MODULE([time], [missing Python module: time])
AX_PYTHON_MODULE([types], [missing Python module: types])
AX_PYTHON_MODULE([unittest], [missing Python module: unittest])
AX_PYTHON_MODULE([warnings], [missing Python module: warnings])
#AX_PYTHON_MODULE([winrandom], [missing Python module: winrandom])

dnl
dnl Check for packages ecdsa requires
dnl
AX_PYTHON_MODULE([base64], [missing Python module: base64])
AX_PYTHON_MODULE([binascii], [missing Python module: binascii])
AX_PYTHON_MODULE([hashlib], [missing Python module: hashlib])
AX_PYTHON_MODULE([hmac], [missing Python module: hmac])
AX_PYTHON_MODULE([math], [missing Python module: math])
AX_PYTHON_MODULE([operator], [missing Python module: operator])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([random], [missing Python module: random])
AX_PYTHON_MODULE([shutil], [missing Python module: shutil])
AX_PYTHON_MODULE([subprocess], [missing Python module: subprocess])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([time], [missing Python module: time])
AX_PYTHON_MODULE([types], [missing Python module: types])
AX_PYTHON_MODULE([unittest], [missing Python module: unittest])

dnl
dnl Check for packages paramiko requires
dnl
AX_PYTHON_MODULE([setuptools], [Python installation is incomplete.  Module setuptools is not installed for $PYTHON.])
AX_PYTHON_MODULE([array], [missing Python module: array])
AX_PYTHON_MODULE([atexit], [missing Python module: atexit])
AX_PYTHON_MODULE([base64], [missing Python module: base64])
AX_PYTHON_MODULE([binascii], [missing Python module: binascii])
AX_PYTHON_MODULE([collections], [missing Python module: collections])
AX_PYTHON_MODULE([ctypes], [missing Python module: ctypes])
AX_PYTHON_MODULE([datetime], [missing Python module: datetime])
AX_PYTHON_MODULE([errno], [missing Python module: errno])
AX_PYTHON_MODULE([fnmatch], [missing Python module: fnmatch])
AX_PYTHON_MODULE([getpass], [missing Python module: getpass])
AX_PYTHON_MODULE([logging], [missing Python module: logging])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([platform], [missing Python module: platform])
AX_PYTHON_MODULE([select], [missing Python module: select])
AX_PYTHON_MODULE([signal], [missing Python module: signal])
AX_PYTHON_MODULE([socket], [missing Python module: socket])
AX_PYTHON_MODULE([stat], [missing Python module: stat])
AX_PYTHON_MODULE([struct], [missing Python module: struct])
AX_PYTHON_MODULE([subprocess], [missing Python module: subprocess])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([tempfile], [missing Python module: tempfile])
AX_PYTHON_MODULE([threading], [missing Python module: threading])
AX_PYTHON_MODULE([time], [missing Python module: time])
AX_PYTHON_MODULE([traceback], [missing Python module: traceback])
AX_PYTHON_MODULE([warnings], [missing Python module: warnings])
AX_PYTHON_MODULE([weakref], [missing Python module: weakref])
AX_PYTHON_MODULE([zlib], [missing Python module: zlib])

dnl
dnl Check for packages Pyro4 requires
dnl
AX_PYTHON_MODULE([code], [missing Python module: code])
AX_PYTHON_MODULE([errno], [missing Python module: errno])
AX_PYTHON_MODULE([functools], [missing Python module: functools])
AX_PYTHON_MODULE([hashlib], [missing Python module: hashlib])
AX_PYTHON_MODULE([hmac], [missing Python module: hmac])
AX_PYTHON_MODULE([linecache], [missing Python module: linecache])
AX_PYTHON_MODULE([logging], [missing Python module: logging])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([platform], [missing Python module: platform])
AX_PYTHON_MODULE([re], [missing Python module: re])
AX_PYTHON_MODULE([select], [missing Python module: select])
AX_PYTHON_MODULE([socket], [missing Python module: socket])
AX_PYTHON_MODULE([struct], [missing Python module: struct])
AX_PYTHON_MODULE([subprocess], [missing Python module: subprocess])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([threading], [missing Python module: threading])
AX_PYTHON_MODULE([time], [missing Python module: time])
AX_PYTHON_MODULE([traceback], [missing Python module: traceback])
AX_PYTHON_MODULE([types], [missing Python module: types])
AX_PYTHON_MODULE([uuid], [missing Python module: uuid])
AX_PYTHON_MODULE([zlib], [missing Python module: zlib])

dnl
dnl Check for packages Serpent requires
dnl
AX_PYTHON_MODULE([ast], [missing Python module: ast])
AX_PYTHON_MODULE([base64], [missing Python module: base64])
AX_PYTHON_MODULE([gc], [missing Python module: gc])
AX_PYTHON_MODULE([os], [missing Python module: os])
AX_PYTHON_MODULE([sys], [missing Python module: sys])
AX_PYTHON_MODULE([types], [missing Python module: types])

dnl
dnl Determine options to preserve metadata while performing installs with cp
dnl
if test "$PLATFORM" = "Linux"; then
   cp_preserve_opts="-dp --no-preserve=ownership"
else
   cp_preserve_opts="-p"
fi
AC_SUBST(CP_PRESERVE_OPTS, "$cp_preserve_opts")

dnl
dnl Set the default installation prefix
dnl
install_prefix='/opt/xdd'
AC_PREFIX_DEFAULT("$install_prefix")

dnl
dnl As a very last step, turn on options that improve debugging:
dnl   make warnings trigger errors.  
dnl
dnl This has to be dnl done after all checks because it may cause
dnl autoconf checks to fail
dnl
AC_ARG_ENABLE([debug], [Enable options supporting debug (Default: no)],
			enable_debug="$enableval", enable_debug="no")
if test "$enable_debug" = "yes"; then
    if test "$CC" = "clang"; then
        CFLAGS="$CFLAGS -Werror"
    elif test "$CC" = "gcc"; then
        AX_CFLAGS_GCC_OPTION([-Werror])
    fi
    dnl Only works with recent-ish versions of autoconf
    dnl AC_HEADER_ASSERT
fi

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
AC_CONFIG_FILES([src/tools/xddmcp/xddmcp], [chmod +x src/tools/xddmcp/xddmcp])
AC_CONFIG_FILES([src/tools/xddmcp/xddmcp-server], [chmod +x src/tools/xddmcp/xddmcp-server])
AC_CONFIG_FILES([src/tools/xddprof/xddprof], [chmod +x src/tools/xddprof/xddprof])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl
dnl Work around platforms that only have GNU make as some name other than make
dnl
if ! test "$_cv_gnu_make_command" = "make" ; then
    mv Makefile GNUmakefile
    echo [.DEFAULT:] > Makefile ;
    echo [\	$_cv_gnu_make_command \$@] >> Makefile;
else
    rm -f GNUmakefile
fi
