#!/bin/sh 
#
# Acceptance test for XDD.
#
# Validate the gethostip program
#
# Description - Get the IP address of the local host
#

#
# Test identity
#
test_name=$(basename $0)
echo "Beginning test . . ."
#
# Source the test configuration environment
#
source ./test_config

#
# Check IP of localhost
#
test_passes=0
ip=$($XDDTEST_XDD_GETHOSTIP_EXE -d localhost)
if [ "127.0.0.1" = "$ip" ]; then
    test_passes=1
fi

# Output test result
if [ "1" == "$test_passes" ]; then
  echo "Acceptance Test - $test_name: PASSED."
  exit 0
else
  echo "Acceptance Test - $test_name: FAILED."
  exit 1
fi
