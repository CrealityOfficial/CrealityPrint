@echo off

call "%~dp0\find-msvcinfo.bat"
if not %errorlevel%==0 (exit /b 1)
		
call "%VSDir%\VC\Auxiliary\Build\vcvars64.bat"

REM 从注册表查询 win10 SDK 安装目录，主要用于判断注册表查询是否被禁，如果被禁 vcvars64.bat 将无法正常配置环境，需要手动配置环境依赖
call "%~dp0\check-vcvars64.bat"

:build
echo "build"

cd %~dp0

cd ../../

mkdir ninja-build
cd ninja-build
mkdir build
cd build

echo "Debug"
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    ..\..\ || exit /b
	
ninja || exit /b

echo "Release"	
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..\..\ || exit /b

rem Build and install the application

ninja || exit /b

ninja install || exit /b

cd ..\..\