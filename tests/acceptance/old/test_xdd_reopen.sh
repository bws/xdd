#!/bin/bash
#
# Acceptance test for XDD
#
# Description - closes and reopens file
#
# Verify -reopen by checking if (number of opens and closes during a pass + 1) equals number of opens and closes during the next incremented pass
#
# Source the test configuration environment
#
source ./test_config

# Skip test on non-Linux platforms
if [ `uname` != "Linux" ]; then
      echo "Acceptance Test - $test_name : SKIPPED"
      exit 0
fi

# pre-test set-up
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

# make test file
test_file=$test_dir/data1
touch $test_file

# create array to store number of opens and closes
declare -a sys_open
declare -a sys_close

# gather number of opens and closes during 2, 3, and 4 passes
# incrementing number of passes by n should also increment number of opens and closes by n
# used -f in strace to find child proccesses (opens and closes)

min_passes=2
max_passes=4

for ((num_passes=$min_passes;num_passes<=$max_passes;num_passes++)); do
      xdd_cmd="$XDDTEST_XDD_EXE -op write -target $test_file -numreqs 10 -passes $num_passes -passdelay 1 -reopen $test_file"
       
      sys_call_open=$(2>&1 strace -cfq -e trace=open $xdd_cmd |grep open| tail -1 | cut -b 32-40)
      sys_call_close=$(2>&1 strace -cfq -e trace=close $xdd_cmd |grep close |cut -b 32-40)

      sys_open[$num_passes]=$sys_call_open
      sys_close[$num_passes]=$sys_call_close
done

pass_count=0

# Check if the first element is 1 less than the next and so on for n-1 elements 
for ((i=$min_passes;i<=$(($max_passes-1));i++)); do
     
      if [ $((${sys_open[$i]}+1)) -eq ${sys_open[$(($i+1))]} ]; then
            pass_count=$(($pass_count+1))
      fi
      if [ $((${sys_close[$i]}+1)) -eq ${sys_close[$(($i+1))]} ]; then
            pass_count=$(($pass_count+1))
      fi
done

correct_count=$(($max_passes-$min_passes))
correct_count=$(($correct_count*2))

# verify output

echo -n "Acceptance Test - $test_name : "
if [ $pass_count -eq $correct_count ]; then
      echo "PASSED"
      exit 0
else
      echo "FAILED"
      exit 1
fi
 




