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
echo "Beginning File List with Restart Test 1 . . ."
test_dir=$XDDTEST_SOURCE_MOUNT/xddcp-F-a-n
rm -rf $test_dir
ssh -q $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/xddcp-F-a-n"
mkdir -p $test_dir

#
# Create the source directories
#
mkdir -p $test_dir/foo1/foo2/foo3
mkdir -p $test_dir/bar1/bar2/bar3
mkdir -p $test_dir/baz1/baz2/baz3/baz4

#
# Create the files
#
targets=( $test_dir/t1 $test_dir/t2  $test_dir/foo1/t3  $test_dir/foo1/t4 $test_dir/foo1/foo2/t5 $test_dir/foo1/foo2/t6 $test_dir/foo1/foo2/foo3/t7 $test_dir/bar1/bar2/bar3/t8 )
    $XDDTEST_XDD_EXE -targets ${#targets[@]} ${targets[@]:0} -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern randbytarget 
\date
# Make them different/wierd sizes
# Build file list
     file_list="${test_dir}/xddcp-F-a-n-file-list"
     pattern=""
     let "k = 0"
     for i in ${targets[@]}; do
         let "k += 1"
         pattern="${pattern}$k"
         echo "$pattern" >> $i
         srcHash[$k]="$(md5sum $i |cut -d ' ' -f 1)"
         echo "srcHash=${srcHash[$k]}: ${XDDTEST_E2E_SOURCE}: ${i}"
         echo "$i" >> $file_list
    done
     
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
          sleep 20
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
             ssh ${XDDTEST_E2E_DEST} "pkill -9 -x xdd"
             sleep 1
             done 
          fi
          (( n -= 1 ))
        done
EOF
) &

#
# Perform a file list copy
#
rc=1
while [ 0 -ne $rc ]; do
	# Perform a recursive copy. If not first pass, restarting
	$XDDTEST_XDDCP_EXE $xddcp_opts -a -n 99 -F $file_list $XDDTEST_E2E_DEST:$XDDTEST_DEST_MOUNT/xddcp-F-a-n
        rc=$?
done

# signal killer proc to exit
touch ${signal_file_path}
# wait for killer proc to finish
wait
# remove signal file
rm ${signal_file_path}

#
# Perform MD5sum checks
#
let "k = 0"
test_passes=1
for i in ${targets[@]}; do
    trimmedSrcFile=${i#${PWD}}
    d=$XDDTEST_DEST_MOUNT/xddcp-F-a-n/${trimmedSrcFile}
    destHash=$(ssh $XDDTEST_E2E_DEST "md5sum $d |cut -d ' ' -f 1")
    let "k += 1"
    echo "srcHash=${srcHash[$k]}: ${XDDTEST_E2E_SOURCE}: ${i}"
    echo "dstHash=$destHash: ${XDDTEST_E2E_DEST}: ${d}"
    if [ "${srcHash[$k]}" != "$destHash" ]; then
        test_passes=0
        echo "ERROR: Failure in xddcp-F-a-n"
        echo "\tSource hash for $i: ${srcHash[$k]}"
        echo "\tDestination hash for $d: $destHash"
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
