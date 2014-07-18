#!/bin/bash
#
# Acceptance test for XDD
#
# Description - Truncate the file to a given length before doing I/O
#
# Validate -pretruncate by checking if the test file size is equal to the
#  truncation size
#
# Source the test configuration environment
#
source ./test_config

# Pre-test create test directory and file
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
rm -rf $test_dir
mkdir -p $test_dir

test_file=$test_dir/data1

# Preallocate test file
pretruncate_size=4096
req_size=1

$XDDTEST_XDD_EXE -target $test_file -op write -reqsize $req_size -numreqs 1 -pretruncate $pretruncate_size 

file_size=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d " ")

test_success=0
if [ $file_size -eq $pretruncate_size ]; then
     test_success=1
else 
     echo "File size is $file_size, but pretruncate size was $pretruncate_size"
fi

# Verify output
echo -n "Acceptance test - $test_name : "
if [ 1 -eq $test_success ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED"
    exit 1
fi
