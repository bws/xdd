#!/bin/bash
#
# Generate the test configuration for use by a buildbot slave.
#

#
# Buildslave volumes
#
buildslave_host=`hostname -s`
nightly_base_path=$(pwd)/build_tests
case "${buildslave_host}" in
    pod7)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        ;;
    pod9)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        ;;
    pod10)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        ;;
    pod11)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/xfs/$USER/source
        dest_mnt=/data/xfs/$USER/dest
        ;;
    spry01)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/$USER/source
        dest_mnt=/data/$USER/dest
        ;;
    natureboy)
        nightly_base_path=$(pwd)/build_tests
        source_mnt=/data/hfsplus/$USER/source
        dest_mnt=/data/hfsplus/$USER/dest
        ;;
esac

#
# Testing locations
#
build_dir=${nightly_base_path}/build
install_dir=$(pwd)
output_dir=${nightly_base_path}/logs
test_src_dir=${source_mnt}/test-data
test_dest_dir=${dest_mnt}/test-data
test_local_dir=${source_mnt}/local-test-data

#
# Other test settings
#
test_timeout=600
srchost=${buildslave_host}
dsthost=${buildslave_host}

#
# Setup the scratch space
#  (this needs better checking)
#
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $output_dir
mkdir -p $test_src_dir
mkdir -p $test_dest_dir
mkdir -p $test_local_dir

#
# Setup the nightly tests config file
#
cat >test_config <<EOF
XDDTEST_XDD_EXE=$install_dir/bin/xdd
XDDTEST_XDD_GETFILESIZE_EXE=$install_dir/bin/xdd-getfilesize
XDDTEST_XDD_GETHOSTIP_EXE=$install_dir/bin/xdd-gethostip
XDDTEST_XDD_PLOT_TSDUMPS_EXE=$install_dir/contrib/xdd-plot-tsdumps
XDDTEST_XDD_PLOT_TSDUMPS_DK_EXE=$install_dir/contrib/xdd-plot-tsdumps-dk
XDDTEST_XDD_READ_TSDUMPS_EXE=$install_dir/bin/xdd-read-tsdumps
XDDTEST_XDD_TRUNCATE_EXE=$install_dir/bin/xdd-truncate
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
XDDTEST_TIMEOUT=${test_timeout}
EOF
