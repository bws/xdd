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
