#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate the verbosity flags with xddcp
#

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



#
# Create a source file
#
test_dir=$XDDTEST_SOURCE_MOUNT/verbose1
test_file=$test_dir/source1
rm -rf $test_dir
mkdir -p $test_dir
rm -rf $XDDTEST_DEST_MOUNT/verbose1
rm -rf xdd*-source*-*log e2e.target.0000.csv
$XDDTEST_XDD_EXE -target $test_file -op write -reqsize 4096 -mbytes 4000 -qd 4 -datapattern random >/dev/null

#
# Test result
#
test_passes=1

#
# Perform a non-verbose transfer
#
$XDDTEST_XDDCP_EXE $xddcp_opts $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-1
rc=$?
if [ 0 -ne $rc ]; then
    echo "Failure: transfer failed: $XDDTEST_XDDCP_EXE $xddcp_opts  $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-1"
    test_passes=0
else
    ls xdd*-source1-*.log &>/dev/null
    if [ 2 -ne $? ]; then
	echo "Failure: produced a log when none requested."
	test_passes=0
    fi
fi

#
# Perform a verbose transfer (ensure logs exist)
#
ln -s $test_dir/source1 $test_dir/source2
test_file=$test_dir/source2
$XDDTEST_XDDCP_EXE $xddcp_opts -v $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-2
rc=$?
if [ 0 -ne $rc ]; then
    echo "Failure: transfer failed: $XDDTEST_XDDCP_EXE $xddcp_opts -v $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-2"
    test_passes=0
else
    ls xdd*-source2-*.log &>/dev/null
    if [ 0 -ne $? ]; then
	echo "Failure: did not produce a log when requested."
	test_passes=0
    fi
fi

#
# Perform an extra verbose transfer (ensure timestamps exist)
#
ln -s $test_dir/source2 $test_dir/source3
test_file=$test_dir/source3
$XDDTEST_XDDCP_EXE $xddcp_opts -V $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-3
rc=$?
if [ 0 -ne $rc ]; then
    echo "Failure: transfer failed: $XDDTEST_XDDCP_EXE $xddcp_opts -V $test_file $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/v1-3"
    test_passes=0
else
    ls xdd-*-source3-*.log &>/dev/null
    if [ 0 -ne $? -a -e e2e.target.0000.csv ]; then
	echo "Failure: did not produce a log when requested."
	test_passes=0
    fi
fi

#
# Perform post-transfer cleanup
#
rm -rf $test_dir
rm -rf $XDDTEST_DEST_MOUNT/verbose1

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance XDDCP4: Verbosity Test - Check: PASSED."
  exit 0
else
  echo "Acceptance XDDCP4: Verbosity Test - Check: FAILED."
  exit 1
fi
