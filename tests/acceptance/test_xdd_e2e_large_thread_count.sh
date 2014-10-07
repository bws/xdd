#!/bin/bash
#
# Test XDDMCP with very large thread counts
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
#wcmd="xdd -op write -target $dfile -e2e isdest -e2e dest $XDDTEST_E2E_DEST:40010,124 -reqsize 1 -blocksize $((4096 * 1024)) -bytes $((1024*1024*768))"
#ssh $XDDTEST_E2E_DEST "$wcmd &"
#sleep 15
#xdd -op read -target $sfile -e2e issource -e2e dest $XDDTEST_E2E_DEST:40010,124 -hb pct -reqsize 1 -blocksize $((4096 * 1024)) -bytes $((1024*1024*768))
#if [ 0 != $? ]; then
#    echo "XDD source command failed"
#    finalize_test 1
#fi
finalize_test 0
exit 0

#
# Compare the md5sums
#
compare_source_dest_md5 "$sfile" "$dfile"
result=$? 
finalize_test $result 
