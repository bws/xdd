#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - See if xddcp/xdd works robustly on a simple file transfer 
# with resume flag set -a to invoke an additional thread (restart thread)
# and over more than one network interface
#
# Description -
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

DEST_DATA_NETWORK_1=${XDDTEST_E2E_DEST}
DEST_DATA_NETWORK_2=${XDDTEST_E2E_DEST}-10g
DEST_DATA_NETWORK_3=${XDDTEST_E2E_DEST}-ib0

$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_1 &>/dev/null
net1_rc=$?
$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_2 &>/dev/null
net2_rc=$?
$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_3 &>/dev/null
net3_rc=$?
if [ 0 -ne $net1_rc -o 0 -ne $net2_rc -o 0 -ne $net3_rc ]; then
    echo "Acceptance $test_name: SKIPPED"
    exit 2
fi


#
# Test stuff
#
startime=$(date +%s)
let "testime = $XDDTEST_TIMEOUT - 30"

# Perform pre-test 
test_dir=$XDDTEST_SOURCE_MOUNT/xddcp-multinic-a
rm -rf $test_dir
mkdir -p $test_dir
ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/xddcp-multinic-a"
ssh $XDDTEST_E2E_DEST "mkdir -p $XDDTEST_DEST_MOUNT/xddcp-multinic-a"

source_file=$test_dir/file1
dest_file=$XDDTEST_DEST_MOUNT/xddcp-multinic-a/file1

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
    ssh $XDDTEST_E2E_DEST "rm -f $XDDTEST_DEST_MOUNT/xddcp-multinic-a/.xdd* $XDDTEST_DEST_MOUNT/xddcp-multinic-a/* "
    bash $XDDTEST_XDDCP_EXE $xddcp_opts -a $source_file $DEST_DATA_NETWORK_1,$DEST_DATA_NETWORK_2,$DEST_DATA_NETWORK_3:$dest_file
    rc=$?
    if [ 0 -ne $rc ]; then
        echo "ERROR: XDDCP exited with: $rc"
        test_passes=0
        break
    fi
    #check destination file after each transfer
    destHash=$(ssh $XDDTEST_E2E_DEST "md5sum $dest_file |cut -d ' ' -f 1")
    if [ "$srcHash" != "$destHash" ]; then
        echo "ERROR: Failure in xddcp-multinic-a"
        echo "\tSource hash for $i: $srcHash"
        echo "\tDestination hash for $d: $destHash"
        test_passes=0
        break
    fi

    let "loopcount += 1"
done

# Perform post-test cleanup
#rm -rf $test_dir
#ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/xddcp-multinic-a"

# Output test result
if [ "1" == "$test_passes" ]; then
    echo "Acceptance Test - $test_name: PASSED."
    exit 0
else
    echo "Acceptance Test - $test_name: FAILED."
    exit 1
fi
