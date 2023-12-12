@echo off
if [%1] == [] (
	echo "build Usage:"
	echo "Only build: version-check.bat [app name:Creative3D] [build type:Alpha]"
	exit /b 0
)
set APP_NAME=%1
set BUILD_TYPE=%2
set FACTORY_TYPE=%3

git tag -l "v*" --sort=taggerdate | tail -n1 >version.txt
set /p TAG_NAME=<version.txt

git show-ref %TAG_NAME% | awk -F ' ' '{print $1}' >cmmid
set /p CMMID=<cmmid
git rev-list HEAD | wc -l >maxcmmid
git rev-list %CMMID% | wc -l >tagcmmid
set /p MAXCMMID=<maxcmmid
set /p TAGCMMID=<tagcmmid
set /a TAGNUMB=%MAXCMMID%-%TAGCMMID%
set TAG_NAME=%TAG_NAME%.%TAGNUMB%

call %~dp0\build-vs2019.bat %TAG_NAME% package %BUILD_TYPE% %FACTORY_TYPE% || exit /b 2
echo "build"
set OUT=%JOB_NAME%/MR%gitlabMergeRequestIid%/%BUILD_ID%

set EXE_NAME=%APP_NAME%-%TAG_NAME%-win64-%BUILD_TYPE%.exe

echo SIGN_PACKAGE_PATH=%OUT%> var.prop
echo SIGN_PACKAGE_NAME=%EXE_NAME%>> var.prop

mkdir %JOB_NAME%\MR%gitlabMergeRequestIid%\%BUILD_ID%

scp -r %JOB_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build || exit /b 2
cd build
scp %EXE_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build/%OUT%/