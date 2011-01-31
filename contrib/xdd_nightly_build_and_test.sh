#!/bin/sh
#
# Nightly build and test script for XDD
#

#
# Status flags
#
BUILD_RC=1
CONFIG_RC=1
INSTALL_RC=1
TEST_RC=1
CRAY_RC=1

#
# System locations
#
nightly_base_path=/nightly
xdd_repo_path=/nightly/xdd/repo/xdd.git

datestamp=$(date +%Y-%m-%d-%H:%M)
build_dir=$nightly_base_path/${USER}/$datestamp/build
install_dir=$nightly_base_path/${USER}/$datestamp/install
test_dir=$nightly_base_path/${USER}/$datestamp/test
output_dir=$nightly_base_path/${USER}/$datestamp/logs
build_log=$output_dir/nightly-build.log
config_log=$output_dir/nightly-config.log
install_log=$output_dir/nightly-install.log
test_log=$output_dir/nightly-test.log
cray_log=$output_dir/nightly-cray.log


#
# Setup the scratch space
#
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $test_dir
mkdir -p $output_dir

#
# Check the results from the Cray 
#
CRAY_RC=0

#
# Retrieve and configure XDD
#
cd $build_dir >>$config_log
git clone $xdd_repo_path >>$config_log
cd xdd >>$config_log
./configure --prefix=$install_dir >>$config_log
CONFIG_RC=$?

#
# Build XDD
#
make >>$build_log
BUILD_RC=$?

#
# Install XDD
#
make install >>$install_log
INSTALL_RC=$?

#
# Run the nightly tests
#
export PATH=$install_dir/bin:$PATH
test_local_dir=$test_dir/local_src
test_src_dir=$test_dir/nightly_src
test_dest_dir=$test_dir/nightly_dest
mkdir -p $test_local_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir
cd $test_dir
cat >test_config <<EOF
XDDTEST_XDD_EXE=$install_dir/bin/xdd.Linux
XDDTEST_XDDCP_EXE=$install_dir/bin/xddcp
XDDTEST_XDDFT_EXE=$install_dir/bin/xddft
XDDTEST_TESTS_DIR=$build_dir/xdd/tests
XDDTEST_LOCAL_MOUNT=$test_local_dir
XDDTEST_SOURCE_MOUNT=$test_src_dir
XDDTEST_DEST_MOUNT=$test_dest_dir
XDDTEST_OUTPUT_DIR=$output_dir
XDDTEST_E2E_SOURCE=localhost
XDDTEST_E2E_DEST=natureboy
EOF
$build_dir/xdd/tests/run_all_tests.sh >>$test_log
TEST_RC=$?

#
# Mail the results of the build and test to durmstrang-io
#
rcpt="durmstrang-io@email.ornl.gov"
rcpt="settlemyerbw@ornl.gov"
if [ 0 -eq $BUILD_RC -a 0 -eq $CONFIG_RC -a 0 -eq $INSTALL_RC -a 0 -eq $TEST_RC ]; then
    mail -s "SUCCESS - Nightly build and test for $datestamp" "$rcpt" <<EOF
Everything completed successfully as far as I can tell.
EOF
else
    mail -s "FAILURE - Nightly build and test for $datestamp" -a $logfile "$rcpt" <<EOF
Cray return code: $CRAY_RC
Configure return code: $CONFIG_RC
Build return code: $BUILD_RC
Install return code: $INSTALL_RC
Test return code: $TEST_RC
Zero indicates success. See attachment for error logs.
EOF
fi
