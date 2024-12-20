#!/bin/bash

mkdir mac-build
cd mac-build
mkdir build
cd build

cmake ../../ -DCMAKE_BUILD_TYPE=Debug \
			 -G "Xcode"

