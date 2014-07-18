#!/bin/bash
#
# Test the data pattern functionality of XDD
#
source ./test_config
source $XDDTEST_TESTS_DIR/acceptance/common.sh
initialize_test

#
# Test that the random data pattern isn't all zeroes
# 
generate_local_file rfile 1024
read -r -n 1024 </dev/zero
zeroes=$REPLY
read -r -n 1024 < $rfile
result=1
if [ "$REPLY" != "$zeroes" ]; then
    result=0
fi
finalize_test $result 
