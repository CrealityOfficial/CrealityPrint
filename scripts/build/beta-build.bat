@echo off
set APP_NAME=%1
set TAG_NAME=%2
set BUILD_TYPE=%3
set FACTORY_TYPE=%4


git show-ref %TAG_NAME% | awk -F ' ' '{print $1}' >cmmid
set /p CMMID=<cmmid
git rev-list HEAD | wc -l >maxcmmid
git rev-list %CMMID% | wc -l >tagcmmid
set /p MAXCMMID=<maxcmmid
set /p TAGCMMID=<tagcmmid
set /a TAGNUMB=%MAXCMMID%-%TAGCMMID%
set TAG_NAME=%TAG_NAME%.%TAGNUMB%
call %~dp0\build-vs2019.bat %TAG_NAME% package %BUILD_TYPE% %FACTORY_TYPE% %APP_NAME% || exit /b 2
set EXE_NAME=%APP_NAME%-%TAG_NAME%-win64-%BUILD_TYPE%.exe
echo "build"
	

echo SIGN_PACKAGE_PATH=%JOB_NAME%> var.prop
echo SIGN_PACKAGE_NAME=%EXE_NAME%>> var.prop
mkdir %JOB_NAME%
scp -r %JOB_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build
cd build
scp %EXE_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build/%JOB_NAME%/
	

