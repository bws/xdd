#!/bin/bash

#
# PATHS
#

SRCDIR=$HOME/Code/xdd
# XDD build system currently needs $BUILDDIR = $SRCDIR
BUILDDIR=$SRCDIR

AUTORECONF=autoreconf
IPERF=iperf
MAKE=make
MKDIR=mkdir
NUMACTL=numactl
SSH=ssh
UNAME=uname

XDD=$BUILDDIR/bin/xdd

#
# BUILD PARAMETERS
#

# build platform (e.g. "Darwin" for Mac OS X and "Linux" for Linux)
PLATFORM=`$UNAME`
# for now debug mode just disables failure on compiler errors
DEBUG=true

#
# NUMA parameters
#

# enable (true) or disable (false) numactl
NUMA=false
# value of numactl --cpunodebind option
NUMANODE=4

#
# NETWORK PARAMETERS
#

# host of source of file transfer
E2ESRC=localhost
# host of destination of file transfer
E2EDEST=localhost
# TCP base port (also used in IB mode for connection management)
E2EPORT=40010
# number of network threads (also number of I/O threads)
E2ETHREADS=1

# device to use for XNI InfiniBand
IBDEVICE="mlx4_0"

# TCP congestion control algorithm
#CONGESTION="htcp"

#
# I/O PARAMETERS
#

# default number of bytes to transfer
BYTES=$((10*1000000000))  # 10 GB
# I/O request block size (KiB)
REQSIZE=8192  #  (8 MiB)
