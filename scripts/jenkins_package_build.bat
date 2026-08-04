@echo off
set ROOT_C3D=%CD%

echo ROOT=%ROOT_C3D%
set build_type=Release
set SIGNTOOL_CMD=
if [%1] == [] (
	echo "build Usage:"
	echo "Only build: build.bat 0.1.0.1"
    echo "%5 is custom type, such as 'test'"
    echo "Build and package:./scripts/jenkins_package_build.bat 0.1.0.1 local_package CrealityPrint Alpha CrealityPrint"
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

if [%5] == [] (
	set CUSTOM_TYPE=""
) else (
  set CUSTOM_TYPE=%5
)

setlocal enabledelayedexpansion
if [%LOCAL_BUILD%]==[OFF] (
    echo TAG_NAME=%TAG_NAME%
    set TAGNUMB=
    for /f %%i in ('git rev-list HEAD --count') do set TAGNUMB=%%i
    if not defined TAGNUMB (
        echo ERROR: Failed to resolve Git commit count
        exit /b 1
    )
    echo TAGNUMB=!TAGNUMB!
    set TAG_NAME=%TAG_NAME%.!TAGNUMB!
)
setlocal disabledelayedexpansion

SET BUILD_DEPLIB=%ROOT_C3D%\dep_Release
SET ENV_DEPS=%DEPS_DLL_PATH_RELEASE%
SET USE_WORKSPACE_DEPS=1
echo ENV_DEPS=%ENV_DEPS%
if [%ENV_DEPS%] == [] (
    echo "ENV_DEPS is empty"
) else (
    SET BUILD_DEPLIB=%ENV_DEPS%
    SET USE_WORKSPACE_DEPS=0
)
SET REBUILD_DEPS_REQUESTED=0
if /I "%REBUILD_DEPS%"=="true" SET REBUILD_DEPS_REQUESTED=1
if /I "%REBUILD_DEPS%"=="1" SET REBUILD_DEPS_REQUESTED=1
if /I "%REBUILD_DEPS%"=="yes" SET REBUILD_DEPS_REQUESTED=1
if /I "%REBUILD_DEPS%"=="on" SET REBUILD_DEPS_REQUESTED=1
echo REBUILD_DEPS=%REBUILD_DEPS_REQUESTED%

for /f "delims=" %%i in ('where signtool.exe 2^>nul') do (
    set SIGNTOOL_CMD=%%i
    goto AfterFindSignTool
)

for /f "delims=" %%i in ('dir /b /ad /o-n "C:\Program Files (x86)\Windows Kits\10\bin" 2^>nul') do (
    if exist "C:\Program Files (x86)\Windows Kits\10\bin\%%i\x64\signtool.exe" (
        set SIGNTOOL_CMD=C:\Program Files (x86)\Windows Kits\10\bin\%%i\x64\signtool.exe
        goto AfterFindSignTool
    )
)

for /f "delims=" %%i in ('dir /b /ad /o-n "C:\Program Files\Windows Kits\10\bin" 2^>nul') do (
    if exist "C:\Program Files\Windows Kits\10\bin\%%i\x64\signtool.exe" (
        set SIGNTOOL_CMD=C:\Program Files\Windows Kits\10\bin\%%i\x64\signtool.exe
        goto AfterFindSignTool
    )
)

:AfterFindSignTool
if "%USE_WORKSPACE_DEPS%"=="0" (
    if "%REBUILD_DEPS_REQUESTED%"=="1" (
        echo "REBUILD_DEPS is ignored because DEPS_DLL_PATH_RELEASE is set: %BUILD_DEPLIB%"
    )
    if exist "%BUILD_DEPLIB%" (
        goto C3DGenerate
    )
    echo "External deps directory does not exist: %BUILD_DEPLIB%"
    exit /b 1
)

if "%REBUILD_DEPS_REQUESTED%"=="1" (
    echo "REBUILD_DEPS is enabled, rebuild workspace deps."
    SET BUILD_DEPS_ARG=-r
    goto DepBuild
)

if exist "%BUILD_DEPLIB%" (
    echo "Use workspace deps: %BUILD_DEPLIB%"
    goto C3DGenerate
)
echo "Workspace deps directory does not exist, build deps: %BUILD_DEPLIB%"

:DepBuild
echo "Current ACTION = DepBuild"
echo "build dep release"
call "%ROOT_C3D%\build_deps.bat" Release %BUILD_DEPS_ARG% || exit /b 1

:C3DGenerate
cd /d "%ROOT_C3D%"
echo "C3DGenerate..."
echo call run_gettext.bat
if [%5] == [] (
    call "%ROOT_C3D%\run_gettext.bat" || exit /b 1
) else (
    echo customum gettext
    call "%ROOT_C3D%\customized\%CUSTOM_TYPE%\copy_resources.bat" || exit /b 1
    call "%ROOT_C3D%\customized\%CUSTOM_TYPE%\run_gettext.bat" || exit /b 1
)

SET C3D_BUILD_DIR=%ROOT_C3D%\build_Release

@REM build orca
echo "%ROOT_C3D%"
cd %ROOT_C3D%
mkdir %C3D_BUILD_DIR%
cd %C3D_BUILD_DIR%
echo cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint"  -DCMAKE_BUILD_TYPE=Release -DPROCESS_NAME=%APPNAME% -DCREALITYPRINT_VERSION=%TAG_NAME% -DCUSTOM_TYPE=%CUSTOM_TYPE%

@REM cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint" -DCMAKE_BUILD_TYPE=%build_type%
cmake .. -G "%VS_Version%" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DUPDATE_ONLINE_MACHINES=1 ^
-DCMAKE_PREFIX_PATH="%BUILD_DEPLIB%\usr\local" ^
-DCMAKE_INSTALL_PREFIX=".\%APPNAME%"  ^
-DCMAKE_BUILD_TYPE=Release ^
-DPROCESS_NAME=%APPNAME% ^
-DCREALITYPRINT_VERSION=%TAG_NAME% ^
-DPROJECT_VERSION_EXTRA=%VERSION_EXTRA% ^
-DCUSTOM_TYPE=%CUSTOM_TYPE%

cd ..
cd %C3D_BUILD_DIR%
cmake --build . --config %build_type% --target ALL_BUILD -- -m

for /f "tokens=1-3 delims=/ " %%1 in ("%date%") do set currentdate=%%1%%2%%3
echo currentdate=%currentdate%
@REM set JOB_NAME=CrealityPrint_Release_Package
echo JOB_NAME=%JOB_NAME%
set zipName=CrealityPrint_%TAG_NAME%_%currentdate%.zip
set EXE_NAME=%APPNAME%_%TAG_NAME%_%VERSION_EXTRA%.exe
set CrealityPrint_EXE=%C3D_BUILD_DIR%\src\%build_type%\%APPNAME%.exe
echo CrealityPrint_EXE=%CrealityPrint_EXE%
if [%INSTALL_TYPE%]==[nsis] (
    echo package....
    @REM Sign CrealityPrint.exe before packaging NSIS installer
    if exist "%CrealityPrint_EXE%" (
        echo Signing CrealityPrint.exe: %CrealityPrint_EXE%
        "C:\curl.exe" -X POST -F file=@%CrealityPrint_EXE% http://172.20.180.14:3001/sign
        "C:\curl.exe" -L http://172.20.180.14:3001/exe/%APPNAME%.exe -o %ROOT_C3D%\CrealityPrint_signed.exe
        copy /Y %ROOT_C3D%\CrealityPrint_signed.exe %CrealityPrint_EXE%
        del /Q %ROOT_C3D%\CrealityPrint_signed.exe
        echo CrealityPrint.exe signed successfully.
    ) else (
        echo WARN: CrealityPrint.exe not found at %CrealityPrint_EXE%
    )
    cmake --build . --target package --config %build_type%
    if [%LOCAL_BUILD%]==[OFF] (
        echo EXE_NAME=%EXE_NAME%
        "C:\curl.exe" -X POST -F file=@%EXE_NAME% http://172.20.180.14:3001/sign
        "C:\curl.exe" -L http://172.20.180.14:3001/exe/%EXE_NAME% -O
        if [%SIGNTOOL_CMD%] == [] (
            echo signtool.exe not found, skip signature verification
        ) else (
            call "%SIGNTOOL_CMD%" verify /pa /q %EXE_NAME% || exit /b 1
        )
        echo SIGN_PACKAGE_PATH=%APPNAME%> %ROOT_C3D%\var.prop
        echo SIGN_PACKAGE_NAME=%EXE_NAME%>> %ROOT_C3D%\var.prop
        mkdir %JOB_NAME%
        scp -P 9122 -r %JOB_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build
        scp -P 9122 %EXE_NAME% cxsw@172.20.180.14:/vagrant_data/www/shared/build/%JOB_NAME%/%EXE_NAME%
    )
    cmake --build . --target install --config %build_type%
    echo zipname=%zipName%
    %ROOT_C3D%\tools\7z.exe a -tzip %zipName% %C3D_BUILD_DIR%\%APPNAME% -xr!MicrosoftEdgeWebView2RuntimeInstallerX64.exe
    echo zipfinished : %zipName%
    if [%LOCAL_BUILD%]==[OFF] (
        scp -P 9122 %zipName% cxsw@172.20.180.14:/vagrant_data/www/shared/build/%JOB_NAME%/%zipName%
    )
) else if [%INSTALL_TYPE%]==[zip] (
    cmake --build . --target install --config %build_type%   
    echo zipname=%zipName%
    %ROOT_C3D%\tools\7z.exe a -tzip %zipName% %C3D_BUILD_DIR%\%APPNAME%
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
