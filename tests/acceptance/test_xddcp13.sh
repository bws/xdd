#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate the filelist flag -F with restart option -a using xddcp
# Scenario:
#   Destination xdd hangs (killed)
#   User or scheduler restarts original xddcp line
#   Modification time changed on a different file every xddcp restart
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
echo "Beginning File List with Restart Test . . ."
src_dir=$XDDTEST_SOURCE_MOUNT/xddcp_test13/data
dest_dir=$XDDTEST_DEST_MOUNT/xddcp_test13/data
rm -rf $XDDTEST_SOURCE_MOUNT/xddcp_test13
ssh -q $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/xddcp_test13"

#
# Create the source directories
#
mkdir -p $src_dir/foo1/foo2/foo3
mkdir -p $src_dir/bar1/bar2/bar3
mkdir -p $src_dir/baz1/baz2/baz3/baz4

#
# Create the files
#
targets=( $src_dir/t1 $src_dir/t2  $src_dir/foo1/t3  $src_dir/foo1/t4 $src_dir/foo1/foo2/t5 $src_dir/foo1/foo2/t6 $src_dir/foo1/foo2/foo3/t7 $src_dir/bar1/bar2/bar3/t8 )
$XDDTEST_XDD_EXE -targets ${#targets[@]} ${targets[@]:0} -op write -reqsize 4096 -mbytes 1024 -qd 4 -datapattern randbytarget 

# Build file list
cd ${src_dir}
file_list="$XDDTEST_SOURCE_MOUNT/xddcp_test13/xddcp_test13_flist"
find . -type f  >${file_list}
     
#
# Start the killer process. Runs until timeout
#

# Construct a NFS based file path to signal Hodson's killer
signal_file_path=${HOME}/xddcp-testing-kill-signal-file

# Start background kills proc w delay
# quites after n passes or testime exceeded
(ssh -q ${XDDTEST_E2E_SOURCE} /bin/bash --login <<EOF 
# Kill just the source side using "-op read" to match the remote xdd
for ((n=0; n < 10; n += 1)); do
    ssh -q $XDDTEST_E2E_DEST "pkill -f \"\\-op read\""
    sleep 20
done
EOF
) &

#
# Perform a list-based copy with a large number of retries
#
$XDDTEST_XDDCP_EXE $xddcp_opts -a -n 99 -F $file_list $XDDTEST_E2E_DEST:$dest_dir
rc=$?

# signal killer proc to exit
touch ${signal_file_path}
# wait for killer proc to finish
wait
# remove signal file
rm ${signal_file_path}

#
# Perform MD5sum checks
#
test_passes=1
for i in `cat ${file_list}`; do
    src_file=${src_dir}/${i}
    dest_file=${dest_dir}/${i}

    # Calculate hashes and compare
    src_hash=$(md5sum ${src_file}|cut -d ' ' -f 1)
    dest_hash=$(ssh -q $XDDTEST_E2E_DEST "md5sum ${dest_file} |cut -d ' ' -f 1")
    if [ "$src_hash" != "$dest_hash" ]; then
        test_passes=0
        echo "ERROR: Hash mismatch ${src_file}: ${src_hash} ${dest_file}: ${dest_hash}"
    fi
done

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance XDDCP: Test 13 - Check: PASSED."
  exit 0
else
  echo "Acceptance XDDCP: Test 13 - Check: FAILED."
  exit 1
fi
