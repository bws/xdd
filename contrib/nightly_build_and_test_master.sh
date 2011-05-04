#!/bin/sh
#
# Nightly build and test script for XDD test master
#

#
# Create the nightly directory
#
datestamp=$(date +%Y-%m-%d-%H%M)
nightly_base_path=/home/nightly
nightly_dir=$nightly_base_path/$datestamp
mkdir -p $nightly_dir

#
# Setup the environment
#
cd $nightly_dir

#
# Run the nightly build and test on all platforms
#
tests="cray natureboy"
for t in $tests; do
    mkdir $t
    cd $t
    git archive --format=tar --prefix=xdd/ --remote=/nightly/xdd/repo/xdd.git master |tar xf -  xdd/contrib/xdd_nightly_build_and_test_${t}.sh
    xdd/contrib/nightly_build_and_test_${t}.sh >/dev/null 2>&1
done

#
# Collect the results
#
for t in $tests; do
    source $t/nightly_results
done

#
# Mail the results of the build and test to durmstrang-io
#
rcpt="durmstrang-io@email.ornl.gov"
if [ 0 -eq $BUILD_RC -a 0 -eq $CONFIG_RC -a 0 -eq $INSTALL_RC -a 0 -eq $TEST_RC ]; then
    body=$(cat $test_log)
    mail -s "SUCCESS - Nightly build and test for $datestamp" "$rcpt" <<EOF
$body.
EOF
else
    body=$(cat $test_log)
    mail -s "FAILURE - Nightly build and test for $datestamp" -a $config_log -a $build_log -a $install_log -a $test_log "$rcpt" <<EOF
Cray return code: $CRAY_RC
Configure return code: $CONFIG_RC
Build return code: $BUILD_RC
Build Tests return code: $BUILD_TEST_RC
Install return code: $INSTALL_RC
Test return code: $TEST_RC
Zero indicates success. See attachment for error logs.

$body
EOF
fi
