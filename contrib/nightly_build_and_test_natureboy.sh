#!/bin/sh
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
nightly_base_path=$(pwd)
xdd_repo_path=/home/nightly/xdd/repo/xdd.git
source_mnt=/data/xfs1
dest_mnt=/data/xfs2

build_dir=$nightly_base_path/build
install_dir=$nightly_base_path/install
output_dir=$nightly_base_path/logs
test_src_dir=$source_mnt/xdd/$datestamp/test
test_dest_dir=$dest_mnt/xdd/$datestamp/test

build_log=$output_dir/nightly-build.log
config_log=$output_dir/nightly-config.log
install_log=$output_dir/nightly-install.log
test_log=$output_dir/nightly-test.log


#
# Setup the scratch space
#
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $output_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir

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
module load openmpi-x86_64
make test >>$build_log 2>&1
BUILD_TEST_RC=$?

#
# Install XDD
#
make install >>$install_log 2>&1
INSTALL_RC=$?

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

#
# Run the nightly tests
#
$build_dir/xdd/tests/run_all_tests.sh >>$test_log 2>&1
TEST_RC=$?

#
# Construct a file that contains all the pertinent info
#
results_file=~/xdd_nightly_results
echo "" > $results_file
echo "BUILD_RC=$BUILD_RC" >> $results_file
echo "CONFIG_RC=$CONFIG_RC" >> $results_file
echo "INSTALL_RC=$INSTALL_RC" >> $results_file
echo "TEST_RC=$TEST_RC" >> $results_file
echo "BUILD_RC=$BUILD_RC" >> $results_file
echo "CONFIG_RC=$CONFIG_RC" >> $results_file
echo "INSTALL_RC=$INSTALL_RC" >> $results_file
echo "TEST_RC=$TEST_RC" >> $results_file

#
# Exit with test return code
#
exit $TEST_RC
