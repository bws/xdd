#!/bin/sh 
#
# Acceptance test for XDD.
#
# Validate the funtionality of -runtime option by reading test file forever untill terminated by -runtime
#
# Description - terminates XDD after a given amount of seconds have passed
#
# Source the test configuration environment
#
source ./test_config

# Perform pre-test 
echo "Beginning Runtime Acceptance Test  . . ."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
rm -rf $test_dir
mkdir -p $test_dir

# ReqSize 4096, Bytes 1GB, Targets 1, QueueDepth 4, Passes 1
data_file=$test_dir/data1
# write a file
$XDDTEST_XDD_EXE -op write -reqsize 4096 -mbytes    1024 -targets 1 $data_file -qd 4                -passes 1 -datapattern random 
# now read forever, small random I/O  with a runtime
$XDDTEST_XDD_EXE -op read  -reqsize    1 -mbytes   16384 -targets 1 $data_file -qd 4 -runtime 10.0  -passes 1 -seek random -seek range 1024 &
pid=$!
ppid=$$
echo "xdd started, pid=$pid"
echo "sleep 30"
sleep 30

# Validate output
test_passes=1
pkill -P $ppid $pid
if [ $? -eq 0 ]; then
   test_passes=0
  echo "Had to kill $pid."
fi

# Perform post-test cleanup
#rm -rf $test_dir

# Output test result
echo "Acceptance Test - $test_name : \c"
if [ 1 -eq $test_passes ]; then
    echo "PASSED"
    exit 0
else
    echo "FAILED"
    exit 1
fi
