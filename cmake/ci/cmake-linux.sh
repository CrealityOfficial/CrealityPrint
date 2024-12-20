#!/bin/bash

# workdir=$(cd $(dirname $0); pwd)
# echo ${workdir}
# cd $workdir
# cd ../../
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

mkdir linux-build
cd linux-build
mkdir build
cd build

cmake ../../ -DCMAKE_BUILD_TYPE=Release

