@echo off
set ROOT_C3D=%CD%

echo ROOT=%ROOT_C3D%
set build_type=Release
if [%1] == [] (
	echo "build Usage:"
	echo "Only build: build.bat v0.1.0.1"
	echo "Build and package:build.bat v0.1.0.1 package Creality_Print"
	exit /b 0
)

IF EXIST "%ROOT_C3D%\tools\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`%ROOT_C3D%\tools\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do (
    set LastVerion=%%i
    )
)
if "%LastVerion%"=="2019" (
    set VS_Version=Visual Studio 16 2019
) else (
    set VS_Version=Visual Studio 17 2022
    set LastVerion=2022
)
echo LastVerion= %LastVerion%
echo Pre_TAG_NAME=%TAG_NAME%

set TAG_NAME=%1

set LOCAL_BUILD=ON
set INSTALL_TYPE=zip
if [%2] == [package] (
    set LOCAL_BUILD=OFF
    set INSTALL_TYPE=nsis
) else (
    if [%2] == [local_package] (
        set LOCAL_BUILD=ON
        set INSTALL_TYPE=nsis
    ) else (
        IF [%2] == [local_zip] (
            set LOCAL_BUILD=ON
            set INSTALL_TYPE=zip
        ) else (
            if [%2] == [zip] (
                set LOCAL_BUILD=OFF
                set INSTALL_TYPE=zip
            )
        )
    )
)

if [%3] == [] (
	set APPNAME="CrealityPrint"
) else (
  set APPNAME=%3
)
if [%4] == [] (
    set VERSION_EXTRA="Alpha"
) else (
  set VERSION_EXTRA=%4
)

setlocal enabledelayedexpansion
if [%LOCAL_BUILD%]==[OFF] (
    echo TAG_NAME=%TAG_NAME%
    echo git show-ref %TAG_NAME% 
    git show-ref %TAG_NAME% | awk -F ' ' '{print $1}' >cmmid
    cat cmmid
    set /p CMMID=<cmmid 
    echo CMMID=!CMMID!
    git rev-list HEAD | wc -l >maxcmmid
    git rev-list !CMMID! | wc -l >tagcmmid
    set /p MAXCMMID=<maxcmmid
    set /p TAGCMMID=<tagcmmid
    set /a TAGNUMB=!MAXCMMID!-!TAGCMMID!
    echo TAGNUMB=!TAGNUMB!
    set TAG_NAME=%TAG_NAME%.!TAGNUMB!
)
setlocal disabledelayedexpansion

SET BUILD_DEPLIB=%ROOT_C3D%\dep_Release
SET ENV_DEPS=%DEPS_DLL_PATH_RELEASE%
echo ENV_DEPS=%ENV_DEPS%
if [%ENV_DEPS%] == [] (
    echo "ENV_DEPS is empty"
) else (
    SET BUILD_DEPLIB=%ENV_DEPS%
)
if exist "%BUILD_DEPLIB%" (
    goto C3DGenerate
)

:DepBuild
echo "Current ACTION = DepBuild"
echo "build dep release"
call build_deps.bat Release

:C3DGenerate
echo "C3DGenerate..."
SET C3D_BUILD_DIR=%ROOT_C3D%\build_Release
@REM IF EXIST %C3D_BUILD_DIR% (
@REM     echo RD /S /Q %C3D_BUILD_DIR%
@REM     RD /S /Q %C3D_BUILD_DIR%
@REM ) ELSE (
@REM     echo HINT: %C3D_BUILD_DIR% NOT EXIST, skip remove
@REM )

@REM build orca
echo "%ROOT_C3D%"
cd %ROOT_C3D%
mkdir %C3D_BUILD_DIR%
cd %C3D_BUILD_DIR%
echo cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint"  -DCMAKE_BUILD_TYPE=Release -DPROCESS_NAME=%APPNAME% -DCREALITYPRINT_VERSION=%TAG_NAME%

@REM cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint" -DCMAKE_BUILD_TYPE=%build_type%
cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 ^
-DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" ^
-DCMAKE_INSTALL_PREFIX=".\CrealityPrint"  ^
-DCMAKE_BUILD_TYPE=Release ^
-DPROCESS_NAME=%APPNAME% ^
-DCREALITYPRINT_VERSION=%TAG_NAME% ^
-DPROJECT_VERSION_EXTRA=%VERSION_EXTRA%

cd ..
echo call run_gettext.bat
call run_gettext.bat || exit /b 1


cd %C3D_BUILD_DIR%
cmake --build . --config %build_type% --target ALL_BUILD -- -m

for /f "tokens=1-3 delims=/ " %%1 in ("%date%") do set currentdate=%%1%%2%%3
echo currentdate=%currentdate%
set JOB_NAME=CrealityPrint_Release_Package
echo JOB_NAME=%JOB_NAME%
set zipName=CrealityPrint_%TAG_NAME%_%currentdate%.zip
set EXE_NAME=%APPNAME%_%TAG_NAME%_%VERSION_EXTRA%.exe
if [%INSTALL_TYPE%]==[nsis] (
    echo package....
    cmake --build . --target package --config %build_type%
    if [%LOCAL_BUILD%]==[OFF] (
        echo EXE_NAME=%EXE_NAME%
        "C:\curl.exe" -X POST -F file=@%EXE_NAME% http://172.20.180.14:3001/sign
        "C:\curl.exe" -L http://172.20.180.14:3001/exe/%EXE_NAME% -O
        echo SIGN_PACKAGE_PATH=%APPNAME%> %ROOT_C3D%\var.prop
        echo SIGN_PACKAGE_NAME=%EXE_NAME%>> %ROOT_C3D%\var.prop
        mkdir %JOB_NAME%
        scp -P 9122 -r %JOB_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build
        scp -P 9122 %EXE_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build/%JOB_NAME%/%EXE_NAME%
    )
) else if [%INSTALL_TYPE%]==[zip] (
    cmake --build . --target install --config %build_type%   
    echo zipname=%zipName%
    %ROOT_C3D%\tools\7z.exe a -tzip %zipName% %C3D_BUILD_DIR%\CrealityPrint
    echo zipfinished : %zipName%
    set EXE_NAME=%zipName%
    echo SIGN_PACKAGE_PATH=%JOB_NAME%> %ROOT_C3D%\var.prop
    echo SIGN_PACKAGE_NAME=%EXE_NAME%>> %ROOT_C3D%\var.prop
    if [%LOCAL_BUILD%]==[OFF] (
        mkdir %JOB_NAME%
        scp -P 9122 -r %JOB_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build
        scp -P 9122 %EXE_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build/%JOB_NAME%/%EXE_NAME%
    )
)
cd ..
exit /b 0

goto End