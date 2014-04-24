#!/bin/bash

source ./functions.sh

declare -r ONEGB=1000000000
declare -r TENGB=$((10*${ONEGB}))
declare -r TWENTYGB=$((20*${ONEGB}))
declare -r FORTYGB=$((40*${ONEGB}))
declare -r EIGHTYGB=$((80*${ONEGB}))

NITER=10

OUTFILE=results.csv

HEADER="proto,pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize"

echo ${HEADER} >${OUTFILE}

for size in ${TENGB} ${TWENTYGB} ${FORTYGB} ${EIGHTYGB}
do
    for i in `seq ${NITER}`
    do
        echo -n "iperf," >>${OUTFILE}
        run_remote_iperf ${size} >>${OUTFILE}
        sleep 4

        echo -n "tcp," >>${OUTFILE}
        run_remote_xdd tcp ${size} >>${OUTFILE}
        sleep 4

        echo -n "xtcp," >>${OUTFILE}
        run_remote_xdd xtcp ${size} >>${OUTFILE}
        sleep 4
    done
done
