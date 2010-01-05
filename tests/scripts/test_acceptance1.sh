#!/bin/sh
#
# Acceptance test for XDD.
#
# Validate the output results of -createnewfiles
#

# Locations
xdd=xdd.Linux
log_file=acceptance1.log
data_file=acceptance1.dat

# Perform pre-test cleanup
echo "Beginning Acceptance Test 1 . . ."
rm -f $data_file.*

# ReqSize 4096, Bytes 1GiB, Targets 1, QueueDepth 4, Passes 10
$xdd -op write -reqsize 4096 -mbytes 1024 -targets 1 $data_file -qd 4 -createnewfiles -passes 10 -datapattern random  >& $log_file

# Validate output
test_passes=1
correct_size=1073741824

for f in `ls $data_file.*`; do 
  file_size=`stat -c %s $f`
  if [ "$correct_size" != "$file_size" ]; then
    test_passes=0
    echo "Incorrect file size for $f.  Size is $file_size but should be $correct_size."
  fi
done

# Perform post-test cleanup
rm $data_file.*

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test 1 - Createnewfiles Size Check: PASSED."
else
  echo "Acceptance Test 1 - Createnewfiles Size Check: FAILED."
fi
