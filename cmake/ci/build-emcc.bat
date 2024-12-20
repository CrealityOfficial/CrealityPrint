@echo off

cd %~dp0

cd ../../

mkdir emcc-build
cd emcc-build
mkdir build
cd build
if "%1"=="-D" (
    echo "Build Debug"
    call emcmake cmake ../../ -G Ninja -DCMAKE_BUILD_TYPE=Debug
) else (
    call emcmake cmake ../../ -G Ninja
)


rem Build and install the application

call emmake ninja || exit /b

cd ..\..\