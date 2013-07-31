#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Ensure that restarts occur when the transfer dies during EOF processing
# Note that the sleep time may need to be tuned depending on the storage
# and connect time.
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

if [ -n $XDDTEST_XDD_REMOTE_PATH ] ; then 
    xddcp_opts="-b $XDDTEST_XDD_REMOTE_PATH"
else
    xddcp_opts=""
fi

if [ -n $XDDTEST_XDD_LOCAL_PATH ] ; then 
    xddcp_opts="${xddcp_opts} -l $XDDTEST_XDD_LOCAL_PATH"
fi

# Perform pre-test 
src_test_dir=$XDDTEST_SOURCE_MOUNT/${test_name}
dest_test_dir=$XDDTEST_DEST_MOUNT/${test_name}
rm -rf $src_test_dir
mkdir -p $src_test_dir
ssh $XDDTEST_E2E_DEST "rm -rf $dest_test_dir"
ssh $XDDTEST_E2E_DEST "mkdir -p $dest_test_dir"

source_file=$src_test_dir/file1
dest_file=$dest_test_dir/file1

#
# Create the source file
#
$XDDTEST_XDD_EXE -target $source_file -op write -reqsize 32768 -numreqs 192 -qd 128 -datapattern random >/dev/null

#
# Start a long copy
#
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
$XDDTEST_XDDCP_EXE $xddcp_opts -a -n 1 -t 128 $source_file $XDDTEST_E2E_DEST:$dest_file &
pid=$!

#
# Kill the destination side
#
sleep 12
ssh $XDDTEST_E2E_DEST "pkill -f \"\\-op write\""

#
# Let the retry complete
#
wait $pid
rc=$?

#
# Verify result as correct
#
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
        echo "ERROR: Failure in retry1"
        echo "\tSource hash for $i: $srcHash"
        echo "\tDestination hash for $d: $destHash"
    fi
else
    echo "ERROR: XDDCP exited with: $rc"
fi

# Output test result
if [ "1" == "$test_passes" ]; then
    echo "Acceptance $test_name - Check: PASSED."
    exit 0
else
    echo "Acceptance $test_name - Check: FAILED."
    exit 1
fi
