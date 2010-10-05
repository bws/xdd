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
DEFAULT_IOP_COUNT=1024
DEFAULT_PROCESS_COUNT=1
DEFAULT_SEED=$(/bin/date +%s)
DEFAULT_THREAD_COUNT=4
DEFAULT_TIMESTAMP=$(/bin/date )
XDD_EXE=$(which xdd.Linux)
DATE_STAMP=`/bin/date +%m.%d.%y.%H.%M.%S`

#
# Print usage
#
function print_usage() {
    echo "Usage: xdd_forkthread.sh [-p PROCS -t THREADS -s SEED -o [1|0]] -f FILE"
}

function get_random_seed() {
    echo $RANDOM
}

#
# Main
#
function main() {

    #
    # Set the fork-thread parameters
    #
    local blockSize=1024
    local dioFlag=1
    local filename=""
    local ioSize=$((DEFAULT_IO_SIZE/1024))
    local iopCount=$DEFAULT_IOP_COUNT
    local processCount=$DEFAULT_PROCESS_COUNT
    local threadCount=$DEFAULT_THREAD_COUNT
    local randomSeed=$(get_random_seed)

    while getopts ":p:t:f:r:s:o:v" option; do
        case $option in
            p) 
                processCount=$OPTARG
                ;;
            t) 
                threadCount=$OPTARG
                ;;
            f) 
                filename=$OPTARG
                ;;
	    r) 
                iopCount=$OPTARG
                ;;
	    s) 
                randomSeed=$OPTARG
                ;;
	    o) 
                dioFlag=$OPTARG 
                ;;
	    v) 
                verboseFlag=1
                ;;
            \?)
                echo "INFO: Unsupported option: -$OPTARG" >/dev/stderr 
		exit 1
                ;;
        esac
    done

    # Validate input
    if [ -z "$filename" ]; then
        print_usage
        exit 1
    fi

    #
    # Calculate seek range
    #
    local fileSize=$(/usr/bin/stat -c %s $filename)
    local seekRange=$(((fileSize-ioSize)/1024))
    local totalIops=$((iopCount*threadCount))
    #
    # Perform IOP tests
    #
    FN="xdd-forkthread.reqsize.${ioSize}.blocksize.${blockSize}.numreqs.${iopCount}.qd.${threadCount}.sr.${seekRange}.ds.${DATE_STAMP}"
    $XDD_EXE -op read \
	-target $filename \
        -reqsize $ioSize \
	-blocksize $blockSize \
	-numreqs $totalIops \
        -seek random \
	-seek seed $randomSeed \
	-seek range $seekRange \
        -dio \
	-qd $threadCount \
	-heartbeat 1 \
	-heartbeat ops \
	-heartbeat pct \
	-verbose \
	-qthreadinfo \
        -ts output ${FN} \
        -csvout ${FN}.csv

    return 0
}

#
# Seed BASH's random number generator
#
RANDOM=$DEFAULT_RANDOM_SEED

#
# Invoke main
#
main $*
exit $?
