#!/bin/bash
#
# Test XDDMCP with thread count exceeding the limit
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
xddmcp -v -t 128 $XDDTEST_E2E_SOURCE:$sfile $XDDTEST_E2E_DEST:$dfile
if [ 1 != $? ]; then
    echo "XDDMCP command did not detect invalid thread count"
    finalize_test 1
fi
finalize_test 0
