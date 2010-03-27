dnl
dnl This file is part of XDD
dnl
dnl ALL RIGHTS RESERVED.
dnl

dnl
dnl Process this file with autoconf to produce a configure script
dnl
AC_INIT(XDD,7.0.0.0)

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
		    [XFS_PACKAGE="LIBXFS_DEVEL"],
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
              [  --enable-xfs            Enable XFS preallocation support],
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
dnl Checks for library functions.
dnl
AC_CHECK_FUNCS([memset])

dnl
dnl Sets the compilation options
dnl
CONFIG_CFLAGS="-g -Wno-unused -Wall -Werror -ansi -femit-class-debug-always"
CONFIG_UNSAFE_CFLAGS="-g" 
CONFIG_LDFLAGS="-g"

AC_SUBST(CONFIG_CFLAGS, "$CONFIG_CFLAGS")
AC_SUBST(CONFIG_UNSAFE_CFLAGS, "$CONFIG_UNSAFE_CFLAGS")
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
