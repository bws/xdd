#!/bin/bash

# load configuration defaults
source ./config.sh

# default config overrides
E2ESRC=localhost
E2EDEST=localhost

# set option variables
XNIOPT="-xni tcp"  # use XNI TCP
TARGETOPT="-targets 1 null"  # reads/writes are no-ops
E2EOPT="-e2e dest ${E2EDEST}:${E2EPORT},${E2ETHREADS}"
BYTESOPT="-bytes ${BYTES}"
REQSIZEOPT="-reqsize ${REQSIZE}"

# start destination side in background and ignore output
${XDD} \
    ${XNIOPT} \
    ${TARGETOPT} \
    -op write -e2e isdest \
    ${E2EOPT} \
    ${BYTESOPT} \
    ${REQSIZEOPT} \
    >/dev/null \
    &

# wait for destination side to start
sleep 3

# start source side and output
# pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
${XDD} \
    ${XNIOPT} \
    ${TARGETOPT} \
    -op read -e2e issrc \
    ${E2EOPT} \
    ${BYTESOPT} \
    ${REQSIZEOPT} \
    | grep -o 'COMBINED .*' \
    | sed 's/  */,/g' \
    | cut -d ',' -f 2-13
