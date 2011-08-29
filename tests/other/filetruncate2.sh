#!/bin/sh
#
# Acceptance test for XDD.
#
# Check file truncation xdd write only
#

# Parameters
xdd=xdd
xddcp=xddcp
log_file=filetruncate2.log
data_file=/data/xfs/${USER}/file
correct_size=1234567891
HDestin=pod10
HDestinIP=192.168.1.5
#end Parameters

rm -f $data_file

#create smaller file
$xdd -op write -datapattern random -bytes $correct_size -targets 1  ${data_file}   > ${log_file} 2>&1
#save size for comparsion
ls -sh ${data_file}    > filetruncate.size.src
rm -f $data_file
#create bigger file
let "nbytes2 = $correct_size * 2"
$xdd -op write -datapattern random -bytes $nbytes2 -targets 1  ${data_file}   > ${log_file} 2>&1
#create smaller file with same name
$xdd -op write -datapattern random -bytes $correct_size -targets 1  ${data_file}   > ${log_file} 2>&1
#save size for comparison
ls -sh ${data_file} > filetruncate.size.dst

# Validate output

test_passes=1
diff filetruncate.size.src filetruncate.size.dst 
 if [ $? -ne 0 ]; then
   test_passes=0
    echo "Incorrect file size for ${data_file}."
  fi

# Perform post-test cleanup
               rm -f $data_file
               rm filetruncate.size.*

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test - filetruncate2: PASSED."
else
  echo "Acceptance Test - filetruncate2: FAILED."
fi

