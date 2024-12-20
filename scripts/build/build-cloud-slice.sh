#!/bin/bash

if [ -z $1 ]; then
    echo "build Usage:"
    echo "Only build: build.sh v0.0.1.0"
    echo "Build and package:build.bat v0.1.0 package Beta"
    exit 0
fi
if [ -z $3 ]; then
	VERSION_EXTRA="test"
else
	VERSION_EXTRA=$3
fi
TAGNAME=${1:1}
arr=(${TAGNAME//./ })
MAJOR=${arr[0]}
MINOR=${arr[1]}
PATCH=${arr[2]}
BUILD=${arr[3]}

# 你的qt路径
#export QT5_DIR=/opt/Qt5.13.2/5.13.2/gcc_64/
export QT5_DIR=/opt/Qt5.12.8/5.12.8/gcc_64

if [ ! -d linux-build ]; then
    mkdir linux-build
fi

cd linux-build
cmake ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/creality -DPROJECT_VERSION_MAJOR=${MAJOR} -DPROJECT_VERSION_MINOR=${MINOR}  -DPROJECT_VERSION_PATCH=${PATCH} -DPROJECT_BUILD_ID=${BUILD} -DPROJECT_VERSION_EXTRA=${VERSION_EXTRA} -DUSE_PROTOBUF_SRC=OFF -DUSE_DLL_SLICER=OFF
make -j4

sudo make install

if [ "$2" = "package" ]; then
    make package
fi

