#!/bin/bash
if [ -z $1 ]; then
	echo "build Usage:"
	echo "Only build: build.sh v0.0.1"
	echo "Build and package:build.bat v0.1.0 package"
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
workdir=$(cd $(dirname $0); pwd)
cd $workdir
cd ..
mkdir out
cd out
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DPROJECT_VERSION_MAJOR=${MAJOR} -DPROJECT_VERSION_MINOR=${MINOR}  -DPROJECT_VERSION_PATCH=${PATCH} -DPROJECT_BUILD_ID=${BUILD} -DPROJECT_VERSION_EXTRA=${VERSION_EXTRA}
make -j8
cmake ../ -DCMAKE_BUILD_TYPE=Release -DPROJECT_VERSION_MAJOR=${MAJOR} -DPROJECT_VERSION_MINOR=${MINOR}  -DPROJECT_VERSION_PATCH=${PATCH} -DPROJECT_BUILD_ID=${BUILD} -DPROJECT_VERSION_EXTRA=${VERSION_EXTRA}
make -j8
sudo make install

if [ "$2" = "package" ]; then
    make package
fi