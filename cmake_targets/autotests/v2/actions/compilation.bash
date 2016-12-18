cd /tmp/oai_test_setup/oai
source oaienv
cd cmake_targets
rm -rf log
mkdir -p log
./build_oai $BUILD_ARGUMENTS
ls $BUILD_OUTPUT
