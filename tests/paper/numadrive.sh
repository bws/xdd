#!/bin/bash

source ./functions.sh

declare -r ONEGB=1000000000
declare -r TENGB=$((10*${ONEGB}))
declare -r TWENTYGB=$((20*${ONEGB}))
declare -r FORTYGB=$((40*${ONEGB}))
declare -r EIGHTYGB=$((80*${ONEGB}))

NITER=10

OUTFILE="numa-node"
if [ "$NUMA" == 'true' ]
then
    OUTFILE="${OUTFILE}${NUMANODE}"
else
    OUTFILE="${OUTFILE}default"
fi
OUTFILE="${OUTFILE}-${E2ETHREADS}stream.csv"

HEADER="proto,pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize"

echo ${HEADER} >${OUTFILE}

for reqsize in 1024 2048 4096 8192 16384
do
    for i in `seq ${NITER}`
    do
        echo -n "iperf," >>${OUTFILE}
        run_remote_iperf ${EIGHTYGB} ${reqsize} >>${OUTFILE}
        sleep 4

        echo -n "xtcp," >>${OUTFILE}
        run_remote_xdd xtcp ${EIGHTYGB} ${reqsize} >>${OUTFILE}
        sleep 4
    done
done
