#!/bin/bash
#
# Test XDDMCP with timestamps enabled
#
source ./test_config
source $XDDTEST_TESTS_DIR/acceptance/common.sh
initialize_test

#
# Generate the source file and destination name
# 
generate_source_file sfile $((1024*1024*768))
generate_dest_filename dfile

#
# Move the file with a large thread count
#
xddmcp -V $XDDTEST_E2E_SOURCE:$sfile $XDDTEST_E2E_DEST:$dfile
if [ 0 != $? ]; then
    echo "XDDMCP command failed"
    finalize_test 1
fi

#
# Compare the md5sums
#
compare_source_dest_md5 "$sfile" "$dfile"
result=$? 
finalize_test $result 
