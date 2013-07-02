#!/bin/bash
#
# Acceptance test for XDD
#
# Description - waits n amount of seconds before starting each pass
# Validate -startdelay by checking if run time is greater or equal to n seconds times the amount of passes 
#
# Source the test configuration environment
#
source ./test_config

# Pre-test set-up
echo "Beginning startdelay function test."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

test_file=$test_dir/data1
touch $test_file



# Determine correct and elapsed time
num_passes=2
start_delay=4
correct_time=$(($num_passes*$start_delay))
xdd_cmd="$XDDTEST_XDD_EXE -target $test_file -op write -reqsize 1024 -numreqs 10 -passes $num_passes -startdelay $start_delay"
timed_pass_output="$(2>&1 time -p $xdd_cmd)"
elapsed_time=$(echo "$timed_pass_output" |grep '^real' |awk '{print $2}')

# Truncate elapsed_time
elapsed_time=$(echo $elapsed_time| cut -d '.' -f 1)


# Verify output 
echo -n "Acceptance Test - $test_name : "
if [ "$elapsed_time" -ge "$correct_time" ]; then
	echo "PASSED"
	exit 0
else	
	echo "FAILED"	
	exit 1
fi 
