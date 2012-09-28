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
# Test a file with two different entries
#
echo -e "td1/td2\ntd3/td4" > tcdd1-src
srcFileList="tcdd1-src"
destPath="tcdd1"
remoteFlag=1
destHost="localhost"
destIP=127.0.0.1
destUser=""
create_destination_directories "${srcFileList}" "${destPath}" "${remoteFlag}" "${destHost}" "${destIP}" "${destUser}"
assert_exists "tcdd1/td1" 
assert_exists "tcdd1/td3" 
rm -rf $srcFileList $destPath

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
