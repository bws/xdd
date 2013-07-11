#!/bin/bash
#
#
# Source the test configuration environment
#
source ./test_config

test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir
rm $test_dir/*
test_file=$test_dir/data1
touch $test_file

run_time=5

sleep .5
$XDDTEST_XDD_EXE -target $test_file -reqsize 9999 -numreqs 9999 -runtime $run_time -hb lf -hb output $test_dir/data2

lines=`wc -l $test_dir/data2.T0000.csv`
match=$(echo $lines | cut -f 1 -d ' ')

echo -n "Acceptance Test - $test_name : "
if [ $match -eq $run_time ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED" 
    exit 1
fi
