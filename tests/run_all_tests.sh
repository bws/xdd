#/bin/bash
#
# Run all tests
#

#
# Source the environment
#
if [ -f ./test_config ]; then
    source ./test_config
else
    echo "ERROR: Must provide tst configuration as ./test_config" >/dev/stderr
    exit 1
fi

#
# Ensure the source and destinations are reachable
#
ping -c 2 "$XDDTEST_E2E_SOURCE"
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_SOURCE" >/dev/stderr
    exit 1
fi

ping -c 2 "$XDDTEST_E2E_DEST"
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_DEST" >/dev/stderr
    exit 2 
fi

#
# Ensure the mount points exist
#
if [ ! -w "$XDDTEST_LOCAL_MOUNT" -o ! -r "$XDDTEST_LOCAL_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_LOCAL_MOUNT" >/dev/stderr
    exit 3
fi

if [ ! -w "$XDDTEST_SOURCE_MOUNT" -o ! -r "$XDDTEST_SOURCE_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_SOURCE_MOUNT" >/dev/stderr
    exit 3
fi

ssh $XDDTEST_E2E_DEST bash <<EOF
    if [ ! -w "$XDDTEST_DEST_MOUNT" -o ! -r "$XDDTEST_DEST_MOUNT" ]; then
        exit 1
    fi
EOF
if [ $? != 0 ]; then
    echo "Cannot read and write loc: $XDDTEST_DEST_MOUNT" >/dev/stderr
    exit 4
fi
    

#
# Run all tests
#
all_tests=$(ls $XDDTEST_TESTS_DIR/scripts/test_acceptance*.sh)
datestamp=$(/bin/date +%s)
failed_test=0
test_log=$XDDTEST_OUTPUT_DIR/test_$datestamp.log

echo "Beginning acceptance test . . ." >$test_log
for test in $all_tests; do
  echo -n "$test . . ."
  $test &> $test_log
    if [ $? -ne 0 ]; then
        echo -e "\r[FAIL] Test $test failed.  See $test_log." >/dev/stderr
        failed_test=1
    else
        echo -e "\r[PASS] Test $test passed." >/dev/stderr
    fi
done
echo "Test output available in $test_log." >/dev/stderr

exit $failed_test
