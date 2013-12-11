#!/bin/bash
set -x

# load configuration defaults
source ./config.sh

# set up configure options

if [ "$DEBUG" = "true" ]
then
    CONFIGUREOPTS="$CONFIGUREOPTS --enable-debug"
fi

if [ "$PLATFORM" = "Darwin" ]
then
    # NUMA, XFS, IB not supported on Mac
    CONFIGUREOPTS="--disable-numa --disable-xfs --disable-ib"
fi

# move to build directory
mkdir -p $BUILDDIR
cd $BUILDDIR

# configure
$AUTORECONF $SRCDIR
$SRCDIR/configure $CONFIGUREOPTS

# make
make
