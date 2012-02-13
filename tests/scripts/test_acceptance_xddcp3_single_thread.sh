#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate the recursive flag with xddcp
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


# Perform pre-test 
echo "Beginning Recursive Test 1 . . ."
test_dir=$XDDTEST_SOURCE_MOUNT/recursive1
rm -rf $test_dir
mkdir -p $test_dir
rm -rf $XDDTEST_DEST_MOUNT/recursive1

#
# Create the source directory
#
mkdir -p $test_dir/foo1/foo2/foo3
mkdir -p $test_dir/bar1/bar2/bar3
mkdir -p $test_dir/baz1

#
# Create the files
#
targets=( $test_dir/t1 $test_dir/t2  $test_dir/foo1/t3  $test_dir/foo1/t4 $test_dir/foo1/foo2/t5 $test_dir/foo1/foo2/t6 $test_dir/foo1/foo2/foo3/t7 $test_dir/bar1/bar2/bar3/t8 )
for i in ${targets[@]}; do
    $XDDTEST_XDD_EXE -target $i -op write -reqsize 4096 -mbytes 192 -qd 4 -datapattern random >/dev/null
done

#
# Perform a recursive copy
#
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
#scp $XDDTEST_XDD_EXE $XDDTEST_E2E_DEST:~/bin/xdd
$XDDTEST_XDDCP_EXE $xddcp_opts -t 1 -r $test_dir $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT
rc=$?
#ssh $XDDTEST_E2E_DEST "rm ~/bin/xdd"

test_passes=0
if [ 0 -eq $rc ]; then

    #
    # Perform MD5sum checks
    #
    test_passes=1
    for i in ${targets[@]}; do
	d=$XDDTEST_DEST_MOUNT/${i##$XDDTEST_SOURCE_MOUNT}
	srcHash=$(md5sum $i |cut -d ' ' -f 1)
	destHash=$(ssh $XDDTEST_E2E_DEST "md5sum $d |cut -d ' ' -f 1")
	if [ "$srcHash" != "$destHash" ]; then
	    test_passes=0
	    echo "ERROR: Failure in recursive1"
	    echo "\tSource hash for $i: $srcHash"
	    echo "\tDestination hash for $d: $destHash"
	fi
    done
fi

# Perform post-test cleanup
#rm -rf $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance 5: Recursive Test - Check: PASSED."
  exit 0
else
  echo "Acceptance 5: Recursive Test - Check: FAILED."
  exit 1
fi
