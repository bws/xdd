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
DEFAULT_THREAD_COUNT=128
DEFAULT_TIMESTAMP=$(/bin/date )
XDD_EXE=$(which xdd.Linux)


#
# Print usage
#
function print_usage() {
    echo "Usage: xdd_forkthread.sh FILE"
}

function get_random_seed() {
    echo $RANDOM
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

    #
    # Seed the BASH random number facility
    #
    RANDOM=$DEFAULT_RANDOM_SEED

    #
    # Set the fork-thread parameters
    #
    local fileSize=$(/usr/bin/stat -c %s $dataFile)
    local ioSize=$DEFAULT_IO_SIZE
    local iopCount=$DEFAULT_IOP_COUNT
    local processCount=$DEFAULT_PROCESS_COUNT
    local threadCount=$DEFAULT_THREAD_COUNT
    local seekRange=$((fileSize-ioSize))

    for ((i=0; i<$processCount; i++)); do

	# Set process specific fork-thread parameters
	local randomSeed=$(get_random_seed)

        $XDD_EXE -op read -target $dataFile \
            -reqsize $ioSize -blocksize 1 -numreqs $iopCount \
            -seek random -seek seed $randomSeed -seek range $seekRange \
            -dio -qd $threadCount \
	    -heartbeat 1 -verbose -ts detailed -qthreadinfo
#        -ts output xdd-forkthread.tsout \
#        -csvout xdd-forkthread.csv
    done

    return 0
}

#
# Invoke main
#
main $*
exit $?
