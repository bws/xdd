#!/bin/bash
#
# Description - sends all heartbeat updates to specified file
#
# Verify -hb output by checking if specified file exists
#
# Source test environment
source ./test_config

# Pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

rm $test_dir/*
test_file=$test_dir/data1
touch $test_file

# -hb should output to file data2.T0000.csv
file_name=data2.T0000.csv

$XDDTEST_XDD_EXE -target $test_file -op write -reqsize 512 -numreqs 512 -hb 1 -hb output $test_dir/data2

#determine whether file exists
f_file=`find $test_dir -name $file_name`
f_file=$(echo ${f_file##/*/})

if [ "$f_file" == "$file_name" ]; then
    test_success=1
else
    test_success=0
fi

# Verify output
echo -n "Acceptance Test - $test_name : "
if [ $test_success -ge 1 ]; then
    echo "PASSED"
    exit 0
else    
    echo "FAILED"   
    exit 1
fi 

