#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate the filelist flag -F with restart option -a using xddcp
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
src_dir=$XDDTEST_SOURCE_MOUNT/xddcp_test11/data
dest_dir=$XDDTEST_DEST_MOUNT/xddcp_test11/data
rm -rf $XDDTEST_SOURCE_MOUNT/xddcp_test11
ssh -q $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/xddcp_test11"

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
$XDDTEST_XDD_EXE -targets ${#targets[@]} ${targets[@]:0} -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern randbytarget 

# Build file list
cd ${src_dir}
file_list="$XDDTEST_SOURCE_MOUNT/xddcp_test11/xddcp_test11_flist"
find . -type f  >${file_list}
     
#
# Start the killer process. Runs until timeout
#

# Construct a NFS based file path to signal Hodson's killer
signal_file_path=${HOME}/xddcp-testing-kill-signal-file
startime=$(date +%s)
let "testime = $XDDTEST_TIMEOUT - 360"
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH

        # start background kills proc w delay
        # quites after n passes or testime exceeded
        (ssh -q ${XDDTEST_E2E_SOURCE} /bin/bash --login <<EOF 
        (( n = 30 ))
        while (( n > 0 ))
        do
          sleep 30
          elapsedtime="\$(\echo "\$(\date +%s) - $startime" | \bc)"
          if [ $testime -lt \$elapsedtime -o -f "$signal_file_path" ]; then
            echo "killer process...EXITing"
            break
          fi
          (( k = n % 5 ))
          if (( k == 0 ))
          then
             pkill -9 -x xddcp
          fi
          if (( k == 1 || k == 3 ))
          then
             for (( i=0; i<7; i++ )) 
             do
               pkill -9 -x xdd
               sleep 1
             done 
          fi
          if (( k == 2 || k == 4 ))
          then
             for (( i=0; i<7; i++ )) 
             do
             ssh -q ${XDDTEST_E2E_DEST} "pkill -9 -x xdd"
             sleep 1
             done 
          fi
          (( n -= 1 ))
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
  echo "Acceptance XDDCP: File List with Restart Test - Check: PASSED."
  exit 0
else
  echo "Acceptance XDDCP: File List with Restart Test - Check: FAILED."
  exit 1
fi
