#!/bin/bash
#
# This script has the options to perform a fork thread style test
# At present, it is completely hard coded.  It could be altered to
# provide a more useful interface.  It assumes the data file exists.
#
# Note:  The default seed ensures that different random offsets are
#  chosen for each invocation.
#

#
# Global constants
#
DEFAULT_IO_SIZE=16384
DEFAULT_IOP_COUNT=100
DEFAULT_PROCESS_COUNT=1
DEFAULT_SEED=$(/bin/date +%s)
DEFAULT_THREAD_COUNT=1
DEFAULT_TIMESTAMP=$(/bin/date )
XDD_EXE=$(which xdd.Linux)


#
# Print usage
#
function print_usage() {
    echo "Usage: xdd_forkthread.sh FILE"
}

#
# Main
#
function main() {


    local dataFile=$1
    if [ -z "$dataFile" ]; then
        print_usage
        exit 1
    fi

    local fileSize=$(/usr/bin/stat -c %s $dataFile)
    local ioSize=$DEFAULT_IO_SIZE
    local iopCount=$DEFAULT_IOP_COUNT
    local targetCount=$DEFAULT_PROCESS_COUNT
    local threadCount=$DEFAULT_THREAD_COUNT
    local randomSeed=$DEFAULT_SEED
    local seekRange=$((fileSize/ioSize))

    $XDD_EXE -op read -target $dataFile \
        -reqsize $ioSize -blocksize 1 -numreqs $iopCount \
        -seek random -seek seed $randomSeed -seek range $fileSize \
        -dio -qd $threadCount \
	-heartbeat 1 -verbose -ts detailed -qthreadinfo
#        -ts output xdd-forkthread.tsout \
#        -csvout xdd-forkthread.csv

    return 0
}

#
# Invoke main
#
main $*
exit $?
