#!/bin/bash
#
# Pre-test set-up
#
# Source test configuration enivornment
source ./test_config
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir
rm $test_dir/*
test_file=$test_dir/data1

req_size=400
num_reqs=600

$XDDTEST_XDD_EXE -target $test_file -op write -runtime 5 -numreqs $num_reqs -reqsize $req_size -hb ops -hb iops -hb lf -hb output $test_dir/data2 &

process=1
i=1 
while [ -n "$process" ]; do
    process=$(ps | grep $test_file | grep -v grep)
    echo "loop" $process
    sleep 1                 # sleep 1 to sync with xdd since hb outputs every second
    file_size[$i]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d ' ')
    i=$(($i+1))
done

wait

# copies data2 to data3 without the empty first line 
sed 1d $test_dir/data2.T0000.csv > $test_dir/data3

line_count=$(wc $test_dir/data3)
line_count=$(echo $line_count | cut -f 1 -d ' ')

# store heartbeat output ops and iops into arrays
declare -a ops
declare -a iops
for i in $(seq 1 $line_count); do
    ops=$(sed -n "$i"p $test_dir/data3 | cut -f 4 -d ',')
    iops=$(sed -n "$i"p $test_dir/data3 | cut -f 6 -d ',')

    ops[$i]=$(echo $ops | sed 's/^0*//')            # remove leading zeros and store into array
    iops[$i]=$(echo ${iops%.*} | sed 's/^0*//')     # truncate, remove leading zeros, and store into array
done

for i in $(seq 1 $line_count); do
    echo "approx ${file_size[$i]} real $((${ops[$i]}*409600))"
done

pass_count=0
error_bound=10
for i in $(seq 1 $line_count); do
    temp=$((${ops[$i]}*409600))       # each op is 409600 bytes so convert ops to bytes

    error=$(($temp-${file_size[$i]}))
    abs_error=$(echo ${error#-})
    rel_error=$(echo "scale=2;$abs_error/$temp" | bc)
    rel_error=$(echo "$rel_error*100" | bc)
    rel_error=$(echo ${rel_error%.*})

    echo "rel $rel_error abs $abs_error"
    if [ $rel_error -lt $error_bound ]; then    
        pass_count=$(($pass_count+1))
    fi
done

echo $pass_count $line_count

# store the difference between one op number and the next into an array






