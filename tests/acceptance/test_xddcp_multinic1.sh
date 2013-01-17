#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate xdd E2E running over multiple NICs 
#
###### set -x
#
# Source the test configuration environment
#
###############################################################
# Set variables to control test configuration
#
#
# Source the test configuration environment
#
source ./test_config
#
###############################################################
#
#
XDDTEST_E2E_CONTROL=$XDDTEST_E2E_SOURCE
###############################################################
REQSIZE=4096
MBYTES=4096
QUEUE_DEPTH=8
DEST_DATA_NETWORK_1=${XDDTEST_E2E_DEST}-ib0
DEST_DATA_NETWORK_2=${XDDTEST_E2E_DEST}-10g
DEST_DATA_NETWORK_3=${XDDTEST_E2E_DEST}
DEST_DATA_NETWORK_4=localhost

# Perform pre-test 
echo "Beginning XDD Multi NIC Test 1 on CONTROL machine $XDDTEST_E2E_CONTROL - DATESTAMP $DATESTAMP "
test_source=$XDDTEST_SOURCE_MOUNT/multinic/source
echo "Multi NIC Test 1 - Making directory $test_source on $XDDTEST_E2E_SOURCE"
ssh $XDDTEST_E2E_SOURCE "rm   -rf $test_source " 
ssh $XDDTEST_E2E_SOURCE "mkdir -p $test_source " 
test_dest=$XDDTEST_DEST_MOUNT/multinic/dest
echo "Multi NIC Test 1 - Making directory $test_dest on $XDDTEST_E2E_DEST" 
ssh $XDDTEST_E2E_DEST "rm   -rf $test_dest" 
ssh $XDDTEST_E2E_DEST "mkdir -p $test_dest"

source_file=$test_source/multinic_test_file_source
dest_file=$test_dest/multinic_test_file_dest
dest_output_file_prefix=$test_dest/multinic_test
destination_log=${dest_output_file_prefix}_destination.log
#
# Create the source file
#
echo "Multi NIC Test 1 - Making test file $source_file on $XDDTEST_E2E_SOURCE using $XDDTEST_XDD_EXE"
ssh $XDDTEST_E2E_SOURCE \
	$XDDTEST_XDD_EXE \
		-target $source_file \
		-op write \
		-dio \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-qd $QUEUE_DEPTH \
		-hb 1 \
		-hb bw \
		-hb etc \
		-datapattern random
sleep 2
#
# Start a copy over two interfaces
#
echo "Multi NIC Test 1 - Starting copy from $XDDTEST_E2E_SOURCE to $XDDTEST_E2E_DEST" 
echo "Multi NIC Test 1 - Destination side first... $XDDTEST_E2E_DEST"
ssh $XDDTEST_E2E_DEST \
	$XDDTEST_XDD_EXE \
		-target $dest_file \
		-op write \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-e2e isdest \
		-hb 1 \
		-hb bw \
		-hb ops \
		-hb etc \
		-qd $QUEUE_DEPTH \
		-e2e destination $DEST_DATA_NETWORK_1 \
		-e2e destination $DEST_DATA_NETWORK_2 \
		-e2e destination $DEST_DATA_NETWORK_3 \
		-e2e destination $DEST_DATA_NETWORK_4  > $destination_log 2>&1 &
sleep 5
echo "Multi NIC Test 1 - ...Now the source side... $XDDTEST_E2E_SOURCE"
ssh $XDDTEST_E2E_SOURCE \
	$XDDTEST_XDD_EXE \
		-target $source_file \
		-op read \
		-reqsize $REQSIZE \
		-mbytes $MBYTES \
		-qd $QUEUE_DEPTH \
		-e2e issource \
		-e2e destination $DEST_DATA_NETWORK_1 \
		-e2e destination $DEST_DATA_NETWORK_2 \
		-e2e destination $DEST_DATA_NETWORK_3 \
	 	-e2e destination $DEST_DATA_NETWORK_4  &

pid=$!
wait $pid
rc=$?

test_passes=0
if [ 0 -eq $rc ]; then

    #
    # Perform MD5sum check
    #
    test_passes=1
    srcHash=$(md5sum $source_file |cut -d ' ' -f 1)
    destHash=$(ssh $XDDTEST_E2E_DEST "md5sum $dest_file |cut -d ' ' -f 1")
    if [ "$srcHash" != "$destHash" ]; then
        test_passes=0
        echo "ERROR: Failure in xdd multi-nic test"
        echo "\tSource hash for $i: $srcHash"
        echo "\tDestination hash for $d: $destHash"
    fi
else
    echo "ERROR: xdd Multi-NIC Test 1 exited with: $rc"
fi
echo "source      md5sum = $srcHash"
echo "destination md5sum = $destHash"

# Perform post-test cleanup
#rm -f $source_file
#ssh $XDDTEST_E2E_DEST "rm -f $dest_file"
echo "Multi NIC Test 1 - Collecting output files from the test..."
scp $XDDTEST_E2E_DEST:${destination_log} $test_source


# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance xdd multi-nic: Test 1 - Check: PASSED."
  exit 0
else
  echo "Acceptance xdd multi-nic: Test 2 - Check: FAILED."
  exit 1
fi
