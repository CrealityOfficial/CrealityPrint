@echo off
set APP_NAME=%1
set TAG_NAME=%2
set BUILD_TYPE=%3
set FACTORY_TYPE=%4
set EXE_NAME=%APP_NAME%-%TAG_NAME%.0-win64-%BUILD_TYPE%.exe

	
call %~dp0\build-vs2019.bat %TAG_NAME%.0 package %BUILD_TYPE% %FACTORY_TYPE% || exit /b 2
echo "build %errorlevel%"
echo "%EXE_NAME%"
echo SIGN_PACKAGE_PATH=%JOB_NAME%> var.prop
echo SIGN_PACKAGE_NAME=%EXE_NAME%>> var.prop
mkdir %JOB_NAME%
scp -r %JOB_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build || exit /b 2
cd build
scp %EXE_NAME% cxsw@172.16.33.10:/vagrant_data/www/shared/build/%JOB_NAME%/ || exit /b 2
	
git branch release-%TAG_NAME%
git checkout release-%TAG_NAME%
git push --follow-tags origin HEAD:release-%TAG_NAME%
git push origin HEAD:master
cnpm run yuque %APP_NAME% %TAG_NAME% 

