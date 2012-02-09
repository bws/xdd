#!/bin/bash
#
# Nightly build and test script for XDD on natureboy
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
nightly_base_path=$(pwd)/build_tests
xdd_repo_path=/ccs/proj/csc040/var/git/xdd.git
source_mnt=/data/xfs/source
dest_mnt=/data/xfs/dest

build_dir=$nightly_base_path/build
install_dir=$(pwd)
output_dir=$nightly_base_path/logs
test_src_dir=$source_mnt/xdd/$datestamp/test
test_dest_dir=$dest_mnt/xdd/$datestamp/test

build_log=$output_dir/nightly-build.log
config_log=$output_dir/nightly-config.log
install_log=$output_dir/nightly-install.log
test_log=$output_dir/nightly-test.log

srchost=$(hostname -s)
dsthost=$(hostname -s)

#
# Setup the scratch space
#
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $output_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir

#
# Setup the nightly tests config file
#
export PATH=$install_dir/bin:$PATH
test_local_dir=$test_src_dir/local_src
test_src_dir=$test_src_dir/
test_dest_dir=$test_dest_dir/
mkdir -p $test_local_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir
#cd $test_dir
cat >test_config <<EOF
XDDTEST_XDD_EXE=$install_dir/bin/xdd
XDDTEST_XDDCP_EXE=$install_dir/contrib/xddcp
XDDTEST_XDDFT_EXE=$install_dir/contrib/xddft
XDDTEST_XDD_PATH=${install_dir}/bin
XDDTEST_MPIL_EXE=$build_dir/xdd/contrib/mpil
XDDTEST_TESTS_DIR=$build_dir/xdd/tests
XDDTEST_LOCAL_MOUNT=$test_local_dir
XDDTEST_SOURCE_MOUNT=$test_src_dir
XDDTEST_DEST_MOUNT=$test_dest_dir
XDDTEST_OUTPUT_DIR=$output_dir
XDDTEST_E2E_SOURCE=${srchost}
XDDTEST_E2E_DEST=${dsthost}
XDDTEST_USER=${USER}
XDDTEST_XDD_REMOTE_PATH=${install_dir}/bin
XDDTEST_XDD_LOCAL_PATH=${install_dir}/bin
EOF
