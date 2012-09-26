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

#
# Source the script to get access to the functions
#
XDDCP_SCRIPT=$HOME/workspace/xdd/contrib/xddcp
exit() { :; }
source $XDDCP_SCRIPT >/dev/null
unset -f exit


#
# Test variables
#
path=""
rFlag=""
user=""
hostArray=()
addrArray=()
nodeArray=()

#
# Test a simple spec
#
spec1="user@localhost:/p1"
parse_endpoint_spec "${spec1}" path rFlag user hostArray addrArray nodeArray
assert_equals "/p1" $path
assert_equals "1" $rFlag
assert_equals "user" $user
assert_equals "localhost" ${hostArray[*]}
assert_equals "127.0.0.1" ${addrArray[*]}
assert_equals "all" ${nodeArray[*]}

#
# Test a multi-host spec
#
spec2="user2@localhost,192.168.1.1:/p2"
parse_endpoint_spec "${spec2}" path rFlag user hostArray addrArray nodeArray
assert_equals "/p2" "$path"
assert_equals "1" "$rFlag"
assert_equals "user2" "$user"
assert_equals "localhost 192.168.1.1" "${hostArray[*]}"
assert_equals "127.0.0.1 192.168.1.1" "${addrArray[*]}"
assert_equals "2" "${#hostArray[@]}"
assert_equals "2" "${#addrArray[@]}"
assert_equals "2" "${#nodeArray[@]}"

#
# Test a three host spec
#
spec3="user3@localhost,localhost,localhost:/p3"
parse_endpoint_spec "${spec3}" path rFlag user hostArray addrArray nodeArray
assert_equals "/p3" "$path"
assert_equals "1" "$rFlag"
assert_equals "user3" "$user"
assert_equals "localhost localhost localhost" "${hostArray[*]}"
assert_equals "127.0.0.1 127.0.0.1 127.0.0.1" "${addrArray[*]}"
assert_equals "3" "${#hostArray[@]}"
assert_equals "3" "${#addrArray[@]}"
assert_equals "3" "${#nodeArray[@]}"

#
# Test a three host spec with a node
#
spec4="user4@localhost,localhost%1,localhost:/p4"
parse_endpoint_spec "${spec4}" path rFlag user hostArray addrArray nodeArray
assert_equals "/p4" "$path"
assert_equals "1" "$rFlag"
assert_equals "user4" "$user"
assert_equals "localhost localhost localhost" "${hostArray[*]}"
assert_equals "127.0.0.1 127.0.0.1 127.0.0.1" "${addrArray[*]}"
assert_equals "all" "${nodeArray[0]}"
assert_equals "1" "${nodeArray[1]}"
assert_equals "all" "${nodeArray[2]}"
assert_equals "3" "${#hostArray[@]}"
assert_equals "3" "${#addrArray[@]}"
assert_equals "3" "${#nodeArray[@]}"

#
# Test a four host spec with each having a node
#
spec5="user5@localhost%2,localhost%1,localhost%3,localhost%4:/p5"
parse_endpoint_spec "${spec5}" path rFlag user hostArray addrArray nodeArray
assert_equals "/p5" "$path"
assert_equals "1" "$rFlag"
assert_equals "user5" "$user"
assert_equals "localhost localhost localhost localhost" "${hostArray[*]}"
assert_equals "127.0.0.1 127.0.0.1 127.0.0.1 127.0.0.1" "${addrArray[*]}"
assert_equals "2" "${nodeArray[0]}"
assert_equals "1" "${nodeArray[1]}"
assert_equals "3" "${nodeArray[2]}"
assert_equals "4" "${nodeArray[3]}"
assert_equals "4" "${#hostArray[@]}"
assert_equals "4" "${#addrArray[@]}"
assert_equals "4" "${#nodeArray[@]}"

#
# Test the real 40G refspec
#
spec6="bws@10.100.7.6%6,10.100.6.6%4,10.100.2.6%3,10.100.1.6%2:test1"
parse_endpoint_spec "${spec6}" path rFlag user hostArray addrArray nodeArray
assert_equals "test1" "$path"
assert_equals "1" "$rFlag"
assert_equals "bws" "$user"
assert_equals "10.100.7.6 10.100.6.6 10.100.2.6 10.100.1.6" "${hostArray[*]}"
assert_equals "10.100.7.6 10.100.6.6 10.100.2.6 10.100.1.6" "${addrArray[*]}"
assert_equals "6" "${nodeArray[0]}"
assert_equals "4" "${nodeArray[1]}"
assert_equals "3" "${nodeArray[2]}"
assert_equals "2" "${nodeArray[3]}"
assert_equals "4" "${#hostArray[@]}"
assert_equals "4" "${#addrArray[@]}"
assert_equals "4" "${#nodeArray[@]}"

#
# A popular testing spec
#
spec7="bws@localhost:t2"
parse_endpoint_spec "${spec7}" path rFlag user hostArray addrArray nodeArray
assert_equals "t2" "$path"
assert_equals "1" "$rFlag"
assert_equals "bws" "$user"
assert_equals "localhost" "${hostArray[*]}"
assert_equals "127.0.0.1" "${addrArray[*]}"
assert_equals "1" "${#hostArray[@]}"
assert_equals "1" "${#addrArray[@]}"
assert_equals "1" "${#nodeArray[@]}"
#
# Report test results
#
if [ 0 -eq $rc ]; then
    echo "SUCCESS - all tests passed."
else
    echo "FAILURE - some tests failed."
fi
exit $rc
