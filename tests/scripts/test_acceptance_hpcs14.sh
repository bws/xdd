#! /bin/sh 

###################################
# gotta set PBS params manually
###################################
#PBS -q batch
#PBS -A csc040
#PBS -N S14
#PBS -j oe
#PBS -o ./results_Scenario_14_Random_Reads_N_Procs_1_File.$PBS_JOBID
#PBS -l walltime=0:05:00,nodes=1:ppn=8
let "totalcores_alloc        =8"
let "corespernode_alloc      =1"
let "totalnodes_alloc        =$totalcores_alloc / $corespernode_alloc"
let "max_apruns              =$totalcores_alloc"
let "memory_per_node         = 30 * 1024 *1024 *1024"

#
# Source the test configuration environment
#
source ./test_config

#
# Skip the test if the hardcoded stuff is too outlandish
#
real_num_cores=$(cat /proc/cpuinfo |grep processor |wc -l)
real_mem_total_kb=$(cat /proc/meminfo |grep MemTotal |awk  '{print $2}')
real_mem_total=$((real_mem_total_kb*1024))
if [ $totalcores_alloc -gt $real_num_cores ]; then
    echo "Acceptance XDD HPCS14: Scenario14 Test - Check: Skipped ($totalcores_alloc cores requested, $real_num_cores exist)."
    exit 2
elif [ $memory_per_node -gt $real_mem_total ]; then
    echo "Acceptance XDD HPCS14: Scenario14 Test - Check: Skipped ($memory_per_node bytes requested, $real_mem_total exist)."
    exit 2
fi

startime=$(date +%s)

# Perform pre-test 
echo "Beginning Acceptance Test HPCScenario #14 . . ."
test_dir=$XDDTEST_LOCAL_MOUNT/acceptance_s14
echo $test_dir
rm -rf $test_dir
mkdir -p $test_dir

TESTDIR=$test_dir
cd $TESTDIR
RESULTS="results_Scenario_14_Small_Random_Reads_N_Procs_1_File"

BINDIR=/home/nightly/bin

echo " /**************************************************************** "
echo " *                                                               * "
echo " *  Script for the following  scenario                           * "
echo " *                                                               * "
echo " * Performance Metrics for file systems                          * "
echo " *                                                               * "
echo " * Version: REPLACED_RELEASE_DATE                                * "
echo " *                                                               * "
echo " * Copyright (c) 2010, UT-Battelle, LLC.                         * "
echo " *                                                               * "
echo " * All Rights Reserved. See the file 'LICENSING.pdf' for the     * "
echo " * License Agreement.                                            * "
echo " *                                                               * "
echo " * Author: Stephen W. Hodson                                     * "
echo " *         Extreme Scale Systems Center                          * "
echo " *         Oak Ridge National Laboratory                         * "
echo " *         PO Box 2008 MS6164                                    * "
echo " *         Oak Ridge TN 37831-6164                               * "
echo " *         hodsonsw@ornl.gov                                     * "
echo " *                                                               * "
echo " * The author also wishes to acknowledge and thank the following * "
echo " * collaborators for our many conversations and their critiques, * "
echo " * questions, and suggestions:                                   * "
echo " *                                                               * "
echo " *         Stephen Poole                                         * "
echo " *         Jeffrey Kuehn                                         * "
echo " *         Brad Settlemyer                                       * "
echo " *                                                               * " 
echo " * Oak Ridge National Laboratory is operated by UT-Battelle, LLC * "
echo " * for the U.S. Department of Energy.                            * "
echo " *                                                               * "
echo " * This work was funded by the Department of Defense.            * "
echo " *                                                               * "
echo " ****************************************************************/ "
echo " "
echo "############################################################################################################################ "
echo "### SCENARIO 14 RANDOM I/O ON A SINGLE FILE "
echo "############################################################################################################################ "
echo "### 14.01 OBJECTIVE "
echo "### To perform both completely random read operations with variable record sizes and a backwards read operation with a "
echo "### random record size and skip increment to a single file. Reads might access some of the same data from different I/O "
echo "### requests. I/O is not atomic for a file. HPCS vendors are free to use asynchronous I/O and direct I/O. "
echo "### 14.02 DESCRIPTION "
echo "### N-processes to a single file, where N is tens (10s) of thousands. The method on how the file is written is not specified."
echo "### The file created should be at least ten (10) times larger than both the memory of the participating processors and the "
echo "### storage cache. The file is to be written sequentially and read randomly. Random I/O operations are not expected to "
echo "### completely read the file, but the count of the number of random I/O operations and the amount of data read is to be tracked. "
echo "### I/O request sizes after the initial file is written are to be a random value between 512 bytes "
echo "### (or 4096 bytes, if that is the default sector size) and 65536 bytes, with one of the tests sector aligned and the "
echo "### other testâ non-sector aligned for the two (2) cases of random read and reading backwards. "
echo "### As writes are being done to a single file with the potential for overlap: "
echo "###  1. Create file system, mount, and tune. "
echo "###  2. Allocate 50% of the machine  memory with integer values and write these values to disk. This step need not be timed. "
echo "###  3. Write the file using any method "
echo "###  4. Barrier all processes. "
echo "###  5. Start time counter and byte counter "
echo "###  6. Perform fifty (50) million sector aligned random reads between 512 bytes and 65536 bytes on "
echo "###     ???each of the files??? with N-processes to the file. "
echo "###  7. Output the time for the fifty (50) million reads and the number of bytes transferred. "
echo "###  8. Perform fifty (50) million non-sector aligned random reads between 512 bytes and 65536 bytes on "
echo "###     ???each of the files??? with N-processes to the file. "
echo "###  9. Output the time for the fifty (50) million reads and the number of bytes transferred. "
echo "### 10. Perform sector aligned reads on the file, reading backwards with reads of a random size between 512 bytes and 65536 bytes "
echo "###     skipping between each of the read requests a sector aligned random skip increment between 8192 bytes and 524288 bytes "
echo "###      with N-processes to ???N-files??? (Really, Henry means 1-file). "
echo "### 11. Output the time for the backwards reads and the number of bytes transferred. "
echo "### 12. Perform non-sector aligned reads on the file, reading backwards with reads of a random size between 512 bytes and 65536 bytes "
echo "###      skipping between each of the read requests a non-sector aligned random skip increment between 8192 bytes and 524288 bytes "
echo "###       with N-processes to ???N-files??? (Really, Henry means 1-file). "
echo "### 13. Output the time for the backwards reads and the number of bytes transferred. "
echo "### 14.03 EXPECTED RESULTS "
echo "### The output should provide the amount of data read and the time to read the data. It is hoped that the HPCS vendor will "
echo "### run the tests on at least three (3) different hardware configurations to understand the scalability of this random I/O test."
echo "### It is important to understand the performance as a percentage of hardware performance in terms of the number of random I/O "
echo "### operations that are available from the storage system and that random I/O scales in performance with the addition of "
echo "### more storage hardware. "
echo "############################################################################################################################ "

module load openmpi-x86_64
mpirun -v
LAUNCH_CMD="mpirun "
XDD=$XDDTEST_XDD_EXE
data_dir=$TESTDIR/testfiles
if [ ! -d \$data_dir ]; then
    echo "Creating directory: \$data_dir"
    mkdir -p $data_dir
fi

data_file=Scenario_14_file
#####################################################
# CHANGE the following parameters
#####################################################
set -x
#################################
# File system tuning parameters #
#################################
FSTYPE="generic"
let "MAX_OSTs              = 672"        #Maximum number of OSTs
let "OST_STRIPE_FILE       =   $totalnodes_alloc"  #number of OSTs to stripe file
OST_STRIPE_SIZE="1m"
#################################
IAM_ROOT="0"
let  "nodes_total          = $totalnodes_alloc"
let  "threads_per_target   = 1" 
let  "bytes_total          = 28 * 1024 * 1024 * 1024"
let  "bytes_per_node       = $bytes_total / $nodes_total"
let  "min_kbytes_per_req   =  4"
let  "max_kbytes_per_req   = 64"
let  "max_kbytes_read      = $bytes_total / 1024"
timelimit_secs="-timelimit  300"  #use time limit or bytes_per_node read
DIO="0"                      # Not 0 for sector aligned test
CREATE_FILES="1"
REMOVE_FILES="1"
#####################################################
# End - CHANGE the following parameters
#####################################################
let  "targets_per_node     = 1"
let  "targets_total        = 1"
let  "file_to_memory_ratio = $bytes_per_node / $memory_per_node"
set +x
echo "############################################################################################################################ "
echo "### Test Parameters ############# Value #################################################################################### "
echo "### nodes_total          = $nodes_total"
echo "### targets_per_node     = $targets_per_node"
echo "### targets_total        = $targets_total"
echo "### threads_per_target   = $threads_per_target"
echo "### bytes_per_node       = $bytes_per_node"
echo "### bytes_per_node       = $bytes_per_node"
echo "### memory_per_node      = $memory_per_node"
echo "### file_to_memory_ratio = $file_to_memory_ratio"
echo "### bytes_total          = $bytes_total"
echo "### min_kbytes_per_req   = $min_kbytes_per_req"
echo "### max_kbytes_per_req   = $max_kbytes_per_req"
echo "### max_kbytes_read      = $max_kbytes_read"
echo "### MAX_OSTs             = $MAX_OSTs"
echo "### OST_STRIPE_FILE      = $OST_STRIPE_FILE"
echo "### OST_STRIPE_SIZE      = $OST_STRIPE_SIZE"
echo "############################################################################################################################ "

verbose=" "

set -x
######################################################
#### start making files
######################################################
if [ "$CREATE_FILES" -ne "0" ]; then
rm -f ${data_dir}/${data_file}*
echo "########################################################"
echo "# Stripe single file accross $OST_STRIPE_FILE or to suit"
echo "########################################################"
date
let "itarget = 0"
let "iost    = 0"
while [ "$itarget" -lt "$targets_total" ]
do
if [ "$FSTYPE" == "lustre" ]; then
lfs setstripe -s ${OST_STRIPE_SIZE} -c ${OST_STRIPE_FILE} -i $iost ${data_dir}/${data_file}.${itarget}
fi
let "itarget = $itarget + 1"
let "iost    = $iost % $MAX_OSTs"
done
date

echo "####################################################################################################################"
echo "### N-processes to a single file, where N is tens (10s) of thousands. The method on how the file is written is not specified."
echo "### The file created should be at least ten (10) times larger than both the memory of the participating processors and the "
echo "### storage cache. The file is to be written sequentially and read randomly. Random I/O operations are not expected to "
echo "### completely read the file, but the count of the number of random I/O operations and the amount of data read is to be tracked. "
echo "### I/O request sizes after the initial file is written are to be a random value between 512 bytes "
echo "### (or 4096 bytes, if that is the default sector size) and 65536 bytes, with one of the tests sector aligned and the "
echo "### other tests non-sector aligned for the two (2) cases of random read and reading backwards. "
echo "### As writes are being done to a single file with the potential for overlap: "
echo "###  1. Create file system, mount, and tune. "
echo "###  2. Allocate 50% of the machines memory with integer values and write these values to disk. This step need not be timed. "
echo "###  3. Write the file using any method "
echo "###  4. Barrier all processes. "
echo "###  DO NOT use -dio on Lustre...performance is poor"
echo "###  Write file with all nodes...use startoffset "
echo "####################################################################################################################"
date
######################################################
#### build xdd setup files for each xdd instance
######################################################
xdd_setup_dir=$TESTDIR/S
if [ ! -d \$xdd_setup_dir ]; then
    echo "Creating directory: \$xdd_setup_dir"
    mkdir -p $xdd_setup_dir
fi 
cd $xdd_setup_dir
rm -f s*

let "reqsize      = 16 * 1024"
let "numreqs      = ${bytes_per_node} / $reqsize / 1024"
let      "itarget = 0"
let      "inode = 0"
TargetList="${data_dir}/${data_file}.$itarget"
let      "nodes_write = $bytes_total / $bytes_per_node"
while [ "$inode" -lt "$nodes_write" ]
do

let "startingblock = $inode * ${bytes_per_node} / 1024"
datapattern=" -datapattern random  "
echo "-targets ${targets_per_node} ${TargetList} -minall -numreqs $numreqs  -reqsize $reqsize -queuedepth $threads_per_target -op write $datapattern -startoffset $startingblock " > s.$inode

let "inode = $inode + 1"

done
cd $TESTDIR


echo "####################################################################################################################"
echo "### Now write this file ${TargetList}"
echo "####################################################################################################################"
set +x
${LAUNCH_CMD} -n $nodes_write $XDDTEST_MPIL_EXE ${XDD} -setup $xdd_setup_dir/s.: 2>&1 | tee $RESULTS
set +x

echo "####################################################################################################################"
echo "### Done creating file ${TargetList} "
echo "####################################################################################################################"

fi
######################################################
#### done making files
######################################################
# get time left to run 2 read tests, reduced by 60s buffer
elapsedtime="$(\echo "$(\date +%s) - $startime" | \bc)"
let "testime = $XDDTEST_TIMEOUT - $elapsedtime"
let "testime = $testime - 60"
let "testime = $testime / 2"
timelimit_secs="-timelimit  $testime"
echo "### timelimit_secs       = $timelimit_secs"
######################################################
#### build xdd setup files for each xdd instance
#### for both random and reverse "random" read tests
######################################################
xdd_setup_dir=$TESTDIR/s
if [ ! -d \$xdd_setup_dir ]; then
    echo "Creating directory: \$xdd_setup_dir"
    mkdir -p $xdd_setup_dir
fi
cd $xdd_setup_dir
#rm -f launch.opts
rm -f s* p* r*
let "itarget = 0"
let      "inode = 0"
while [ "$inode" -lt "$nodes_total" ]
do

TargetList="${data_dir}/${data_file}.$itarget"
let "reqsize =  $inode   * ${min_kbytes_per_req}" #request size in 1k byte blocks, multiple of sector size
let "reqsize =  $reqsize % ${max_kbytes_per_req}" #request size modulo ${max_kbytes_per_req}
if [ "$reqsize" -eq "0" ]
then
let "reqsize =  ${max_kbytes_per_req}"            #request size for ${max_kbytes_per_req} case
fi
let "range1kblocks  = $bytes_total / 1024 - $reqsize"
let "kbytes_to_read = $max_kbytes_read / $max_kbytes_per_req * $reqsize"
let "iseed = ${inode} + 1"

accessPATTERN="-seek random -seek range ${range1kblocks} -seek seed ${iseed} -seek save ${xdd_setup_dir}/p.${inode}"
echo "-targets ${targets_per_node} ${TargetList} -minall -kbytes ${kbytes_to_read} -reqsize $reqsize ${verbose} -queuedepth $threads_per_target -op read  ${accessPATTERN} ${timelimit_secs} " > s.$inode

accessPATTERN="-seek load ${xdd_setup_dir}/rseek_list.${inode}"
echo "-targets ${targets_per_node} ${TargetList} -minall -kbytes ${kbytes_to_read} -reqsize $reqsize ${verbose} -queuedepth $threads_per_target -op read  ${accessPATTERN} ${timelimit_secs} " > r.$inode

let "inode = $inode + 1"

done
cd ..

echo "#################################################################################################################"
echo "### sector aligned operations on Lustre...performance is bad...skip"
echo "#################################################################################################################"
if [ "$DIO" -ne "0" ]
then

date
echo "####################################################################################################################################"
echo "###  4. Barrier all processes. "
echo "###  5. Start time counter and byte counter "
echo "###  6. Perform fifty (50) million sector aligned random reads between 512 bytes and 65536 bytes on "
echo "###     ???each of the files??? with N-processes to the file. "
echo "###  7. Output the time for the fifty (50) million reads and the number of bytes transferred. "
echo "###  Implement by launching 1 process/node using $threads_per_target threads/target/node, $ntarget_per_node targets, $nodes_total nodes "
echo "###  Request size constant per xdd instance, but varies between 4-64 1K block accross xdd instances, each instance with different seed"
echo "####################################################################################################################################"

${LAUNCH_CMD} -n $nodes_total $XDDTEST_MPIL_EXE ${XDD} -dio -setup $xdd_setup_dir/s.: 2>&1 | tee -a $RESULTS

date
fi

date
echo "####################################################################################################################################"
echo "###  8. Perform fifty (50) million non-sector aligned random reads between 512 bytes and 65536 bytes on "
echo "###     ???each of the files??? with N-processes to the file. "
echo "###  9. Output the time for the fifty (50) million reads and the number of bytes transferred. "
echo "###  Implement by launching 1 process/node using $threads_per_target threads/target/node, $ntarget_per_node targets, $nodes_total nodes "
echo "###  Request size constant per xdd instance, but varies between 4-64 1K block accross xdd instances, each instance with different seed"
echo "####################################################################################################################################"

${LAUNCH_CMD} -n $nodes_total $XDDTEST_MPIL_EXE ${XDD} -setup $xdd_setup_dir/s.: 2>&1 | tee -a $RESULTS

date
echo "####################################################################################################################################"
echo "### Generate reverse sort seek lists "
echo "####################################################################################################################################"
cp /bin/sort .
${LAUNCH_CMD} -n $nodes_total $XDDTEST_MPIL_EXE ./sort -k 2 -o $xdd_setup_dir/rseek_list.: $xdd_setup_dir/p.:.T0Q0.txt

echo "#################################################################################################################################"
echo "### 10. Perform sector aligned reads on the file, reading backwards with reads of a random size between 512 bytes and 65536 bytes "
echo "###     skipping between each of the read requests a sector aligned random skip increment between 8192 bytes and 524288 bytes "
echo "###      with N-processes to ???N-files??? (Really, Henry means 1-file). "
echo "### 11. Output the time for the backwards reads and the number of bytes transferred. "
echo "#############################################################################################################################"
if [ "$DIO" -ne "0" ]
then
${LAUNCH_CMD} -n $nodes_total $XDDTEST_MPIL_EXE ${XDD} -dio -setup $xdd_setup_dir/r.: 2>&1 | tee -a $RESULTS
fi

date
echo "####################################################################################################################################"
echo "### 12. Perform non-sector aligned reads on the file, reading backwards with reads of a random size between 512 bytes and 65536 bytes "
echo "###      skipping between each of the read requests a non-sector aligned random skip increment between 8192 bytes and 524288 bytes "
echo "###       with N-processes to ???N-files??? (Really, Henry means 1-file). "
echo "### 13. Output the time for the backwards reads and the number of bytes transferred. "
echo "### Note these tests are aimed at defeating read ahead...."
echo "#################################################################################################################################"

${LAUNCH_CMD} -n $nodes_total $XDDTEST_MPIL_EXE ${XDD}       -setup $xdd_setup_dir/r.: 2>&1 | tee -a $RESULTS

echo "############################################################################################################################ "
echo "### 14.03 EXPECTED RESULTS "
echo "### The output should provide the amount of data read and the time to read the data. It is hoped that the HPCS vendor will "
echo "### run the tests on at least three (3) different hardware configurations to understand the scalability of this random I/O test."
echo "### It is important to understand the performance as a percentage of hardware performance in terms of the number of random I/O "
echo "### operations that are available from the storage system and that random I/O scales in performance with the addition of "
echo "### more storage hardware. "
echo "############################################################################################################################ "
date
if [ "$REMOVE_FILES" -ne "0" ]; then
rm -f ${data_dir}/${data_file}*
fi

date
echo "####################################################################################################################################"
echo "###  Sum columns 5, 6, & 9 to get total bytes, total ops, total ops/s respectively"
echo "####################################################################################################################################"
# Validate output
test_passes=1
grep COMB $RESULTS
if [ $? -ne 0 ]; then
   test_passes=0
  echo "No COMB lines"
fi

# Perform post-test cleanup
#rm -rf $test_dir

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test s14 - HPCScenario #14: PASSED."
  exit 0
else
  echo "Acceptance Test s14 - HPCScenario #14: FAILED."
  exit 1
fi
