#!/bin/bash
if [ -z $1 ]; then
	echo "build Usage:"
	echo "Only build: build.sh v0.0.1.0"
	echo "Build and package:build.bat v0.1.0.0 package Alpha Creality_Print"
	exit 0
fi

if [ -z $3 ]; then
	VERSION_EXTRA="test"
else
	VERSION_EXTRA=$3
fi

if [ -z $4 ]; then
	APPNAME="Creality_Print"
else
	APPNAME=$4
fi

if [ -z $5 ]; then
	CUSTOM_TYPE=""
else
	CUSTOM_TYPE=$5
fi

TAGNAME=${1:1}
arr=(${TAGNAME//./ })
MAJOR=${arr[0]}
MINOR=${arr[1]}
PATCH=${arr[2]}
BUILD=${arr[3]}

mkdir mac-build
cd mac-build
mkdir build
cd build
cmake ../../ -DCMAKE_BUILD_TYPE=Release -DBUILD_PACKAGE=ON -DAPPNAME=${APPNAME} -DCUSTOM_TYPE=${CUSTOM_TYPE} -DPROJECT_VERSION_MAJOR=${MAJOR} -DPROJECT_VERSION_MINOR=${MINOR}  -DPROJECT_VERSION_PATCH=${PATCH} -DPROJECT_BUILD_ID=${BUILD} -DPROJECT_VERSION_EXTRA=${VERSION_EXTRA}
make -j8
#make install
if [ "$2" = "package" ]; then
make package
fi
