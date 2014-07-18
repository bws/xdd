#!/bin/bash
#
# Description - displays time of day
#
# Validate -hb tod by comparing it to current time
#
# Source test configuration environment
source ./test_config

# pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

test_file=$test_dir/data1
test_file2=$test_dir/data2
rm -f $test_dir/*

touch $test_file

declare -a rc_time

field=4
cs_time=55
run_time=5

# loops until current seconds is less than 40
while [ $cs_time -gt 40 ]; do
    cs_time=`date | cut -f 3 -d ':' | cut -f 1 -d ' '`
done

$XDDTEST_XDD_EXE -target $test_file -op write -reqsize 9999 -numreqs 9999 -runtime $run_time -hb tod -hb output $test_file2

# gets displayed date from output file
for ((j=1; j<$run_time; j++)); do
    rc_time[$j]=`cut -f $(($field*$j)) -d ',' $test_file2.T0000.csv`
done

# gets current minute, hour, day, and month
c_time=`date`
cm_time=$(echo $c_time | cut -f 4 -d  ' ' | cut -f 2 -d ':')
ch_time=$(echo $c_time | cut -f 4 -d  ' ' | cut -f 1 -d ':')
cd_time=$(echo $c_time | cut -f 1 -d ' ')
cmo_time=$(echo $c_time | cut -f 2 -d  ' ')

# gets displayed minute, hour, day, and month
for ((j=1; j<$run_time; j++)); do
    rc_time[$j]=`cut -f $(($field*$j)) -d ',' $test_file2.T0000.csv`
done

test_success=0
declare -a match=(0 0 0 0)

# compares displayed time to current time
for ((count=1; count<$run_time; count++)); do

    r_time=${rc_time[$count]}
    rm_time=$(echo $r_time | cut -f 4 -d ' ' | cut -f 2 -d ':')
    rh_time=$(echo $r_time | cut -f 4 -d ' ' | cut -f 1 -d ':')
    rd_time=$(echo $r_time | cut -f 1 -d ' ')
    rmo_time=$(echo $r_time | cut -f 2 -d ' ')

    if [ $rm_time -eq $cm_time ]; then
        match[0]=1
    fi
    if [ $rh_time -eq $ch_time ]; then
        match[1]=1
    fi
    if [ $rd_time == $cd_time ]; then
        match[2]=1
        
    fi
    if [ $rmo_time == $cmo_time ]; then
        match[3]=1
    fi
done

# adds 1 to sum if matching and 0 if not
for i in {0..3}; do
      sum=$[$sum+${match[$i]}]
done

# if all dates match then sum should equal 4 
if [ $sum -eq 4 ]; then
      test_success=1
fi

# Verify output
echo -n "Acceptance Test - $test_name : "
if [ $test_success -eq 1 ]; then
    echo "PASSED"
    exit 0
else    
    echo "FAILED"   
    exit 1
fi 
