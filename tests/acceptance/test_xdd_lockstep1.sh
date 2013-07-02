#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Do the simplest lockstep command possible
#
#
# Source the test configuration environment
#
source ./test_config

# Create the test location
test_name=$(basename $0)
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
rm -f $test_dir
mkdir -p $test_dir

# A super simple lockstep
$XDDTEST_XDD_EXE -targets 2 $test_dir/foo $test_dir/foo -op target 0 write -op target 1 read \
  -reqsize 1024 -numreqs 10 -lockstep  0 1 op 1 op 1 wait complete -verbose

# Validate output
test_passes=0
correct_size=$((1024*1024*10))
file_size=$($XDDTEST_XDD_PATH/xdd-getfilesize $test_dir/foo |cut -f 1 -d ' ')
if [ "$correct_size" == "$file_size" ]; then
    test_passes=1
else
    echo "Incorrect file size.  Size is $file_size but should be $correct_size."
fi

# Output test result
/bin/echo -n "Acceptance Test - $test_name: "
if [ "1" == "$test_passes" ]; then
  echo "PASSED."
  exit 0
else
  echo "FAILED."
  exit 1
fi
