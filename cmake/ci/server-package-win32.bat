@echo off
if [%1] == [] (
	echo "build Usage:"
	echo "Only build: build.bat v0.1.0.1"
	echo "Build and package:build.bat v0.1.0.1 package alpha"
	exit /b 0
)
if [%3] == [] (
	set VERSION_EXTRA="test"
) else (
	set VERSION_EXTRA=%3%
)
set APP_VER=%1
set APP_VER=%APP_VER:~1%
for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
set MAJOR=%%a
set APP_VER=%%b
echo %MAJOR%
)
for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
set MINOR=%%a
set APP_VER=%%b
echo %MINOR%
)
for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
set PATCH=%%a
set APP_VER=%%b
echo %PATCH%
)
for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
set BUILD=%%a
set APP_VER=%%b
echo %BUILD%
)
echo %MAJOR%
echo %MINOR%
echo %PATCH%
echo %BUILD%
echo %VERSION_EXTRA%

@echo off

rem Configure the application in the current directory
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
echo "build"

rd /s /q out
mkdir out
cd out

echo "Debug"
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=out ^
	-DPROJECT_VERSION_MAJOR=%MAJOR% ^
	-DPROJECT_VERSION_MINOR=%MINOR% ^
	-DPROJECT_VERSION_PATCH=%PATCH% ^
	-DPROJECT_BUILD_ID=0 ^
	-DPROJECT_VERSION_EXTRA=%VERSION_EXTRA% ^
    ..\ || exit /b
	
ninja || exit /b

echo "Release"	
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=out ^
	-DPROJECT_VERSION_MAJOR=%MAJOR% ^
	-DPROJECT_VERSION_MINOR=%MINOR% ^
	-DPROJECT_VERSION_PATCH=%PATCH% ^
	-DPROJECT_BUILD_ID=%BUILD% ^
	-DPROJECT_VERSION_EXTRA=%VERSION_EXTRA% ^
    ..\ || exit /b

rem Build and install the application

ninja || exit /b

if "%2"=="package" (
ninja install || exit /b
ninja package || exit /b
)

cd ..\