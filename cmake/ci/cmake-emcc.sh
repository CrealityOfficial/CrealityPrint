#!/bin/bash
echo "$(dirname "$0" && pwd)" 
pwd
emcmake cmake -S ../../ -B ../../emcc-build/build 
cd ..
cd ..
