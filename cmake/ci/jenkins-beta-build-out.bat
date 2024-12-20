@echo off
set APP_NAME=%1
set TAG_NAME=%2
set BUILD_TYPE=%3
set SIGIN=%4
set CUSTOM_TYPE=%5
set PACKAGE_CMD=package
if [%6] == [1] (
	set PACKAGE_CMD=package_zip
)
if [%7] == [] (
	set USE_LOCAL_PARAM_PACKAGE=0
) else (
	set USE_LOCAL_PARAM_PACKAGE=%7
)
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
echo "build:" %PACKAGE_CMD%
rem rd /s /q build
echo %USE_LOCAL_PARAM_PACKAGE%
if not exist build md build
if exist build\CMakeCache.txt del build\CMakeCache.txt
python cmake\ci\conan-cmake.py -t jwin -b %BUILD_TYPE% -n %APP_NAME% -p %USE_LOCAL_PARAM_PACKAGE%

git show-ref %TAG_NAME% | awk -F ' ' '{print $1}' >cmmid
set /p CMMID=<cmmid
git rev-list HEAD | wc -l >maxcmmid
git rev-list %CMMID% | wc -l >tagcmmid
set /p MAXCMMID=<maxcmmid
set /p TAGCMMID=<tagcmmid
set /a TAGNUMB=%MAXCMMID%-%TAGCMMID%
set TAG_NAME=%TAG_NAME%.%TAGNUMB%
call %~dp0\build-vs2019.bat %TAG_NAME% %PACKAGE_CMD% %BUILD_TYPE% %SIGIN% %APP_NAME% %CUSTOM_TYPE% "ON" %USE_LOCAL_PARAM_PACKAGE% || exit /b 2
set EXE_NAME=%APP_NAME%-%TAG_NAME%-win64-%BUILD_TYPE%.exe
echo PACKAGE_CMD = %PACKAGE_CMD%
if [%PACKAGE_CMD%] == [package_zip] (
  set EXE_NAME=%APP_NAME%-%TAG_NAME%-win64-%BUILD_TYPE%.zip
  echo CUS_PACKAGE_TYPE= %PACKAGE_CMD%
) else (
  cd build
  "C:\curl.exe" -X POST -F file=@%EXE_NAME% http://172.20.180.14:3001/sign
  "C:\curl.exe" -L http://172.20.180.14:3001/exe/%EXE_NAME% -O
  cd ..
)
echo CUR_EXE_NAME = %EXE_NAME%
echo SIGN_PACKAGE_PATH=%JOB_NAME%> var.prop
echo SIGN_PACKAGE_NAME=%EXE_NAME%>> var.prop
mkdir %JOB_NAME%
scp -P 9122 -r %JOB_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build
scp -P 9122 build/%EXE_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build/%JOB_NAME%/%EXE_NAME%
