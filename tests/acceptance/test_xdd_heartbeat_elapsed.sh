#!/bin/bash
#
#
#
#
# Source the test configuration environment
#
source ./test_config

# Pre-test set-up
echo "Beginning heartbeat -lf utility test."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir $test_dir
rm -r $test_dir/*
test_file=$test_dir/data1
touch $test_file

delay_time=6

$XDDTEST_XDD_EXE -target $test_file -op write -reqsize 1 -numreqs 1 -startdelay $delay_time -hb elapsed -hb output $test_dir/data2 

data_loc=$test_dir/data2.T0000.csv

for refresh in {1..5}; do 
      time_refresh=$(($refresh*4))
      run_time=$(cut $data_loc -f $time_refresh -d ',')
      echo $run_time
done

for i in {1..5};

