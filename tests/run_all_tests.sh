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
# Global Data
#
MAX_TEST_TIME=10m
g_alarmPID=0
g_testPID=0
g_testTimedOut=0

#
# Handle exit cleanly
#
function handle_exit
{
    kill -KILL -$$ &> /dev/null
}

#
# Setup test timeout alarm functionality
#
function test_timeout_handler
{
    g_testTimedOut=0
    if [ 0 -ne $g_testPID ]; then
	pkill -KILL -P $$ $g_testPID
        rc=$?
	if [ 0 -eq $rc ]; then
	    g_testTimedOut=1
        else
            echo "Error terminating process: $g_testPID rc: $rc"
	fi
	g_testPID=0
    else
	echo "ERROR:  Alarm signalled for invalid test pid: $g_testPID"
    fi
    return 0
}

#
# Fork the alarm wait process to a subshell
#
function test_timeout_alarm_helper
{
    sleep $MAX_TEST_TIME &
    wait $!
    kill -ALRM $$
}

#
# Set an alarm signal in a subshell that signals this shell
#
function test_timeout_alarm
{
    test_timeout_alarm_helper &>/dev/null &
    g_alarmPID=$!
    return 0
}

#
# Disable an existing alarm
#
function test_timeout_alarm_cancel
{
    trap 'echo 0 >/dev/null' 14
    if [ 0 -ne $g_alarmPID ]; then
	pkill -P $g_alarmPID -u $USER sleep
	g_alarmPID=0
	return 0
    else
	return 1
    fi    
}

#
# Make sure all children stop on exit
#
trap 'handle_exit' exit

#
# Create the log file
#
datestamp=$(/bin/date +%s)
test_log=$XDDTEST_OUTPUT_DIR/test_$datestamp.log

#
# Ensure the source and destinations are reachable
#
ping -c 2 "$XDDTEST_E2E_SOURCE" >>$test_log 2>&1
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_SOURCE" >>$test_log 2>&1
    exit 1
fi

ping -c 2 "$XDDTEST_E2E_DEST" &>>$test_log
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_DEST" >>$test_log 2>&1
    exit 2 
fi

#
# Ensure the mount points exist
#
if [ ! -w "$XDDTEST_LOCAL_MOUNT" -o ! -r "$XDDTEST_LOCAL_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_LOCAL_MOUNT" >>$test_log 2>&1
    exit 3
fi

if [ ! -w "$XDDTEST_SOURCE_MOUNT" -o ! -r "$XDDTEST_SOURCE_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_SOURCE_MOUNT" >>$test_log 2>&1
    exit 3
fi

ssh $XDDTEST_E2E_DEST bash <<EOF
    if [ ! -w "$XDDTEST_DEST_MOUNT" -o ! -r "$XDDTEST_DEST_MOUNT" ]; then
        exit 1
    fi
EOF
if [ $? != 0 ]; then
    echo "Cannot read and write loc: $XDDTEST_DEST_MOUNT" >>$test_log 2>&1
    exit 4
fi
    

#
# Run all tests
#
all_tests=$(ls $XDDTEST_TESTS_DIR/scripts/test_acceptance*.sh)
failed_test=0

echo "Beginning acceptance tests . . ." >>$test_log
for test in $all_tests; do
    test_base=$(basename $test)


    # Allow the test to run until timeout, then kill it automatically
    trap 'test_timeout_handler' 14
    test_timeout_alarm
    echo -n "Running ${test_base} . . . "
    $test >> $test_log 2>&1 &
    g_testPID=$!
    test_result=""
    wait $g_testPID
    test_result=$?

    # Cancel the alarm (test finished) 
    test_timeout_alarm_cancel
    
    # Check test's status
    if [ 1 -eq $g_testTimedOut -a -z "$test_result" ]; then
        echo -e "[FAILED] Timed out.  See $test_log."
        failed_test=1
    elif [ $test_result -eq 0 ]; then
        echo -e "[PASSED]"
    elif [ $test_result -eq 2 ]; then
	echo -e "[SKIPPED]"
    else
        echo -e "[FAILED] See $test_log."
        failed_test=1
    fi
done
echo "Test output available in $test_log."

exit $failed_test
