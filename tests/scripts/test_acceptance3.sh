#!/bin/sh 
#
# Acceptance test for XDD.
#
# Validate the funtionality of -timelimit option
#
#
# Source the test configuration environment
#
source ./test_config

exit_error(){
  echo "Acceptance Test 3 - -timelimit failed to terminate XDD: FAILED."
  exit 1
}

exit_success(){
  echo "Acceptance Test 3 - -timelimit termintated XDD: PASSED."
  exit 0
}

# Perform pre-test 
echo "Beginning Acceptance Test 3 . . ."
test_dir=$XDDTEST_LOCAL_MOUNT/acceptance3
rm -rf $test_dir
mkdir -p $test_dir || exit_error

# ReqSize 4096, Bytes 1GB, Targets 1, QueueDepth 4, Passes 1
data_file=$test_dir/test
# write a file
$XDDTEST_XDD_EXE -op write -reqsize 4096 -mbytes    1024 -targets 1 $data_file -qd 4                  -passes 1 -datapattern random 
# now read forever, small random I/O  with a timelimit
$XDDTEST_XDD_EXE -op read  -reqsize    1 -mbytes   16384 -targets 1 $data_file -qd 4 -timelimit 10.0  -passes 1 -seek random -seek range 1024 &
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
rm -rf $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  exit_success
else
  exit_error
fi
