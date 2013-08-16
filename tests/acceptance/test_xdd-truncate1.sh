#!/bin/sh 
#
# Acceptance test for XDD.
#
# Validate the truncate program
#
# Description - truncates target file to a certain size
#

#
# Test identity
#
test_name=$(basename $0)
echo "Beginning $test_name . . ."

#
# Source the test configuration environment
#
source ./test_config

#
# Create the file to truncate
#
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
rm -rf $test_dir
mkdir -p $test_dir
tfile=$test_dir/truncfile1
touch $tfile

#
# Truncate a file to a size
#
test_passes=0
$XDDTEST_XDD_PATH/xdd-truncate -s 64 $tfile
fsize1=$($XDDTEST_XDD_PATH/xdd-getfilesize $tfile  | cut -f 1 -d " ")
if [ 64 -eq "$fsize1" ]; then
    test_passes=1
fi

$XDDTEST_XDD_PATH/xdd-truncate -s 4 $tfile
fsize2=$($XDDTEST_XDD_PATH/xdd-getfilesize $tfile  | cut -f 1 -d " ")
if [ 4 -eq "$fsize2" ]; then
    test_passes=1
else
    test_passes=0
fi

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test - $test_name: PASSED."
  exit 0
else
  echo "Acceptance Test - $test_name: FAILED."
  exit 1
fi
