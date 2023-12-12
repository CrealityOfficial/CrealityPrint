@echo off

set Generator=Ninja
if [%1] == [] (
	set Generator=Visual Studio 16 2019
)

if "%1" == "Ninja" ^
(

set VSENV="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
call %VSENV% 
goto :build
) else (echo "not find vs in" %VSENV%)

set VSENV="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
call %VSENV% 
goto :build
) else (echo "not find vs in" %VSENV%)

set VSENV="D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (call %VSENV% ) else (exit /B)

)
else ^
(

call "%~dp0\find-msvcinfo.bat"
if not %errorlevel%==0 (exit /b 1)
		
call "%VSDir%\VC\Auxiliary\Build\vcvars64.bat"

REM 从注册表查询 win10 SDK 安装目录，主要用于判断注册表查询是否被禁，如果被禁 vcvars64.bat 将无法正常配置环境，需要手动配置环境依赖
call "%~dp0\check-vcvars64.bat"

)
:build
echo "build"

cd %~dp0

cd ../../

echo Generator : %Generator%
mkdir test-build
cd test-build
mkdir build
cd build

echo "Release"	
cmake ^
    -G "%Generator%" ^
    -DCMAKE_BUILD_TYPE=Release ^
	-DCC_UNIT_TESTING=ON ^
    ..\..\ || exit /b

rem Build and install the application
if "%1" == "Ninja" ^
(
ninja || exit /b -1
)

cd ../../
