#!/bin/bash
#
#
#
#
# Source the test configuration environment
#
source ./test_config

# Pre-test set-up
echo "Beginning heartbeat -elapsed utility test."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name

mkdir $test_dir
rm -r $test_dir/*

test_file=$test_dir/data1
touch $test_file

data_loc=$test_dir/data2.T0000.csv
delay_time=3


$XDDTEST_XDD_EXE -target $test_file -op write -passes 3 -reqsize 1 -numreqs 1 -startdelay $delay_time -hb elapsed -hb output $test_dir/data2 


for refresh in $(seq 1 $(($delay_time-1))); do 
      time_refresh=$(($refresh*4))
      run_time=$(cut -f $time_refresh -d ',' $data_loc)
      echo $run_time
done



