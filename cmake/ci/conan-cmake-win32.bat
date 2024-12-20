@echo off

mkdir win32-build
cd win32-build
mkdir build
cd build

conan install -g cmake_multi -s build_type=Debug ../../
conan install -g cmake_multi -s build_type=Release ../../

@REM example: VS_VERSION=Visual Studio 17 2022 in system PATH
set Generator=%VS_VERSION%
if not "%VS_VERSION%" == "" (
    goto :start_cmake
)

set Generator=Visual Studio 16 2019

:start_cmake
echo "start_cmake"

cmake ../../ -G "%Generator%" -DCMAKE_USE_CONAN=ON

cd ../../
