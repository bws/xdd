#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - See if xddcp/xdd works robustly on a simple file transfer 
# with resume flag set -a to invoke an additional thread (restart thread)
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

startime=$(date +%s)
let "testime = $XDDTEST_TIMEOUT - 30"

# Perform pre-test 
echo "Beginning XDDCP Robust Transfer Test 1 . . ."
test_dir=$XDDTEST_SOURCE_MOUNT/robust1
rm -rf $test_dir
mkdir -p $test_dir
ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/robust1"
ssh $XDDTEST_E2E_DEST "mkdir -p $XDDTEST_DEST_MOUNT/robust1"

source_file=$test_dir/file1
dest_file=$XDDTEST_DEST_MOUNT/robust1/file1

#
# Create the source file
#
$XDDTEST_XDD_EXE -target $source_file -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern random >/dev/null

srcHash=$(md5sum $source_file |cut -d ' ' -f 1)

#
# Loop 100 times or until it hangs. If it hangs, the uber test script will kill it
#
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
 rc=0
 test_passes=1
 loopcount=1
 while [ $loopcount -lt 100 ]
 do

  elapsedtime="$(\echo "$(date +%s) - $startime" | bc)"
  echo "starting transfer # $loopcount elapsed time=${elapsedtime} timelimit=$testime"
  if [ $testime -lt $elapsedtime ]; then
    echo "OUT OF TIME...EXIT"
    break
  fi
  #force each transfer by removing restart cookies and also destination file at destination
  ssh $XDDTEST_E2E_DEST "rm -f $XDDTEST_DEST_MOUNT/robust1/.xdd* $XDDTEST_DEST_MOUNT/robust1/* "
  bash $XDDTEST_XDDCP_EXE $xddcp_opts -a $source_file $XDDTEST_E2E_DEST:$dest_file
  rc=$?
  if [ 0 -ne $rc ]; then
    echo "ERROR: XDDCP exited with: $rc"
    test_passes=0
    break
  fi
  #check destination file after each transfer
  destHash=$(ssh $XDDTEST_E2E_DEST "md5sum $dest_file |cut -d ' ' -f 1")
  if [ "$srcHash" != "$destHash" ]; then
    echo "ERROR: Failure in robust1"
    echo "\tSource hash for $i: $srcHash"
    echo "\tDestination hash for $d: $destHash"
    test_passes=0
    break
  fi

  let "loopcount += 1"
done

# Perform post-test cleanup
#rm -rf $test_dir
#ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/robust1"

# Output test result
if [ "1" == "$test_passes" ]; then
    echo "Acceptance XDDCP5: Robust Transfer Test - Check: PASSED."
    exit 0
else
    echo "Acceptance XDDCP5: Robust Transfer Test - Check: FAILED."
    exit 1
fi
