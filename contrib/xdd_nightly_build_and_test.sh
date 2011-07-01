#!/bin/sh -x
#
# Nightly build and test script for XDD
#

#
# Status flags
#
BUILD_RC=1
BUILD_TEST_RC=1
CONFIG_RC=1
INSTALL_RC=1
TEST_RC=1
CRAY_RC=1

#
# System locations
#
nightly_base_path=/home/nightly
xdd_repo_path=/home/nightly/xdd/repo/xdd.git
source_mnt=/mnt/disk1
dest_mnt=/mnt/disk2

datestamp=$(date +%Y-%m-%d-%H%M)
build_dir=$nightly_base_path/xdd/$datestamp/build
install_dir=$nightly_base_path/xdd/$datestamp/install
output_dir=$nightly_base_path/xdd/$datestamp/logs
test_dir=$nightly_base_path/xdd/$datestamp/test
test_src_dir=$source_mnt/xdd/$datestamp/test
test_dest_dir=$dest_mnt/xdd/$datestamp/test

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
mkdir -p $output_dir
mkdir -p $test_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir

#
# Check the results from the Cray 
#
CRAY_RC=0

#
# Retrieve and configure XDD
#
cd $build_dir >>$config_log 2>&1

git clone $xdd_repo_path >>$config_log 2>&1
cd xdd >>$config_log 2>&1
./configure --prefix=$install_dir >>$config_log  2>&1
CONFIG_RC=$?

#
# Build XDD
#
make >>$build_log 2>&1
BUILD_RC=$?

#
# Build XDD tests (relies on Fedora specific junk.  Will require fix.)
#
make test >>$build_log 2>&1
BUILD_TEST_RC=$?

#
# Install XDD
#
make install >>$install_log 2>&1
INSTALL_RC=$?

#
# Run the nightly tests
#
export PATH=$install_dir/bin:$PATH
test_local_dir=$test_src_dir/local_src
test_src_dir=$test_src_dir/
test_dest_dir=$test_dest_dir/
mkdir -p $test_local_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir
cd $test_dir
cat >test_config <<EOF
XDDTEST_XDD_EXE=$install_dir/bin/xdd.Linux
XDDTEST_XDDCP_EXE=$install_dir/bin/xddcp
XDDTEST_XDDFT_EXE=$install_dir/bin/xddft
XDDTEST_MPIL_EXE=$build_dir/xdd/contrib/mpil
XDDTEST_TESTS_DIR=$build_dir/xdd/tests
XDDTEST_LOCAL_MOUNT=$test_local_dir
XDDTEST_SOURCE_MOUNT=$test_src_dir
XDDTEST_DEST_MOUNT=$test_dest_dir
XDDTEST_OUTPUT_DIR=$output_dir
XDDTEST_E2E_SOURCE=localhost
XDDTEST_E2E_DEST=natureboy
EOF
$build_dir/xdd/tests/run_all_tests.sh >>$test_log 2>&1
TEST_RC=$?

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
