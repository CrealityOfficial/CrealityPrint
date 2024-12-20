#!/bin/bash

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++


#mkdir linux-build
#cd linux-build
#mkdir build
#cd build
python cmake/ci/conan-cmake.py -t linux
 
#conan install -g cmake_multi -s build_type=Debug ../../
#conan install -g cmake_multi -s build_type=Release ../../

 
cmake ../../ -DCMAKE_USE_CONAN=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install/

#cd ../../

