#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate xdd E2E running over multiple NICs 
#
set -x
#
# Source the test configuration environment
#
source ./test_config
REQSIZE=4096
MBYTES=4096
QUEUE_DEPTH=4
DEST_DATA_NETWORK_1=pod11

# Perform pre-test 
echo "Beginning XDD Multi NIC Test 1 . . ."
test_source=$XDDTEST_SOURCE_MOUNT/source
echo "Multi NIC Test 1 - Making directory $test_source on $XDD_E2E_SOURCE"
ssh $XDDTEST_E2E_SOURCE "rm -rf $test_source "
ssh $XDDTEST_E2E_SOURCE "mkdir -p $test_source "
test_dest=$XDDTEST_SOURCE_MOUNT/dest
echo "Multi NIC Test 1 - Making directory $test_dest on $XDD_E2E_DEST"
ssh $XDDTEST_E2E_DEST "rm -rf $test_dest"
ssh $XDDTEST_E2E_DEST "mkdir -p $test_dest"

source_file=$test_source/multinic_test_file_source
dest_file=$test_dest/multinic_test_file_dest

#
# Create the source file
#
echo "Multi NIC Test 1 - Making test file $source_file on $XDD_E2E_SOURCE"
ssh $XDDTEST_E2E_SOURCE \
	~/bin/xdd  \
		-target $source_file \
		-op write \
		-dio \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-qd $QUEUE_DEPTH \
		-hb 1 \
		-hb bw \
		-datapattern random 
sleep 5
#
# Start a copy over two interfaces
#
echo "Multi NIC Test 1 - Starting copy from $XDD_E2E_SOURCE to $XDD_E2E_DEST"
echo "Multi NIC Test 1 - Destination side first... $XDD_E2E_DEST"
ssh $XDDTEST_E2E_DEST \
	~/bin/xdd  \
		-target $dest_file \
		-op write \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-e2e isdest \
		-hb 1 \
		-hb bw \
		-e2e destination $DEST_DATA_NETWORK_1 &
sleep 2
echo "Multi NIC Test 1 - ...Now the source side... $XDD_E2E_SOURCE"
ssh $XDDTEST_E2E_SOURCE \
	~/bin/xdd  \
		-target $source_file \
		-op read \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-e2e issource \
		-e2e destination $DEST_DATA_NETWORK_1 &

echo "Multi NIC Test 1 - WAITING FOR COMPLETION.... `date`"
wait

echo "Multi NIC Test 1 - COMPLETED! `date`"

echo "Multi NIC Test 1 - Source file looks like so:"
ssh $XDDTEST_E2E_SOURCE "ls -l $source_file "
echo "Multi NIC Test 1 - Destination file looks like so:"
ssh $XDDTEST_E2E_DEST "ls -l $dest_file"
