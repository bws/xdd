#!/bin/sh
#
# Acceptance test for XDD.
#
# Description - Validate the recursive flag -r with restart option -a using xddcp
# and more than one network interface
# Scenario:
#   Destination xdd hangs (killed)
#   User or scheduler restarts original xddcp line
#   Modification time changed on a different file every xddcp restart
#
#
# Test identity
#
test_name=$(basename $0)
echo "Beginning $test_name . . ."

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

DEST_DATA_NETWORK_1=${XDDTEST_E2E_DEST}
DEST_DATA_NETWORK_2=${XDDTEST_E2E_DEST}-10g
DEST_DATA_NETWORK_3=${XDDTEST_E2E_DEST}-ib0

$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_1 &>/dev/null
net1_rc=$?
$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_2 &>/dev/null
net2_rc=$?
$XDDTEST_XDD_GETHOSTIP_EXE $DEST_DATA_NETWORK_3 &>/dev/null
net3_rc=$?
if [ 0 -ne $net1_rc -o 0 -ne $net2_rc -o 0 -ne $net3_rc ]; then
    echo "Acceptance $test_name: SKIPPED"
    exit 2
fi

# Perform pre-test 
src_dir=$XDDTEST_SOURCE_MOUNT/test_xddcp_multinic5/data
dest_dir=$XDDTEST_DEST_MOUNT/test_xddcp_multinic5/data
rm -rf $XDDTEST_SOURCE_MOUNT/test_xddcp_multinic5
ssh -q $XDDTEST_E2E_DEST "rm -rf $XDDTEST_DEST_MOUNT/test_xddcp_multinic5"

#
# Create the source directories
#
mkdir -p $src_dir/foo1/foo2/foo3
mkdir -p $src_dir/bar1/bar2/bar3
mkdir -p $src_dir/baz1/baz2/baz3/baz4
mkdir -p $src_dir/bay1/bay2/bay3/bay4/bay5
mkdir -p $src_dir/bax1/bax2/bax3/bax4/bax5/bax6
mkdir -p $src_dir/box1/box2/box3/box4/box5/box6/box7/box8/box9

# Create the destination diretories
ssh -q $XDDTEST_E2E_DEST "mkdir -p $XDDTEST_DEST_MOUNT/test_xddcp_multinic5"

#
# Create the files
#
targets=( $src_dir/t1 $src_dir/t2  $src_dir/foo1/t3  $src_dir/foo1/t4 $src_dir/foo1/foo2/t5 $src_dir/foo1/foo2/t6 $src_dir/foo1/foo2/foo3/t7 $src_dir/bar1/bar2/bar3/t8 )
$XDDTEST_XDD_EXE -targets ${#targets[@]} ${targets[@]:0} -op write -reqsize 4096 -mbytes 4096 -qd 4 -datapattern randbytarget 
     
#
# Start killer process. Runs until timeout
#
killer_timeout=360
startime=$(date +%s)
rm -f ${HOME}/xdd_killer_exit
export PATH=$(dirname $XDDTEST_XDD_EXE):/usr/bin:$PATH
        # start background kills proc w delay
        # quites after n passes or testime exceeded
(ssh -q ${XDDTEST_E2E_SOURCE} /bin/bash --login <<EOF 
    for (( i=0; i < 30; i++ )); do
        sleep 20
        elapsedtime="\$(\echo "\$(\date +%s) - $startime" | \bc)"
        if [ $killer_timeout -lt \$elapsedtime -o -f ${HOME}/xdd_killer_exit ]; then
            echo "killer process...EXITing"
            break
        fi
        for (( j=0; j<7; j++ )); do 
            ssh ${XDDTEST_E2E_DEST} "pkill -9 -x xdd"
            sleep 1
        done
    done
EOF
) &

#
# Perform a recursive copy
#
rc=1
$XDDTEST_XDDCP_EXE $xddcp_opts -r -a -n 99 $src_dir $DEST_DATA_NETWORK_1,$DEST_DATA_NETWORK_2,$DEST_DATA_NETWORK_3:$XDDTEST_DEST_MOUNT/test_xddcp_multinic5
rc=$?

# signal killer proc to exit
touch $HOME/xdd_killer_exit
wait

# remove signal file
rm -f $HOME/xdd_killer_exit

#
# Perform MD5sum checks
#
test_passes=1
for i in ${targets[@]}; do
    src_file=${i}
    dest_file=${dest_dir}/${i##$src_dir}

    # Calculate hashes and compare
    src_hash=$(md5sum ${src_file}|cut -d ' ' -f 1)
    dest_hash=$(ssh -q $XDDTEST_E2E_DEST "md5sum ${dest_file} |cut -d ' ' -f 1")
    if [ "$src_hash" != "$dest_hash" ]; then
        test_passes=0
        echo "ERROR: Hash mismatch ${src_file}: ${src_hash} ${dest_file}: ${dest_hash}"
    fi

done

# Perform post-test cleanup

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance XDDCP: Multinic Test 5 - Check: PASSED."
  exit 0
else
  echo "Acceptance XDDCP: Multinic Test 5 - Check: FAILED."
  exit 1
fi
