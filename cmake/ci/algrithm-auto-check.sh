#!/bin/bash

PATH=$PATH:/home/cxsw/.local/bin/
origin_path=$PWD

#compile
echo "compile"
python3 cmake/ci/conan-cmake.py -t linux || exit -2
cmake -S . -B linux-build/build/ -DONLY_UNIT_TESTS=ON
cd linux-build/build/
ninja all
cd $origin_path

#test
pwd
echo "run test"
python3 cmake/ci/algrithm-auto-check.py $1 || exit -2