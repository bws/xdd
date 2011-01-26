#!/bin/sh
#
# Nightly build and test script for XDD
#

#
# Status flags
#
BUILD_RC=1
INSTALL_RC=1
TEST_RC=1

#
# System locations
#
nightly_base_path=/nightly
xdd_repo_path=/nightly/xdd/repo/xdd.git

datestamp=$(date +%s)
build_dir=$nightly_base_path/${USER}/$datestamp/build
install_dir=$nightly_base_path/${USER}/$datestamp/install
test_dir=$nightly_base_path/${USER}/$datestamp/test


#
# Setup the scratch space
#
mkdir -p $build_dir
mkdir -p $install_dir
mkdir -p $test_dir

#
# Build and install XDD
#
cd $build_dir
git clone $xdd_repo_path
cd xdd
./configure --prefix=$install_dir
make
BUILD_RC=0

make install
INSTALL_RC=0

#
# Run the nightly tests
#
export PATH=$install_dir/bin:$PATH
if [ "$(which xdd)" = "$install_dir/bin/xdd" ]; then
    echo "ERROR: Not using the nightly test version of XDD"
    exit 1
fi

cd $test_dir
$build_dir/tests/run_all_tests.sh
TEST_RC=0

#
# Mail the results of the build and test to durmstrang-io
#
rcpt="durmstrang-io@email.ornl.gov"
if [ 0 -eq $BUILD_RC -a 0 -eq $INSTALL_RC -a 0 -eq $TEST_INSTALL ]; then
    #mail -s "SUCCESS - Nightly build and test for $datestamp" "$rcpt" <<EOF
echo <<EOF 
Everything completed successfully as far as I can tell.
EOF
else
    #mail -s "FAILURE - Nightly build and test for $datestamp" -a $logfile "$rcpt" <<EOF
echo <<EOF 
Build return code: $BUILD_RC
Install return code: $INSTALL_RC
Test return code: $TEST_RC
See attachment for errors.
EOF
fi
