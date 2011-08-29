#!/bin/sh
#
# Acceptance test for XDD.
#
# Check file truncation at destination using xddcp
#

# Parameters
xdd=xdd
xddcp=xddcp
log_file=filetruncate1.log
data_file=/data/xfs/${USER}/file
correct_size=1234567891
HDestin=pod10
HDestinIP=192.168.1.5
#end Parameters

               rm -f $data_file
ssh ${HDestin} rm -f $data_file

#create source file
$xdd -op write -datapattern random -bytes $correct_size -targets 1  ${data_file}   > ${log_file} 2>&1
#(echo -n MD5SUM=;md5sum                                       ${data_file} ) >> ${log_file} 2>&1
#create destination file 2 x source file size
let "nbytes2 = $correct_size * 2"
ssh pod10 $xdd -op write -datapattern random -bytes $nbytes2 -targets 1  ${data_file}   > ${log_file} 2>&1

$xddcp ${data_file} ${HDestinIP}:${data_file}

# Validate output

                  ls -sh ${data_file}    > filetruncate.size.src
ssh ${HDestin}  "(ls -sh ${data_file} )" > filetruncate.size.dst

test_passes=1
diff filetruncate.size.src filetruncate.size.dst 
 if [ $? -ne 0 ]; then
   test_passes=0
    echo "Incorrect file size for ${HDestin}:${data_file}."
  fi

# Perform post-test cleanup
               rm -f $data_file
ssh ${HDestin} rm -f $data_file
               rm filetruncate.size.*

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test - filetruncate1: PASSED."
else
  echo "Acceptance Test - filetruncate1: FAILED."
fi

