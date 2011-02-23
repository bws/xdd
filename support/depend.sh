#!/bin/sh
#
# Copyright (C) 2007,2009,2011 Brad Settlemyer
#
# ALL RIGHTS RESERVED
#

#
# Usage: depend.sh <file_dir> <compiler> <compiler arguments>
#

#
# Retrieve arguments
#
dir="${1}"
shift

#
# Path complement the directory
#
if [ -n "${dir}" ]; then
  dir="${dir}"/
fi

#
# Run the dependency generator, but path complement items on the *left*
# side of the colon
#
add_dir_re="s@^\(.*\)\.o:@${dir}\1.d ${dir}\1.o:@"
exec "$@" | sed -e "${add_dir_re}"
exit $?
