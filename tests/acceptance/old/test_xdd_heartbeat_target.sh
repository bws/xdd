#!/bin/bash
#
# Description - displays the target number during each output
#
# Verify -tgt by checking if target number displayed matches target number tested
#
# Source test environment
source ./test_config

# pre-test set-up
echo "Beginning heartbeat utility test."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

test_file=$test_dir/data1
test_file2=$test_dir/data2
rm $test_dir/*
touch $test_file
touch $test_file2

$XDDTEST_XDD_EXE -targets 2 $test_file $test_file2 -op target 0 write -op target 1 write -reqsize 512 -numreqs 512 -hb tgt -hb output $test_dir/data3

# get target numbers
target=`grep -m1 tgt $test_dir/data3.T0000.csv`
target_num=$(echo $target |cut -f 4 -d ',' | cut -c 4)

target2=`grep -m1 tgt $test_dir/data3.T0001.csv`
target_num2=$(echo $target2 |cut -f 4 -d ',' |cut -c 4)

# Verify output
echo -n "Acceptance Test - $test_name : "
if [ $target_num -eq 0 -a $target_num2 -eq 1 ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED"
    exit 1
fi
