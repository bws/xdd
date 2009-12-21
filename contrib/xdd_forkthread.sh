#!/bin/sh
#
# This script has the options to perform a fork thread style test
# At present, it is completely hard coded.  It could be altered to
# provide a more useful interface.  It assumes the data file exists.
#
# Note:  The seed argument ensures that different random offsets are
#  chosen for each invocation.
#
xdd_exe=xdd.Linux
data_file=/data/xfs/${USER}/ft.file

$xdd_exe -op read \
     -targets 1 $data_file \
     -targets 1 $data_file \
     -blocksize 16384 \
     -reqsize 1 \
     -seek range 16384000 \
     -seek random \
     -seek seed `date +%s` \
     -verbose \
     -heartbeat 1 \
     -numreqs 8192 \
     -verify contents \
     -qd 4 \
     -ts detailed \
     -dio \
     -targetoffset 317 \
     -qthreadinfo \
     -ts output xdd.ft.dio \
     -csvout xdd.ft.dio.csv
