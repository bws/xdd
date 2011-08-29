#!/bin/sh 
#
# Acceptance test for XDD.
#
# Check file restart option at destination using xddcp
#

# Parameters
xdd=xdd
xddcp=xddcp
log_file=restart1.log
data_file=/data/xfs/${USER}/file
correct_size=12345678913
HDestin=pod10
HDestinIP=192.168.1.5
HDestinIP=pod10
#end Parameters

               rm -f $data_file
ssh ${HDestin} rm -f $data_file

#create source file
$xdd -op write -datapattern random -bytes $correct_size -targets 1  ${data_file}   > ${log_file} 2>&1
#create destination file 2 x source file size
#let "nbytes2 = $correct_size * 1"
#ssh ${HDestin} $xdd -op write -datapattern random -bytes $nbytes2 -targets 1  ${data_file}   > ${log_file} 2>&1

$xddcp ${data_file} ${HDestinIP}:${data_file} &
pid=$$
echo "xddcp started, pid=$pid"
#set delays to suit
for i in 8 6 4 2 1; do
echo "sleep $i"
sleep $i
#3 methods of getting killed
#1. kill xddcp script
echo "kill -3 $pid"
kill -3 $pid
#2. Kill xdd at source
                pgrep -u ${USER} -f xdd | xargs kill -9 2> /dev/null
#3. Kill xdd at destination
ssh ${HDestin} "pgrep -u ${USER} -f xdd | xargs kill -9"
#### end of kill methods
sleep $i
$xddcp -a ${data_file} ${HDestinIP}:${data_file} &
pid=$$
echo "xddcp restarted, pid=$pid"
done
wait

# Validate output

#########################
#check the files
#########################
                 md5sum             ${data_file}   > restart.md5sum.src &
ssh ${HDestin}   md5sum             ${data_file}   > restart.md5sum.dst
wait

test_passes=1
diff restart.md5sum.src restart.md5sum.dst 
 if [ $? -ne 0 ]; then
   test_passes=0
    echo "Incorrect file md5sum for ${HDestin}:${data_file}."
  fi

# Perform post-test cleanup
#               rm -f $data_file
#ssh ${HDestin} rm -f $data_file
#              rm restart.md5sum.*

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test - restart1: PASSED."
else
  echo "Acceptance Test - restart1: FAILED."
fi

