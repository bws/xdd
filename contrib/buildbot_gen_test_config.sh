#!/bin/bash
#
# Generate the test configuration for a buildbot slave
#
# Step 1:  Generate a local test_config
# Step 2:  autoconf
# Step 3:  ./configure
#

#
# Buildslave volumes
#
buildslave_host=`hostname -s`
nightly_base_path=$(pwd)/build_tests

#
# Default test settings
#
nightly_base_path=$(pwd)/build_tests
source_mnt=/data/xfs/$USER/source
dest_mnt=/data/xfs/$USER/dest
srchost=${buildslave_host}
dsthost=${buildslave_host}
test_timeout=600

#
#
#
configure_flags="--enable-debug"

#
# Overrides for specific hosts
#
case "${buildslave_host}" in
    pod7)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        configure_flags="$configure_flags --disable-numa"
        ;;
    pod9)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        configure_flags="$configure_flags --disable-numa"
        ;;
    pod10)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        configure_flags="$configure_flags --disable-numa"
        ;;
    pod11)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        configure_flags="$configure_flags --disable-numa"
        ;;
    spry02)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/$USER/source
        dest_mnt=/data/$USER/dest
        configure_flags="CC=gcc $configure_flags --disable-numa --disable-xfs"
        ;;
    natureboy)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/hfsplus/$USER/source
        dest_mnt=/data/hfsplus/$USER/dest
        srchost=${buildslave_host}
        dsthost=localhost
        configure_flags="$configure_flags --disable-numa --disable-xfs --disable-ib"
        ;;
esac

#
# Testing locations
#
build_dir=${nightly_base_path}/build
install_dir=${nightly_base_path}/install
output_dir=${nightly_base_path}/logs
test_src_dir=${source_mnt}/test-data
test_dest_dir=${dest_mnt}/test-data
test_local_dir=${source_mnt}/local-test-data

#
# Setup the scratch space
#  (this needs better checking)
#
rm -rf $source_mnt
rm -rf $dest_mnt
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $output_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir
mkdir -p $test_local_dir

#
# Generate the nightly tests config file
#
cat >test_config <<EOF
XDDTEST_TESTS_DIR=$(pwd)/tests
XDDTEST_XDD_EXE=$install_dir/bin/xdd
XDDTEST_XDD_GETFILESIZE_EXE=$install_dir/bin/xdd-getfilesize
XDDTEST_XDD_GETHOSTIP_EXE=$install_dir/bin/xdd-gethostip
XDDTEST_XDD_PLOT_TSDUMPS_EXE=$install_dir/contrib/xdd-plot-tsdumps
XDDTEST_XDD_PLOT_TSDUMPS_DK_EXE=$install_dir/contrib/xdd-plot-tsdumps-dk
XDDTEST_XDD_READ_TSDUMPS_EXE=$install_dir/bin/xdd-read-tsdumps
XDDTEST_XDD_TRUNCATE_EXE=$install_dir/bin/xdd-truncate
XDDTEST_XDDCP_EXE=$install_dir/src/tools/xddcp/xddcp
XDDTEST_XDDFT_EXE=$install_dir/src/tools/xddft/xddft
XDDTEST_XDDFT_EXE=$install_dir/src/tools/xddmcp/xddmcp
XDDTEST_XDD_PATH=${install_dir}/bin
XDDTEST_MPIL_EXE=$build_dir/xdd/contrib/mpil
XDDTEST_LOCAL_MOUNT=$test_local_dir
XDDTEST_SOURCE_MOUNT=$test_src_dir
XDDTEST_DEST_MOUNT=$test_dest_dir
XDDTEST_OUTPUT_DIR=$output_dir
XDDTEST_E2E_SOURCE=${srchost}
XDDTEST_E2E_DEST=${dsthost}
XDDTEST_USER=${USER}
XDDTEST_E2E_DEST_XDD_PATH=${install_dir}/bin
XDDTEST_E2E_SOURCE_XDD_PATH=${install_dir}/bin
XDDTEST_XDD_REMOTE_PATH=${install_dir}/bin
XDDTEST_XDD_LOCAL_PATH=${install_dir}/bin
XDDTEST_TIMEOUT=${test_timeout}
EOF

#
# Perform the configure
#
autoconf
./configure --prefix=${install_dir} $configure_flags
