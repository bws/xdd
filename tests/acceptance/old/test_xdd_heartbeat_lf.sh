#!/bin/bash
#
# Description - prints a new line after each heartbeat update
#
# Verifty -hb lf by checking if output is printed with new lines
#
# Source the test configuration environment
#
source ./test_config

# pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir
rm $test_dir/*
test_file=$test_dir/data1
touch $test_file


run_time=6
lines=$(($run_time-1))
$XDDTEST_XDD_EXE -target $test_file -reqsize 1024 -numreqs 1024 -runtime $run_time -hb lf -hb output $test_dir/data2

# get number of lines printed
head -$lines $test_dir/data2.T0000.csv >> $test_dir/data3
num_lines=`wc -l $test_dir/data3`
match=$(echo $lines | cut -f 1 -d ' ')


# verify output
echo -n "Acceptance Test - $test_name : "
if [ $match -eq $lines ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED" 
    exit 1
fi
