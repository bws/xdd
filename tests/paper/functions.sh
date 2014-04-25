#!/bin/bash

# load configuration defaults
source ./config.sh

# Arguments: message
abort () {
    echo "abort: $1" >&2
    exit 101
}

# Arguments: transport [bytes] [reqsize]
# transport can be tcp, xtcp, or ib
run_remote_xdd () {
    # set option variables
    NUMACMD=""
    if [ "$NUMA" == 'true' ]
    then
        NUMACMD="${NUMACTL} --cpunodebind=${NUMANODE}"
    fi

    if [ "$1" = 'tcp' ]
    then
        local XNIOPT=''
    elif [ "$1" = 'xtcp' ]
    then
        local XNIOPT='-xni tcp'
    elif [ "$1" = 'ib' ]
    then
        local XNIOPT='-xni ib'
        local IBDEVICEOPT="-ibdevice ${IBDEVICE}"
    else
        abort "unknown transport '$1'"
    fi

    if [ -n "$2" ]
    then
        local BYTES="$2"
    fi

    if [ -n "$3" ]
    then
        local REQSIZE="$3"
    fi

    local TARGETOPT="-targets 1 null"  # reads/writes are no-ops
    local E2EOPT="-e2e dest ${E2EDEST}:${E2EPORT},${E2ETHREADS}"
    [ -n "${CONGESTION}" ] && local CONGESTIONOPT="-congestion ${CONGESTION}"
    local BYTESOPT="-bytes ${BYTES}"
    local REQSIZEOPT="-reqsize ${REQSIZE}"
    local SSHOPT="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"\
" -o BatchMode=yes"

    # start destination side in background and ignore output
    ${NUMACMD} \
        ${XDD} \
            ${XNIOPT} \
            ${IBDEVICEOPT} \
            ${TARGETOPT} \
            -op write -e2e isdest \
            ${E2EOPT} \
            ${CONGESTIONOPT} \
            ${BYTESOPT} \
            ${REQSIZEOPT} \
    >/dev/null \
    &

    # wait for destination side to start
    sleep 3

    # start source side on remote host using ssh and output
    # pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
    ${SSH} \
        ${SSHOPT} \
        ${E2ESRC} \
        ${NUMACMD} \
            ${XDD} \
                ${XNIOPT} \
                ${IBDEVICEOPT} \
                ${TARGETOPT} \
                -op read -e2e issrc \
                ${E2EOPT} \
                ${CONGESTIONOPT} \
                ${BYTESOPT} \
                ${REQSIZEOPT} \
    | grep -o 'COMBINED .*' \
    | sed 's/  */,/g' \
    | cut -d ',' -f 2-13
}

function ceiling() {
  local dividend=$1
  local divisor=$2
  local result=$(( (${dividend} - 1)/${divisor} + 1))
  echo ${result}
}

# Arguments: [bytes] [reqsize]
run_remote_iperf () {
    # ensure that Iperf can understand our command line and that we
    # can understand Iperf's output
    local VERSION=`${IPERF} --version 2>&1 | cut -d ' ' -f 1-3`
    if [ "$VERSION" != 'iperf version 2.0.5' ]
    then
        abort "unsupported iperf: ${VERSION}"
    fi


    # set option variables
    local NUMACMD=""
    if [ "$NUMA" == 'true' ]
    then
        NUMACMD="${NUMACTL} --cpunodebind=${NUMANODE}"
    fi

    if [ -n "$1" ]
    then
        local BYTES="$1"
    fi

    if [ -n "$2" ]
    then
        local REQSIZE="$2"
    fi

    local CLIENTOPT="-c ${E2EDEST}"
    local CSVOPT="-y c"
    local BUFLENOPT="-l $((${REQSIZE}*1024))"
    local NUMOPT="-n `ceiling ${BYTES} ${E2ETHREADS}`"
    local PORTOPT="-p ${IPERFPORT}"
    local PARALLELOPT="-P ${E2ETHREADS}"
    [ -n "${CONGESTION}" ] && local CONGESTIONOPT="-Z ${CONGESTION}"
    local SSHOPT="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"\
" -o BatchMode=yes"

    # start destination side in background and ignore output
    ${NUMACMD} \
        ${IPERF} \
        -s \
        ${PORTOPT} \
        ${CONGESTIONOPT} \
    >/dev/null 2>/dev/null \
    &
    local IPERFPID=$!

    # wait for destination side to start
    sleep 3

    # start source side on remote host using ssh and save output
    local CSVOUT=$(
        ${SSH} \
            ${SSHOPT} \
            ${E2ESRC} \
            ${NUMACMD} \
            ${IPERF} \
            ${CLIENTOPT} \
            ${PORTOPT} \
            ${CSVOPT} \
            ${BUFLENOPT} \
            ${NUMOPT} \
            ${PARALLELOPT} \
            ${CONGESTIONOPT} \
        | tail -1
    )

    # parse output values
    local ELAPSEDOUT=`cut -d , -f 7 <<<${CSVOUT} | cut -d - -f 2`
    local BANDWIDTHOUT=$((`cut -d , -f 9 <<<${CSVOUT}`/8000000))

    # output
    # pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
    echo ,,,${BYTES},,${ELAPSEDOUT},${BANDWIDTHOUT},,,,,${REQSIZE}

    # cleanup by killing iperf server
    kill "${IPERFPID}"
}
