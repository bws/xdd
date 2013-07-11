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
      
      sys_call=$(2>&1 strace -cfq -e trace=open,close $xdd_cmd |tail -4 |grep -e 'open\|close' |cut -b 32-40,51-)
      open_num=$(echo $sys_call |cut -f 1 -d ' ') 
      close_num=$(echo $sys_call |cut -f 3 -d ' ') 
      
      open_name=$(echo $sys_call |cut -f 2 -d ' ')
      close_name=$(echo $sys_call |cut -f 4 -d ' ')

      sys_open[$num_passes]=$open_num
      sys_close[$num_passes]=$close_num
done

# Verify output 
test_success=1
for ((i=$min_passes;i<=$(($max_passes-1));i++)); do
      
      if [ $((${sys_open[$i]}+1)) -ne ${sys_open[$(($i+1))]} -o "$open_name" != "open" ]; then
            test_success=0
      fi
      if [ $((${sys_close[$i]}+1)) -ne ${sys_close[$(($i+1))]} -o "$close_name" != "close" ]; then
            test_success=0
      fi
done

echo -n "Acceptance Test - $test_name : "
if [ $test_success -eq 1 ]; then
      echo "PASSED"
      exit 0
else
      echo "FAILED"
      exit 1
fi
 




