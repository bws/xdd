#!/bin/bash

# load configuration defaults
source ./config.sh

# default config overrides
E2ESRC=localhost
E2EDEST=localhost

# set option variables
CLIENTOPT="-c ${E2EDEST}"
CSVOPT="-y c"
INTERVALOPT="-i 3600"  # large interval so only the total is output
BUFLENOPT="-l $((${REQSIZE}*1024))"
NUMOPT="-n ${BYTES}"

# start destination side in background and ignore output
${IPERF} \
    -s \
    >/dev/null 2>/dev/null \
    &

# wait for destination side to start
sleep 3

# start source side and save output
CSVOUT=$(
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
