@echo off

set VSENV="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
call %VSENV% 
goto :build
) else (echo "not find vs in" %VSENV%)



set VSENV="D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
call %VSENV% 
goto :build
) else (echo "not find vs in" %VSENV%)



set VSENV="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
call %VSENV% 
goto :build
) else (echo "not find vs in" %VSENV%)

set VSENV="D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (call %VSENV% ) else (exit /B)

:build

cd ..
rd /s /q ninja
mkdir ninja
cd ninja

mkdir build
cd build

echo "Debug"
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Debug ^
	-DCMAKE_INSTALL_PREFIX=../../install ^
    ..\..\c3d || exit /b
	
ninja || exit /b

echo "Release"	
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=../../install ^
    ..\..\c3d || exit /b

rem Build and install the application

ninja || exit /b

ninja install || exit /b

cd ..\..\c3d