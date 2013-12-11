#!/bin/bash

# load configuration defaults
source ./config.sh

# set option variables
XNIOPT=""  # don't use XNI
TARGETOPT="-targets 1 null"  # reads/writes are no-ops
E2EOPT="-e2e dest localhost:${E2EPORT},${E2ETHREADS}"
BYTESOPT="-bytes ${BYTES}"
REQSIZEOPT="-reqsize ${REQSIZE}"

# start destination side in background
${XDD} \
    ${XNIOPT} \
    ${TARGETOPT} \
    -op write -e2e isdest \
    ${E2EOPT} \
    ${BYTESOPT} \
    ${REQSIZEOPT} \
    &

# wait for destination side to start
sleep 3

# start source side
${XDD} \
    ${XNIOPT} \
    ${TARGETOPT} \
    -op read -e2e issrc \
    ${E2EOPT} \
    ${BYTESOPT} \
    ${REQSIZEOPT}
