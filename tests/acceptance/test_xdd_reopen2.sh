#!/bin/bash
#
# Acceptance test for XDD
#
# Description - closes and reopens file
# 
# Verify -reopen by checking if last test file accessed is different than last test file accessed after XDD finishes
#
# Source the test configuration environment
#
source ./test_config

# Pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name

mkdir -p $test_dir

test_file=$test_dir/data1
test_file2=$test_dir/data2

# Remove and make test files
rm $test_file
rm $test_file2
touch $test_file

$XDDTEST_XDD_EXE -op write -target $test_file -numreqs 100 -passes 2 -passdelay 5 -reopen $test_file &

