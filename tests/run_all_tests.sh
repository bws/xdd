#/bin/bash -x
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
g_parentPID=$$
g_alarmPID=0
g_testPID=0
g_testTimedOut=0

#
# Handle exit cleanly
#
function handle_exit
{
    # First kill any test script children
    pkill -P $g_parentPID >/dev/null 2>&1
    sleep 1
    pkill -KILL -P $g_parentPID >/dev/null 2>&1

    # Kill any XDD processes that have been orphaned (transfer destinations)
    pkill -u nightly -P 1 xdd >/dev/null 2>&1
    sleep 1
    pkill -KILL -u nightly -P 1 xdd >/dev/null 2>&1

    # Kill any remaning members of process group (e.g. the timeout signaller)
    #kill -$$ >/dev/null 2>&1
}

#
# Setup test timeout alarm functionality
#
function test_timeout_handler
{
    ps -aef |grep nightly > before_timeout_killer
    g_testTimedOut=0
    if [ 0 -ne $g_testPID ]; then
        echo "Test timeout triggered for process: $g_testPID"

        # Kill any of the test script's children processes
        pkill -P $g_testPID >/dev/null 2>&1
        sleep 1
        pkill -KILL -P $g_testPID >/dev/null 2>&1

        # Kill any XDD processes that are orphaned (destinations on transfers)
        pkill -u nightly -P 1 xdd >/dev/null 2>&1
        sleep 1
        pkill -KILL -u nightly -P 1 xdd >/dev/null 2>&1

        # Finally, kill the test script that has timed out
        rc=1
        pgrep $g_testPID
        script_running=$?
        if [ $script_running -eq 0 ]; then
            #
            # We can't use SIGKILL here because it triggers a BASH bug that 
            # leaves the stdout for this process in a corrupt state.  The 
            # next write will fail, so let's hope SIGTERM works here
	    pkill -P $g_parentPID $g_testPID >/dev/null 2>&1
            rc=$?
        
	    if [ 0 -eq $rc ]; then
	        g_testTimedOut=1
            else
                echo "Error terminating process: $g_testPID rc: $rc" >>/dev/stderr
	    fi
        else
            echo "Warning: Test process $g_testPID exited." >>/dev/stderr
        fi
	g_testPID=0
    else
	echo "ERROR:  Alarm signalled for invalid test pid: $g_testPID" >>/dev/stderr
    fi
    ps -aef |grep nightly > after_timeout_killer

    # Sleep to let the system state quiesce
    sleep 2

    return 0
}

#
# Fork the alarm wait process to a subshell
#
function test_timeout_alarm_helper
{
    sleep $MAX_TEST_TIME &
    sleep_pid=$!
    echo "Sleeping as $sleep_pid, sending alarm to $$"
    wait $!
    kill -ALRM $g_parentPID
}

#
# Set an alarm signal in a subshell that signals this shell
#
function test_timeout_alarm
{
    test_timeout_alarm_helper >timeout-trace.txt 2>&1 &
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
# Install handler to make sure all children die on exit
#
trap 'handle_exit' exit

#
# Create the log file
#
datestamp=$(/bin/date +%s)
all_test_log=$XDDTEST_OUTPUT_DIR/test_$datestamp.log

#
# Ensure the source and destinations are reachable
#
ping -c 2 "$XDDTEST_E2E_SOURCE" >>$all_test_log 2>&1
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_SOURCE" >>$all_test_log 2>&1
    exit 1
fi

ping -c 2 "$XDDTEST_E2E_DEST" >>$all_test_log 2>&1
if [ $? -ne 0 ]; then
    echo "Unable to contact source: $XDDTEST_E2E_DEST" >>$all_test_log 2>&1
    exit 2 
fi

#
# Ensure the mount points exist
#
if [ ! -w "$XDDTEST_LOCAL_MOUNT" -o ! -r "$XDDTEST_LOCAL_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_LOCAL_MOUNT" >>$all_test_log 2>&1
    tail -1 $all_test_log >/dev/stderr
    exit 3
fi

if [ ! -w "$XDDTEST_SOURCE_MOUNT" -o ! -r "$XDDTEST_SOURCE_MOUNT" ]; then
    echo "Cannot read and write loc: $XDDTEST_SOURCE_MOUNT" >>$all_test_log 2>&1
    tail -1 $all_test_log >/dev/stderr
    exit 3
fi

ssh $XDDTEST_E2E_DEST bash <<EOF
    if [ ! -w "$XDDTEST_DEST_MOUNT" -o ! -r "$XDDTEST_DEST_MOUNT" ]; then
        exit 1
    fi
EOF
if [ $? != 0 ]; then
    echo "Cannot read and write loc: $XDDTEST_DEST_MOUNT" >>$test_log 2>&1
    tail -1 $all_test_log >/dev/stderr
    exit 4
fi
    

#
# Run all tests
#
all_tests=$(ls $XDDTEST_TESTS_DIR/scripts/test_acceptance*.sh)
failed_test=0

echo "Beginning acceptance tests . . ." >> $all_test_log
for test in $all_tests; do
    test_base=$(basename $test)
    test_name=$(echo $test_base |cut -d . -f 1)
    test_log="$XDDTEST_OUTPUT_DIR/${test_name}.log"
    echo "Running test $test_name, see output in $test_log" >> $all_test_log

    # Log the system state before each test
    echo "Process Table before Test $test_base ------------" >> $test_log
    ps -aef |grep nightly >> $test_log
    echo "Process Table before Test $test_base -------------" >> $test_log

    # Allow the test to run until timeout, then kill it automatically
    trap 'test_timeout_handler' 14
    test_timeout_alarm
    echo -n "Running ${test_base} . . . "
    $test >> $test_log 2>&1 &
    g_testPID=$!
    echo "Test process id is: $g_testPID" >> $test_log
    test_result=""
    wait $g_testPID
    test_result=$?

    # Cancel the alarm (test finished) 
    test_timeout_alarm_cancel

    # Check test's status
    if [ 1 -eq ${g_testTimedOut} -a -z "$test_result" ]; then
        echo -e "[FAILED] Timed out.  See $test_log."
        failed_test=1
    elif [ ${test_result} -eq 0 ]; then
        echo -e "[PASSED]"
    elif [ ${test_result} -eq 2 ]; then
	echo -e "[SKIPPED]"
    else
        echo -e "[FAILED] See $test_log."
        failed_test=1
    fi

    # Log the system state before each test
    echo "Process Table after Test -----------------------" >> $test_log
    ps -aef |grep nightly >> $test_log
    echo "Process Table after Test -----------------------" >> $test_log

    # Sync the fs and sleep to quiesce the system between tests
    sync
    sleep 2
done

echo "Test output available in $all_test_log."

exit $failed_test
