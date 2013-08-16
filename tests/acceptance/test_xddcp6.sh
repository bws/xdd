#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate the retry flag with xddcp without the -a resume flag in single threaded mode
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
test_sdir=$XDDTEST_SOURCE_MOUNT/$test_name
test_ddir=$XDDTEST_DEST_MOUNT/$test_name
rm -rf $test_sdir $test_ddir
mkdir -p $test_sdir $test_ddir

source_file=$test_dir/file1

#
# Create the source file
#
source_file=$test_sdir/source1
$XDDTEST_XDD_EXE -target $source_file -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern random >/dev/null

#
# Start a long copy
#
dest_file=$test_ddir/file1
$XDDTEST_XDDCP_EXE $xddcp_opts -t 1 -n 1 $source_file $XDDTEST_E2E_DEST:$dest_file &
pid=$!

#
# Kill the destination side
#
sleep 10
ssh $XDDTEST_E2E_DEST "pkill -9 -x xdd" &>/dev/null

#
# Let the retry complete
#
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
        echo "ERROR: Failure in retry1"
        echo "\tSource hash for $i: $srcHash"
        echo "\tDestination hash for $d: $destHash"
    fi
else
    echo "ERROR: XDDCP exited with: $rc"
fi

# Perform post-test cleanup
#rm -rf $test_dir
#ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/retry1"

# Output test result
if [ "1" == "$test_passes" ]; then
    echo "Acceptance $test_name - Check: PASSED."
    exit 0
else
    echo "Acceptance $test_name - Check: FAILED."
    exit 1
fi
