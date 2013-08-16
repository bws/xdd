#!/bin/bash
#
# Description - synchronizes write ops after each pass, flushing all data to the physical media
#
# Test -syncwrite by checking if number of passes equals the number of fdatasyncs
#
# Source the test configuration environment
#
source ./test_config

#skip test on non-Linux platforms
if [ `uname` != "Linux" ]; then
      echo "Acceptance Test - $test_name : SKIPPED"
      exit 0
fi

# Pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

test_file=$test_dir/data1
touch $test_file

num_passes=10

# get the amount of fdatasyncs
xdd_cmd="$XDDTEST_XDD_EXE -target $test_file -op write -numreqs 10 -passes $num_passes -syncwrite"
sys_call=$(2>&1 strace -cfq -e trace=fdatasync $xdd_cmd |grep "fdatasync")
sync_num=$(echo $sys_call |cut -f 4 -d ' ')

# Verify output
echo -n "Acceptance Test - $test_name : "
if [ $sync_num -eq $num_passes ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED"
    exit 1
fi
