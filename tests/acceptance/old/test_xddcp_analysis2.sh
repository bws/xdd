#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate the post analysis with kernel tracing flag with xddcp for the -W flag
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

# check for existence of iotrace_init, decode, kernel module
# Skip test on non-Linux platforms
\which iotrace_init
if [ 0 -ne $? ]; then
  echo "Acceptance XDDCP-W: XDDCP Post Analysis w Kernel Tracing - iotrace_init missing...test: SKIPPED."
  exit 1
fi
\which decode
if [ 0 -ne $? ]; then
  echo "Acceptance XDDCP-W: XDDCP Post Analysis w Kernel Tracing - decode missing...test: SKIPPED."
  exit 1
fi
if [ ! -e /dev/iotrace_data ]; then
  echo "Acceptance XDDCP-W: XDDCP Post Analysis w Kernel Tracing - /dev/iotrace_data missing...test: SKIPPED."
  exit 1
fi

# Perform pre-test 
echo "Beginning XDDCP Post Analysis w Kernel Tracing Test 1 . . ."
test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
test_dir=$XDDTEST_SOURCE_MOUNT/postanalysis-W
rm -rf $test_dir
mkdir -p $test_dir
ssh $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/postanalysis-W"
ssh $XDDTEST_E2E_DEST "mkdir -p $XDDTEST_DEST_MOUNT/postanalysis-W"

source_file=$test_dir/file1
dest_file=$XDDTEST_DEST_MOUNT/postanalysis-W/file1

#
# Create the source file
#
$XDDTEST_XDD_EXE -target $source_file -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern random >/dev/null

#
# Start a long copy
#
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
bash $XDDTEST_XDDCP_EXE $xddcp_opts -W 5 $source_file $XDDTEST_E2E_DEST:$dest_file &
pid=$!

wait $pid
rc=$?

test_passes=0
if [ 0 -eq $rc ]; then

    #
    # Check for existence of post analysis files with kernel tracing
    #
    test_passes=1
    xfer_files=$(ls -1 ANALYSIS*/analysis.dat | wc -l)
    if [ "$xfer_files" != "1" ]; then
	test_passes=0
	echo "ERROR: Failure in Post Analysis with Kernel Tracing Test 1"
        echo "\tfile analysis.dat is missing"
    fi
    xfer_files=$(ls -1 ANALYSIS*/*eps | wc -l)
    if [ "$xfer_files" != "18" ]; then
	test_passes=0
	echo "ERROR: Failure in Post Analysis with Kernel Tracing Test 1"
	echo "\tNumber of *eps files is: $xfer_files, should be 18"
    fi
else
    echo "ERROR: XDDCP exited with: $rc"
fi

# Perform post-test cleanup
#rm -rf $test_dir
#create directory to save all source side files
test_dir=$XDDTEST_LOCAL_MOUNT/postanalysis-W
rm   -rf           $test_dir
mkdir -p           $test_dir
mv xdd*            $test_dir
mv iotrace*        $test_dir
mv dictionary*     $test_dir
mv ANALYSIS*       $test_dir
mv gnuplot*        $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance XDDCP-W: XDDCP Post Analysis w Kernel Tracing - Check: PASSED."
  exit 0
else
  echo "Acceptance XDDCP-W: XDDCP Post Analysis w Kernel Tracing - Check: FAILED."
  exit 1
fi
