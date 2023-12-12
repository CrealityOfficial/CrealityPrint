#!/bin/bash

export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

workdir=$(cd $(dirname $0); pwd)
echo ${workdir}
cd $workdir
cd ../../

mkdir mac-build
cd mac-build
mkdir build
mkdir lib

cd lib
mkdir Release
cd ..
mkdir bin
cd bin
mkdir Release
cd ..

cd build

cmake ../../ -DCMAKE_BUILD_TYPE=Debug
make -j8
cmake ../../ -DCMAKE_BUILD_TYPE=Release
make -j8
sudo make install

cd ../../
