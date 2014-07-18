#!/bin/bash
#
# Description - outputs total number of ops completed and current ops per second
#
# Verify -hb ops and iops by comparing current total number of ops displayed to current target file size in bytes. 
# Then compare the estimated iops obtained from ops to the actual iops
#
# Source test configuration enivornment
source ./test_config

# pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir
rm $test_dir/*

test_file=$test_dir/data1

req_size=1000
num_reqs=3000

os=`uname`

# Invoke xdd to generate a csv file heartbeat data for ops and iops.
$XDDTEST_XDD_EXE -target $test_file -op write -runtime 15 -numreqs $num_reqs -reqsize $req_size -hb ops -hb iops -hb lf -hb output $test_dir/data2 & 

process=1
i=1 
declare -a file_size

# gets target file size as xdd runs in the background
if [ "$os" == "Darwin" ]; then
    while [ -n "$process" ]; do
        process=$(ps | grep $test_file | grep -v grep)
        sleep 1                 # sleep 1 to sync with xdd since hb outputs every second
        file_size[$i]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d ' ')
        i=$(($i+1))
    done
elif [ "$os" == "Linux" ]; then
    while [ -n "$process" ]; do
        process=$(ps -e| grep -w "xdd" | grep -v grep)
        sleep 1                 # sleep 1 to sync with xdd since hb outputs every second
        file_size[$i]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d ' ')
        i=$(($i+1))

    done
fi
while [ -n "$process" ]; do
    process=$(ps | grep $test_file | grep -v grep)
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
    current_ops=$(sed -n "$i"p $test_dir/data3 | cut -f 4 -d ',')
    current_iops=$(sed -n "$i"p $test_dir/data3 | cut -f 6 -d ',')

    ops[$i]=$(echo $current_ops | sed 's/^0*//')       # remove leading zeros and store into array
    
    iops[$i]=$(echo $current_iops | sed 's/^0*//')     # remove leading zeros
    iops[$i]=$(printf "%.0f" "${iops[$i]}")    # round and store into array 
done

pass_count=0
error_bound=20      # bounds approximation error to 20 percent of actual
for i in $(seq 1 $line_count); do
    temp=$((${ops[$i]}*1024000))       # each op is 1024000 bytes so convert ops to bytes
                                       # 1024000 = bytes transfered / ops 
                                       # this is true as long as reqsize equals 1000 
    error=$(($temp-${file_size[$i]}))
    abs_error=$(echo ${error#-})
    rel_error=$(echo "scale=2;$abs_error/$temp" | bc)
    rel_error=$(echo "$rel_error*100" | bc)
    rel_error=$(echo ${rel_error%.*})

    if [ $rel_error -lt $error_bound ]; then    
        pass_count=$(($pass_count+1))
    fi
done


# get the aggregate i/o operations by dividing the current number of ops by the current line number
declare -a aggr
for i in $(seq 1 $line_count); do
    aggr[$i]=$(echo "scale=2;${ops[$i]}/$i" | bc)
done


# compare the aggregate iops derived from ops to the actual iops from output file
# only do this if number of ops matches number of bytes
error_bound=5
if [ $pass_count -eq $line_count ]; then
    for i in $(seq 1 $line_count); do
        error=$(echo "${iops[$i]}-${aggr[$i]}" | bc)
        abs_error=$(echo ${error#-})
        
        rel_error=$(echo "scale=2;$abs_error/${iops[$i]}" | bc)
        rel_error=$(echo "$rel_error*100" | bc)
        rel_error=$(echo ${rel_error%.*})

        if [ $rel_error -lt $error_bound ]; then    
            pass_count=$(($pass_count+1))
        fi
    done
fi


# Verify output
test_success=0
if [ $pass_count -eq $(($line_count*2)) ]; then
    test_success=1
fi

echo -n "Acceptance Test - $test_name : "
if [ $test_success -eq 1 ]; then
    echo "PASSED"
    exit 0
else    
    echo "FAILED"   
    exit 1
fi 
