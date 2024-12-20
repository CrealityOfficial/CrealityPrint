@echo off
set APP_NAME=%1
set TAG_NAME=%2
set BUILD_TYPE=%3
set SIGIN=%4
set CUSTOM_TYPE=%5

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
echo "build"
rem rd /s /q build
if not exist build md build
if exist build\CMakeCache.txt del build\CMakeCache.txt
python cmake\ci\conan-cmake.py -t jwin -b %BUILD_TYPE% -n %APP_NAME%

git show-ref %TAG_NAME% | awk -F ' ' '{print $1}' >cmmid
set /p CMMID=<cmmid
git rev-list HEAD | wc -l >maxcmmid
git rev-list %CMMID% | wc -l >tagcmmid
set /p MAXCMMID=<maxcmmid
set /p TAGCMMID=<tagcmmid
set /a TAGNUMB=%MAXCMMID%-%TAGCMMID%
set TAG_NAME=%TAG_NAME%.%TAGNUMB%
call %~dp0\build-vs2019.bat %TAG_NAME% package %BUILD_TYPE% %SIGIN% %APP_NAME% "%CUSTOM_TYPE%" "OFF"|| exit /b 2
set EXE_NAME=%APP_NAME%-%TAG_NAME%-win64-%BUILD_TYPE%.exe
echo "build"

echo SIGN_PACKAGE_PATH=%JOB_NAME%> var.prop
echo SIGN_PACKAGE_NAME=%EXE_NAME%>> var.prop
mkdir %JOB_NAME%
scp -r %JOB_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build
cd build
scp %EXE_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build/%JOB_NAME%/
