#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate the post analysis with kernel tracing flag with xdd and xdd-read-tsdumps 
#
#
# Source the test configuration environment
#
source ./test_config

# check for existence of iotrace_init, decode, kernel modulei
# skip test if on non-Linux platforms
\which iotrace_init
if [ 0 -ne $? ]; then
  echo "Acceptance XDD-ANALYSIS1: XDD Post Analysis w Kernel Tracing - iotrace_init missing...test: SKIPPED."
  exit 1
fi
\which decode
if [ 0 -ne $? ]; then
  echo "Acceptance XDD-ANALYSIS1: XDD Post Analysis w Kernel Tracing - decode missing...test: SKIPPED."
  exit 1
fi
if [ ! -e /dev/iotrace_data ]; then
  echo "Acceptance XDD-ANALYSIS1: XDD Post Analysis w Kernel Tracing - /dev/iotrace_data missing...test: SKIPPED."
  exit 1
fi

# Perform pre-test 
echo "Beginning XDD Post Analysis w Kernel Tracing Test 1 . . ."

test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
test_dir=$XDDTEST_SOURCE_MOUNT/postanalysis-xdd
rm -rf $test_dir
mkdir -p $test_dir

source_file=$test_dir/file1

#set as passed, then run the gauntlet
    test_passes=1

export TRACE_LOG_LOC=$PWD
#
# Write test with kernel tracing
#
iotrace_init $XDDTEST_XDD_EXE -target $source_file -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern random -ts dump xdd-write-tsdump

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDDTEST_XDD_EXE failed on write, exited with: $rc"
fi
#
# write postanalysis with xdd-read-tsdumps
#
XDD_READ_TSDUMPS_EXE=$(dirname $XDDTEST_XDD_EXE)/xdd-read-tsdumps
$XDD_READ_TSDUMPS_EXE -t 1 -k -o ANALYSIS_write xdd-write-tsdump.target.0000.bin

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDD_READ_TSDUMPS_EXE failed on write analysis, exited with: $rc"
fi
#
# Read test with kernel tracing
#
iotrace_init $XDDTEST_XDD_EXE -target $source_file -op read -reqsize 4096 -mbytes 4096 -qd 4 -ts dump xdd-read-tsdump

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDDTEST_XDD_EXE failed on read, exited with: $rc"
fi
#
# read postanalysis with xdd-read-tsdumps
#
$XDD_READ_TSDUMPS_EXE -t 1 -k -o ANALYSIS_read xdd-read-tsdump.target.0000.bin

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDD_READ_TSDUMPS_EXE failed on read analysis, exited with: $rc"
fi

#
# Mixed test with kernel tracing
#
iotrace_init $XDDTEST_XDD_EXE -target $source_file -op read -reqsize 4096 -mbytes 4096 -qd 4 -ts dump xdd-mixed-tsdump -rwratio 50

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDDTEST_XDD_EXE failed on mixed, exited with: $rc"
fi
#
# read postanalysis with xdd-read-tsdumps
#
$XDD_READ_TSDUMPS_EXE -t 1 -k -o ANALYSIS_mixed xdd-mixed-tsdump.target.0000.bin

rc=$?
if [ 0 -ne $rc ]; then
  test_passes=0
  echo "ERROR: $XDD_READ_TSDUMPS_EXE failed on mixed analysis, exited with: $rc"
fi

    #
    # Check for existence of post analysis files with kernel tracing
    #
    xfer_files=$(ls -1 ANALYSIS*/analysis.dat | wc -l)
    if [ "$xfer_files" != "3" ]; then
	test_passes=0
	echo "ERROR: Failure in xdd-analysis1 with Kernel Tracing Test 1"
        echo "\tNumber of analysis.dat files is: $xfer_files, should be 3"
    fi
    xfer_files=$(ls -1 ANALYSIS*/*eps | wc -l)
    if [ "$xfer_files" != "6" ]; then
	test_passes=0
	echo "ERROR: Failure in xdd-analysis on write op with Kernel Tracing Test 1"
	echo "\tNumber of *eps files is: $xfer_files, should be 6"
    fi

# Perform post-test cleanup
#rm -rf $test_dir
#create directory to save all source side files
test_dir=$XDDTEST_LOCAL_MOUNT/xdd-analysis1
rm   -rf           $test_dir
mkdir -p           $test_dir
mv xdd*            $test_dir
mv iotrace*        $test_dir
mv dictionary*     $test_dir
mv ANALYSIS*       $test_dir
mv gnuplot*        $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance XDD-ANALYSIS1: XDD Post Analysis w Kernel Tracing - Check: PASSED."
  exit 0
else
  echo "Acceptance XDD-ANALYSIS1: XDD Post Analysis w Kernel Tracing - Check: FAILED."
  exit 1
fi
