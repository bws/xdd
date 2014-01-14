#!/bin/bash

# load configuration defaults
source ./config.sh

# Arguments: message
abort () {
    echo "abort: $1" >&2
    exit 101
}

#TODO: NUMA
#TODO: congestion
# Arguments: transport [bytes]
# transport can be tcp, xtcp, or ib
run_local_xdd () {
    # override configuration values
    E2ESRC=localhost
    E2EDEST=localhost

    # set option variables
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

    local TARGETOPT="-targets 1 null"  # reads/writes are no-ops
    local E2EOPT="-e2e dest ${E2ESRC}:${E2EPORT},${E2ETHREADS}"
    local REQSIZEOPT="-reqsize ${REQSIZE}"
    local BYTESOPT="-bytes ${BYTES}"

    # start destination side in background and ignore output
    ${XDD} \
        ${XNIOPT} \
        ${IBDEVICEOPT} \
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
        ${IBDEVICEOPT} \
        ${TARGETOPT} \
        -op read -e2e issrc \
        ${E2EOPT} \
        ${BYTESOPT} \
        ${REQSIZEOPT} \
    | grep -o 'COMBINED .*' \
    | sed 's/  */,/g' \
    | cut -d ',' -f 2-13
}

#TODO: NUMA
#TODO: congestion
# Arguments: [bytes]
run_local_iperf () {
    # override configuration values
    E2ESRC=localhost
    E2EDEST=localhost

    # set option variables
    if [ -n "$1" ]
    then
        local BYTES="$1"
    fi

    local CLIENTOPT="-c ${E2EDEST}"
    local CSVOPT="-y c"
    local INTERVALOPT="-i 3600"  # large interval so only the total is output
    local BUFLENOPT="-l $((${REQSIZE}*1024))"
    local NUMOPT="-n ${BYTES}"

    # start destination side in background and ignore output
    ${IPERF} \
        -s \
    >/dev/null 2>/dev/null \
    &

    # wait for destination side to start
    sleep 3

    # start source side and save output
    local CSVOUT=$(
        ${IPERF} \
            ${CLIENTOPT} \
            ${CSVOPT} \
            ${INTERVALOPT} \
            ${BUFLENOPT} \
            ${NUMOPT} \
        | tail -1
    )

    # parse output values
    local ELAPSEDOUT=`cut -d , -f 7 <<<${CSVOUT} | cut -d - -f 2`
    local BANDWIDTHOUT=$((`cut -d , -f 9 <<<${CSVOUT}`/8000000))

    # output
    # pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
    echo ,,,${BYTES},,${ELAPSEDOUT},${BANDWIDTHOUT},,,,,${REQSIZE}
}

# Arguments: transport [bytes]
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

# Arguments: [bytes]
run_remote_iperf () {
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

    local CLIENTOPT="-c ${E2EDEST}"
    local CSVOPT="-y c"
    local INTERVALOPT="-i 3600"  # large interval so only the total is output
    local BUFLENOPT="-l $((${REQSIZE}*1024))"
    local NUMOPT="-n ${BYTES}"
    [ -n "${CONGESTION}" ] && local CONGESTIONOPT="-Z ${CONGESTION}"
    local SSHOPT="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"\
" -o BatchMode=yes"

    # start destination side in background and ignore output
    ${NUMACMD} \
        ${IPERF} \
        -s \
        ${CONGESTIONOPT} \
    >/dev/null 2>/dev/null \
    &

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
            ${CSVOPT} \
            ${INTERVALOPT} \
            ${BUFLENOPT} \
            ${NUMOPT} \
            ${CONGESTIONOPT} \
        | tail -1
    )

    # parse output values
    local ELAPSEDOUT=`cut -d , -f 7 <<<${CSVOUT} | cut -d - -f 2`
    local BANDWIDTHOUT=$((`cut -d , -f 9 <<<${CSVOUT}`/8000000))

    # output
    # pass,target,queue,size,ops,elapsed,bandwidth,iops,latency,cpu,op,reqsize
    echo ,,,${BYTES},,${ELAPSEDOUT},${BANDWIDTHOUT},,,,,${REQSIZE}
}
