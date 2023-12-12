@echo off

mkdir win32-build
cd win32-build
mkdir build
cd build

conan install -g cmake_multi -s build_type=Debug ../../
conan install -g cmake_multi -s build_type=Release ../../
cmake ../../ -G "Visual Studio 16 2019" -DCMAKE_USE_CONAN=ON
			 
cd ../../