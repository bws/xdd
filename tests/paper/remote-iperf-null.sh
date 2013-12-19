#!/bin/bash

# load configuration defaults
source ./config.sh

# set option variables
NUMACMD=""
if [ "$NUMA" == 'true' ]
then
    NUMACMD="${NUMA} --cpunodebind=${NUMANODE}"
fi

CLIENTOPT="-c ${E2EDEST}"
CSVOPT="-y c"
INTERVALOPT="-i 3600"  # large interval so only the total is output
BUFLENOPT="-l $((${REQSIZE}*1024))"
NUMOPT="-n ${BYTES}"
SSHOPT="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"\
" -o BatchMode=yes"

# start destination side in background and ignore output
${NUMACMD} \
    ${IPERF} \
        -s \
    >/dev/null 2>/dev/null \
    &

# wait for destination side to start
sleep 3

# start source side on remote host using ssh and save output
CSVOUT=$(
    ${SSH} \
        ${SSHOPT} \
        ${E2ESRC} \
            ${NUMACMD} \
                ${IPERF} \
                ${CLIENTOPT} \
                ${CSVOPT} \
                ${INTERVALOPT} \
                ${BUFLENOPT} \
                ${NUMOPT} \
    | tail -1
)

# parse output values
#SIZEOUT=`cut -d , -f 8 <<<${CSVOUT}`
ELAPSEDOUT=`cut -d , -f 7 <<<${CSVOUT} | cut -d - -f 2`
BANDWIDTHOUT=$((`cut -d , -f 9 <<<${CSVOUT}`/8000000))

# output
# pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
echo ,,,${BYTES},,${ELAPSEDOUT},${BANDWIDTHOUT},,,,,${REQSIZE}
