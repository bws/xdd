#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate a zero-to-null transfer
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

#
# Try a zero -> null copy
#
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
$XDDTEST_XDDCP_EXE $xddcp_opts /dev/zero $XDDTEST_E2E_DEST:/dev/null 25600000000
rc=$?

# Check validity
test_passes=0
if [ 0 -eq $rc ]; then
    test_passes=1
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
