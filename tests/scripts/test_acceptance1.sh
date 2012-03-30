#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate the output results of -createnewfiles on 1GB files
#

#
# Source the test configuration environment
#
source ./test_config

# Perform pre-test 
echo "Beginning Acceptance Test 1 . . ."
test_dir=$XDDTEST_LOCAL_MOUNT/acceptance1
rm -rf $test_dir
mkdir -p $test_dir

# ReqSize 4096, Bytes 1GiB, Targets 1, QueueDepth 4, Passes 4
data_file=$test_dir/test
$XDDTEST_XDD_EXE -op write -reqsize 4096 -mbytes 1024 -targets 1 $data_file -qd 4 -createnewfiles -passes 4 -datapattern random 

# Validate output
test_passes=1
correct_size=1073741824

data_files="$data_file.00000001 $data_file.00000002 $data_file.00000003 $data_file.00000004"
for f in $data_files; do 
  file_size=`stat -c %s $f`
  if [ "$correct_size" != "$file_size" ]; then
    test_passes=0
    echo "Incorrect file size for $f.  Size is $file_size but should be $correct_size."
  fi
done

# Perform post-test cleanup
rm -rf $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test 1 - Createnewfiles Size Check: PASSED."
  exit 0
else
  echo "Acceptance Test 1 - Createnewfiles Size Check: FAILED."
  exit 1
fi
