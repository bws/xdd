#!/bin/bash
#
# Utility functions for acceptance testing.  
#
# Highlights are:
#  - initializes test by:
#      creating data directories
#  - finalizes test by:
#      printing a pretty formatted message 
#      cleaning up test data if the test passed or skipped
#  - generates correct test data files
#  - generates correct test file names
#

TESTNAME=$(basename $0 |cut -f 1 -d .)
common_next_file=""

#
# Initialize test
#
initialize_test() {
    # Ensure that the test config has been sourced
    if [ -z "$XDDTEST_OUTPUT_DIR" ]; then
        echo "No properly sourced test_config during initialization"
        finalize_test 2
    fi

    # Create directories associated with local tests
    \mkdir -p $XDDTEST_LOCAL_MOUNT/$TESTNAME
    if [ 0 -ne $? ]; then
        echo "Unable to create $XDDTEST_LOCAL_MOUNT/$TESTNAME"
        finalize_test 2
    fi
    ssh $XDDTEST_E2E_SOURCE "\mkdir -p $XDDTEST_SOURCE_MOUNT/$TESTNAME"
    if [ 0 -ne $? ]; then
        echo "Unable to create $XDDTEST_SOURCE_MOUNT/$TESTNAME"
        finalize_test 2
    fi
    ssh $XDDTEST_E2E_DEST "\mkdir -p $XDDTEST_DEST_MOUNT/$TESTNAME"
    if [ 0 -ne $? ]; then
        echo "Unable to create $XDDTEST_DEST_MOUNT/$TESTNAME"
        finalize_test 2
    fi

    # Create the log directory
    \mkdir -p $XDDTEST_OUTPUT_DIR

    # Initialize the uid for data files
    common_next_file="0"
}

#
# Finalize the test by checking its status
# 0 indicates a pass
# -1 indicates the test was skipped
# all other values indicate a test failure
#
finalize_test() {
  local status="$1"

  local rc=1
  if [ $status -eq 0 ]; then
      pass
      rc=0
  elif [ $status -eq -1 ]; then
      skip
      rc=0
  else
      fail
  fi

  if [ 0 -eq $rc ]; then
      cleanup_test_data
  fi
  exit $rc
}

#
# Generate a local filename
#
generate_local_filename() {
    local varname="$1"
    local name="$XDDTEST_LOCAL_MOUNT/$TESTNAME/file${common_next_file}.tdt"
    eval "$varname=$name"
    common_next_file=$((common_next_file + 1))
    return 0
}

#
# Generate a source filename
#
generate_source_filename() {
    local varname="$1"
    local name="$XDDTEST_SOURCE_MOUNT/$TESTNAME/file${common_next_file}.tdt"
    eval "$varname=$name"
    common_next_file=$((common_next_file + 1))
    return 0
}

#
# Generate a destination filename
#
generate_dest_filename() {
    local varname="$1"
    local name="$XDDTEST_DEST_MOUNT/$TESTNAME/file${common_next_file}.tdt"
    eval "$varname=$name"
    common_next_file=$((common_next_file + 1))
    return 0
}

#
# Create local test file of size n, outputs filename
#
generate_local_file() {
    local varname="$1"
    local size="$2"

    if [ -z "$size" ]; then
        echo "No test file data size specified."
        finalize_test 2
    fi

    local lfname=""
    generate_local_filename lfname
    $XDDTEST_XDD_EXE -op write -target $lfname -reqsize 1 -blocksize $((1024*1024)) -bytes $size -datapattern random >/dev/null 2>&1

    # Ensure the file size is correct
    asize=$($XDDTEST_XDD_GETFILESIZE_EXE $lfname)
    if [ "$asize" != "$size" ]; then
        echo "Unable to generate test file data of size: $size"
        finalize_test 2
    fi

    # Ensure the file isn't all zeros
    read -r -n 4 < /dev/zero
    local data=$REPLY
    read -r -n 4 < $lfname
    if [ "$REPLY" = "$data" ]; then
        echo "Unable to generate random test file data"
        finalize_test 2
    fi
    eval "$varname=$lfname"
    return 0
}

#
# Create source-side test file of size n,
#
generate_source_file() {
    local varname="$1"
    local size="$2"

    if [ -z "$size" ]; then
        echo "No test file data size specified."
        finalize_test 2
    fi

    local fname=""
    generate_source_filename fname
    ssh $XDDTEST_E2E_SOURCE "$XDDTEST_E2E_SOURCE_XDD_PATH/xdd -op write -target $fname -reqsize 1 -blocksize $((1024*1024)) -bytes $size -datapattern random >/dev/null 2>&1"

    # Ensure the file size is correct
    local asize=$(ssh $XDDTEST_E2E_SOURCE "$XDDTEST_E2E_SOURCE_XDD_PATH/xdd-getfilesize $fname")
    if [ "$asize" != "$size" ]; then
        echo "Unable to generate test file data of size: $size"
        finalize_test 2
    fi

    # Ensure the file isn't all zeros
    read -r -n 4 < /dev/zero
    local zeroes=$REPLY
    local data=$(ssh $XDDTEST_E2E_SOURCE "read -r -n 4 < $fname; echo \$REPLY")
    if [ 0 -ne $? -o "$data" = "$zeroes" ]; then
        echo "Unable to generate random test file data"
        finalize_test 2
    fi
    eval "$varname=$fname"
    return 0
}

#
# Returns 0 is the md5's match, 1 if they don't
#
compare_source_dest_md5() {
    local sfile="$1"
    local dfile="$2"
    # Make sure the names are valid
    if [ -z $sfile ]; then
        echo "No source file name provided"
        finalize_test 2
    fi
    if [ -z $dfile ]; then
        echo "No destination file name provided"
        finalize_test 2
    fi

    local ssumcmd="if [ -z \$(which md5 2>/dev/null) ]; then md5sum $sfile; else md5 -r $sfile; fi"
    local ssum=$(ssh $XDDTEST_E2E_SOURCE "$ssumcmd")
    if [ 0 -ne $? -o -z "$ssum" ]; then
        echo "Unable to md5sum $sfile: $ssum"
        finalize_test 2
    fi

    local dsumcmd="if [ -z \$(which md5 2>/dev/null) ]; then md5sum $dfile; else md5 -r $dfile; fi"
    local dsum=$(ssh $XDDTEST_E2E_DEST "$dsumcmd")
    if [ 0 -ne $? -o -z "$dsum" ]; then
        echo "Unable to md5sum $dfile: $dsum"
        finalize_test 2
    fi

    ssum=$(echo $ssum |cut -f 1 -d ' ')
    dsum=$(echo $dsum |cut -f 1 -d ' ')
    if [ -z "$ssum" -o -z "$dsum" -o "$ssum" != "$dsum" ]; then
        echo "Mismatched checksums: $ssum $dsum"
        return 1
    else
        return 0
    fi
}

#
# Remove any generated test data
#
cleanup_test_data() {

    # Remove files associated with local tests
    \rm -rf $XDDTEST_LOCAL_MOUNT/$TESTNAME
    ssh $XDDTEST_E2E_SOURCE "\rm -rf $XDDTEST_LOCAL_MOUNT/$TESTNAME"
    ssh $XDDTEST_E2E_DEST "\rm -rf $XDDTEST_DEST_MOUNT/$TESTNAME"
}

#
# Indicate a test has failed
#
fail() {
    printf "%-20s\t%10s\n" $TESTNAME "FAIL"
}

#
# Indicate a test has passed
#
pass() {
    printf "%-20s\t%10s\n" $TESTNAME "PASS"
}

#
# Indicate a test was skipped
#
skip() {
    printf "%-20s\t%10s\n" $TESTNAME "SKIPPED"
}

