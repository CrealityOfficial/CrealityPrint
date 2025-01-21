@echo off
@REM Only use to build depslib from deps src
set ROOT=%CD%
set build_type=%1
set cache_dep_continue=OFF
IF EXIST "%ROOT%\tools\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`%ROOT%\tools\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do (
    set LastVerion=%%i
    )
)
if "%LastVerion%"=="2019" (
    set VS_Version=Visual Studio 16 2019
) else (
    set VS_Version=Visual Studio 17 2022
)
echo LastVerion= %LastVerion%
if [%1] == [] (
    goto help
)
if [%1] == [Debug] (
    set build_type=Debug
    set Build_ON=ON
) else (
    set build_type=Release
    set Build_ON=OFF
)

set DEL_ZIP_CACHE=ON
SET ROOT_DEP=%ROOT%\deps
SET DEP_BUILD_DIR=%ROOT_DEP%\build_%build_type%
SET DEP_INSTALL_DIR=%ROOT%\dep_%build_type%
if(%build_type% == Debug) (
    echo "Debug build"
) else (
    SET ENV_DEPS=%DEPS_DLL_PATH_RELEASE%
    echo ENV_DEPS=%ENV_DEPS%
    if [%ENV_DEPS%] == [] (
        echo "ENV_DEPS is empty"
    ) else (
        SET DEP_INSTALL_DIRATH=%ENV_DEPS%
    )
)


if [%2] == [-c] (
    set cache_dep_continue=ON
    goto GenerateDep
) else (
    set cache_dep_continue=OFF
)


:::deps before
echo %DEP_INSTALL_DIRATH%
if exist "%DEP_INSTALL_DIR%" (
	rem need not download third deplib
	echo "%DEP_INSTALL_DIR% is exist,skip build."
    goto End
) else (
    goto downDepsSrcZip
) 

:downDepsSrcZip
set "url=http://172.20.180.14/soft/DepsLibBuild.zip"
set "zipfile=%ROOT%\DepsLibBuild.zip"
set "unzipdir=%ROOT%/"
REM 1. download zip file
echo downloading... %zipfile%
powershell -Command "(New-Object Net.WebClient).DownloadFile('%url%', '%zipfile%')"
REM 2. check download result
echo %zipfile%
if not exist "%zipfile%" (
    echo download failed！
    exit /b 1
)

REM 3. mkdir destdir
mkdir "%unzipdir%"

REM 4. unzip
echo unziping...
powershell -Command "Expand-Archive -LiteralPath '%zipfile%' -DestinationPath '%unzipdir%' -Force"

REM 5. check unzip result
if errorlevel 1 (
    echo unzip failed.
    exit /b 1
)
del "%zipfile%"
echo unzip finished


:GenerateDep_Before
echo GenerateDep

@REM Remove dep build dir if exist

IF EXIST %DEP_BUILD_DIR% (
    echo RD /S /Q %DEP_BUILD_DIR%
    RD /S /Q %DEP_BUILD_DIR%
) ELSE (
    echo HINT: %DEP_BUILD_DIR% NOT EXIST, skip remove
)

@REM Remove dep install dir if exist
IF EXIST %DEP_INSTALL_DIR% (
    echo RD /S /Q %DEP_INSTALL_DIR%
    RD /S /Q %DEP_INSTALL_DIR%
) ELSE (
    echo HINT: %DEP_INSTALL_DIR% NOT EXIST, skip remove
)

:GenerateDep
@REM build dep
cd %ROOT_DEP%
echo my=%ROOT_DEP%
mkdir build_%build_type%
cd build_%build_type%
echo cmake ../ -G "%VS_Version%" -A x64 -DDESTDIR="%DEP_INSTALL_DIR%" -DCMAKE_BUILD_TYPE=%build_type% -DDEP_DEBUG=%Build_ON% -DORCA_INCLUDE_DEBUG_INFO=OFF
cmake ../ -G "%VS_Version%" -A x64 -DDESTDIR="%DEP_INSTALL_DIR%" -DCMAKE_BUILD_TYPE=%build_type% -DDEP_DEBUG=%Build_ON% -DORCA_INCLUDE_DEBUG_INFO=OFF
if %cache_dep_continue% == ON (
    echo cmake --build . --config %build_type% --target deps -- -m
    cmake --build . --config %build_type% --target deps -- -m
    goto End
)

@REM Extract DL_CACHE.zip
cd %ROOT_DEP%
IF EXIST DL_CACHE\ (
    echo RD /S /Q DL_CACHE
    RD /S /Q DL_CACHE
) ELSE (
    echo HINT: DL_CACHE\ NOT EXIST, Skip remove
)
echo 7z x %ROOT%\DL_CACHE.zip
%ROOT%\tools\7z.exe x %ROOT%\DL_CACHE.zip

@REM REM Deal with wxWidgets
set WXWIDGETS_PREFIX_ROOT=%DEP_BUILD_DIR%\dep_wxWidgets-prefix
cd %WXWIDGETS_PREFIX_ROOT%\src
IF EXIST dep_wxWidgets\ (
    echo RD /S /Q dep_wxWidgets\
    RD /S /Q dep_wxWidgets\
)
echo 7z x %ROOT%\dep_wxWidgets.zip
%ROOT%\tools\7z.exe x %ROOT%\dep_wxWidgets.zip

echo python %ROOT%\wxWidgets_scrip.py %WXWIDGETS_PREFIX_ROOT%
python %ROOT%\wxWidgets_scrip.py %WXWIDGETS_PREFIX_ROOT%

:GenerateDep_After
echo GenerateDep_After

:del_unzip_cache
echo del_unzip_cache
if %DEL_ZIP_CACHE% == ON (
    echo myroot=%ROOT%
    del "%ROOT%\build.bat"
    del "%ROOT%\DL_CACHE.zip"
    del "%ROOT%\wxWidgets_scrip.py"
    del "%ROOT%\dep_wxWidgets.zip"
)

:BuildDep
echo BuildDep Start
cd %DEP_BUILD_DIR%
echo cmake --build . --config %build_type% --target deps -- -m
cmake --build . --config %build_type% --target deps -- -m

:BuildDep_After
echo BuildDep_After


goto End
:help
echo -------------------------------help-----------------------
echo build.bat Debug
echo build.bat Release
echo build.bat Debug zipcache
echo build.bat Release zipcache
echo ----------------------------------------------------------

:End
echo build_deps end