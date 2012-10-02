#!/bin/bash

rc=0
function assert_equals {
    if [ "$1" != "$2" ]; then
        echo "Assert failed: $1 != $2"
        echo "Call stack: $FUNCNAME"
        for ((i=0; i < ${#FUNCNAME[*]}; i=$i+1)); do
       	    echo "INFO: Call Stack ${FUNCNAME[$i]}"
        done
        rc=1
    fi
}

function assert_exists {
    if [ ! -x "$1" ]; then
        echo "Assert failed: $1 does not exist"
        echo "Call stack: $FUNCNAME"
        for ((i=0; i < ${#FUNCNAME[*]}; i=$i+1)); do
       	    echo "INFO: Call Stack ${FUNCNAME[$i]}"
        done
        rc=1
    fi
}

#
# Source the script to get access to the functions
#
XDDCP_SCRIPT=$HOME/workspace/xdd/contrib/xddcp
exit() { :; }
source $XDDCP_SCRIPT &>/dev/null
unset -f exit


#
# Test variables
#
srcFileList=""
destPath=""
remoteFlag=""
destHost=""
destIP=""
destUser=""

#
# Test a file with two different relative paths
#
function test1 {
    echo -e "td1/td2\ntd3/td4" > tcdd1-src
    srcFileList="tcdd1-src"
    srcPath="."
    destPath="${PWD}/tcdd1"
    remoteFlag=1
    destHost="localhost"
    destIP=127.0.0.1
    destUser=""
    create_destination_directories "${srcFileList}" "${srcPath}" "${destPath}" "${remoteFlag}" "${destHost}" "${destIP}" "${destUser}"
    assert_exists "${PWD}/tcdd1/td1" 
    assert_exists "${PWD}/tcdd1/td3" 
    rm -rf $srcFileList $destPath
}
test1

#
# Test a file with two different absolute paths
#
function test2 {
    echo -e "/data/xfs/td10/td20\n/data/xfs/td30/td40" > tcdd2-src
    srcFileList="tcdd2-src"
    srcPath="/data/xfs"
    destPath="${PWD}/tcdd2"
    remoteFlag=1
    destHost="localhost"
    destIP=127.0.0.1
    destUser=""
    create_destination_directories "${srcFileList}" "${srcPath}" "${destPath}" "${remoteFlag}" "${destHost}" "${destIP}" "${destUser}"
    assert_exists "${PWD}/tcdd2/td10" 
    assert_exists "${PWD}/tcdd2/td30" 
    rm -rf $srcFileList $destPath
}
test2

#
# Test a file with two different absolute paths, but root in common
#
function test3 {
    echo -e "/data/xfs/td1/td2\n/data/xfs/td3/td4" > tcdd3-src
    srcFileList="tcdd3-src"
    srcPath="/data"
    destPath="${PWD}/tcdd3"
    remoteFlag=1
    destHost="localhost"
    destIP=127.0.0.1
    destUser=""
    create_destination_directories "${srcFileList}" "${srcPath}" "${destPath}" "${remoteFlag}" "${destHost}" "${destIP}" "${destUser}"
    assert_exists "${PWD}/tcdd3/xfs/td1" 
    assert_exists "${PWD}/tcdd3/xfs/td3" 
    rm -rf $srcFileList $destPath
}
test3

#
# Test a file with two duplicate destination paths
#

#
# Report test results
#
if [ 0 -eq $rc ]; then
    echo "SUCCESS - all tests passed."
else
    echo "FAILURE - some tests failed."
fi
exit $rc
