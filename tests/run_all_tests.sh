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
# Create the log file
#
datestamp=$(/bin/date +%s)
test_log=$XDDTEST_OUTPUT_DIR/test_$datestamp.log

#
# Ensure the source and destinations are reachable
#
ping -c 2 "$XDDTEST_E2E_SOURCE" &>>$test_log
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_SOURCE" &>>$test_log
    exit 1
fi

ping -c 2 "$XDDTEST_E2E_DEST" &>>$test_log
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_DEST" &>>$test_log
    exit 2 
fi

#
# Ensure the mount points exist
#
if [ ! -w "$XDDTEST_LOCAL_MOUNT" -o ! -r "$XDDTEST_LOCAL_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_LOCAL_MOUNT" &>>$test_log
    exit 3
fi

if [ ! -w "$XDDTEST_SOURCE_MOUNT" -o ! -r "$XDDTEST_SOURCE_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_SOURCE_MOUNT" &>>$test_log
    exit 3
fi

ssh $XDDTEST_E2E_DEST bash <<EOF
    if [ ! -w "$XDDTEST_DEST_MOUNT" -o ! -r "$XDDTEST_DEST_MOUNT" ]; then
        exit 1
    fi
EOF
if [ $? != 0 ]; then
    echo "Cannot read and write loc: $XDDTEST_DEST_MOUNT" &>>$test_log
    exit 4
fi
    

#
# Run all tests
#
all_tests=$(ls $XDDTEST_TESTS_DIR/scripts/test_acceptance*.sh)
failed_test=0

echo "Beginning acceptance test . . ." >>$test_log
for test in $all_tests; do
  echo -n "$test . . ."
  $test &>> $test_log
    if [ $? -ne 0 ]; then
        echo -e "\r[FAIL] Test $test failed.  See $test_log."
        failed_test=1
    else
        echo -e "\r[PASS] Test $test passed."
    fi
done
echo "Test output available in $test_log."

exit $failed_test
