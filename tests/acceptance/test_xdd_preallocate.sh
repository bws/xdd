#!/bin/bash
#
# Acceptance test for XDD
#
# Description - sets aside n number of bytes if on an xfs file system
#
# Validate -preallocate by checking if the test file is an xfs type system and if the preallocated space is equal to the test file size 
#
# Source the test configuration environment
#
source ./test_config

# Pre-test create test directory and file
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir

test_file=$test_dir/data1
touch $test_file

# Preallocate test file
preallocate_size=1024
req_size=1

$XDDTEST_XDD_EXE -target $test_file -op write -reqsize $req_size -numreqs 1 -preallocate $preallocate_size 

# Determine file system type and file size
is_xfs=0
xfs=$(mount | grep xfs |cut -f 3 -d ' ')
if [ $test_file != ${test_file#$xfs} ]; then
   is_xfs=1
fi





file_size=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d " ")

# Only XFS supports preallocation, so test success based on xfs_pass 
test_success=0
test_skip=0
if [ $is_xfs -eq 1 ]; then
   if [ $file_size -eq $preallocate_size ]; then
         test_success=1
   else 
         echo "XFS File size is $file_size, but preallocate size was $preallocate_size"
   fi  
# Test file is not XFS
elif [ $file_size -eq $((req_size*1024)) ]; then 
         test_success=1
         test_skip=1
         echo "File is not XFS type"
# Test file is not XFS or correct size
else
         echo "Non-XFS File size is $file_size, but request size was $(($req_size*1024))"
fi



# Verify output
echo -n "Acceptance test - $test_name : "
if [ 1 -eq $test_success ]; then
    if [ 0 -eq $test_skip ]; then
      echo "PASSED"
      exit 0
    else 
      echo "SKIPPED"
      exit 0  
    fi
else
      echo "FAILED"
      exit 1
fi
