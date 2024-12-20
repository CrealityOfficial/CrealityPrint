@echo off
if [%1] == [] (
	echo "build Usage:"
	echo "Only build: build.bat v0.1.0.1"
	echo "Build and package:build.bat v0.1.0.1 package Alpha 0 Creality_Print"
	exit /b 0
)
if [%2] == [package] (
	set PACKAGE_TYPE="NSIS"
) else (
	set PACKAGE_TYPE="ZIP"
)

if [%3] == [] (
	set VERSION_EXTRA= "test"
) else (
	set VERSION_EXTRA=%3
)

if "%4" == "0" (
	set SIGN_PACKAGE="OFF"
) else (
	set SIGN_PACKAGE="ON"
)

if [%5] == [] (
	set APPNAME="Creality_Print"
) else (
  set APPNAME=%5
)

if [%6] == [] (
	set CUSTOM_TYPE=""
) else (
  set CUSTOM_TYPE=%6
)
if [%7] == [] (
	set SIGIN_PACKAGE_OUT="OFF"
) else (
  set SIGIN_PACKAGE_OUT=%7
)
if [%8] == [] (
	set USE_LOCAL_PARAM_PACKAGE=0
) else (
  set USE_LOCAL_PARAM_PACKAGE=1
)
set APP_VER=%1
set APP_VER=%APP_VER:~1%
for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
  set MAJOR=%%a
  set APP_VER=%%b
)

for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
  set MINOR=%%a
  set APP_VER=%%b
)

for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
  set PATCH=%%a
  set APP_VER=%%b
)

for /f "tokens=1,* delims=." %%a in ("%APP_VER%") do (
  set BUILD=%%a
  set APP_VER=%%b
)

echo MAJOR=%MAJOR%
echo MINOR=%MINOR%
echo PATCH=%PATCH%
echo BUILD=%BUILD%
echo VERSION_EXTRA=%VERSION_EXTRA%
echo PACKAGE_TYPE=%PACKAGE_TYPE%
echo APPNAME=%APPNAME%
echo CUSTOM_TYPE=%CUSTOM_TYPE%
echo SIGN_PACKAGE=%SIGN_PACKAGE%
echo USE_LOCAL_PARAM_PACKAGE=%USE_LOCAL_PARAM_PACKAGE%
rem Configure the application in the current directory

@REM example: VS_PATH=D:\Microsoft Visual Studio\2022\Enterprise in system PATH
set VSENV="%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if not "%VS_PATH%" == "" (
  if exist %VSENV% (
    call %VSENV%
    goto :build
  ) else (echo "not find vs in" %VSENV%)
) else (echo "not find vs in" %VSENV%)

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

:build
cd build
cmake ^
  -G"Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=out ^
  -DPROJECT_VERSION_MAJOR=%MAJOR% ^
  -DPROJECT_VERSION_MINOR=%MINOR% ^
  -DPROJECT_VERSION_PATCH=%PATCH% ^
  -DPROJECT_BUILD_ID=%BUILD% ^
  -DPROJECT_VERSION_EXTRA=%VERSION_EXTRA% ^
  -DAPPNAME=%APPNAME% ^
  -DCUSTOM_TYPE=%CUSTOM_TYPE% ^
  -DPACKAGE_TYPE=%PACKAGE_TYPE% ^
  -DSIGN_PACKAGE=%SIGN_PACKAGE% ^
  -DSIGN_PACKAGE_OUT=%SIGIN_PACKAGE_OUT% ^
  -DUSE_LOCAL_PARAM_PACKAGE=%USE_LOCAL_PARAM_PACKAGE% ^
  ..\ || exit /b 2

rem Build and install the application

ninja || exit /b 2
if "%2"=="package" (
  ninja install || exit /b 2
  ninja package || exit /b 2
)
else if "%2"=="package_zip" (
  ninja install || exit /b 2
  ninja package || exit /b 2
)
cd ..\
