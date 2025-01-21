@echo off
set WP=%CD%

set debug=OFF
set debuginfo=OFF
set compile=OFF
if "%1"=="debug" set debug=ON
if "%2"=="debug" set debug=ON
if "%1"=="debuginfo" set debuginfo=ON
if "%2"=="debuginfo" set debuginfo=ON
if "%1"=="Debug" set debug=ON
if "%2"=="Debug" set debug=ON
if "%debug%"=="ON" (
    set build_type=Debug
    set build_dir=build_Debug
) else (
    if "%debuginfo%"=="ON" (
        set build_type=RelWithDebInfo
        set build_dir=build-dbginfo
    ) else (
        set build_type=Release
        set build_dir=build_Release
    )
)
if "%1"=="compile" set compile=ON
if "%2"=="compile" set compile=ON
if "%1"=="--help" (
	goto Help
)
echo build type set to %build_type%

:slicer
echo "building Orca Slicer11..."
set DEPS=%WP%\thirdDeps\dep_%build_type%
set OLDDEPS=%WP%\dep_%build_type%
if exist "%OLDDEPS%" (
	rem need not download third deplib
	echo "building Orca Slicer33..."
	set DEPS=%OLDDEPS%
)
echo "building Orca Slicer22..."
if not exist "%DEPS%" (
	if "%build_type%" == "Debug" (
		echo "unzip before"
		call extract_deps.bat %WP% Debug || exit /b 1
	) else (
		echo "building Orca Slicer55..."
		call extract_deps.bat %WP%  || exit /b 1
	)
)
mkdir %build_dir%
cd %build_dir%

echo "building Orca Slicer..."
cd %WP%
mkdir %build_dir%
cd %build_dir%

echo cmake .. -G "Visual Studio 16 2019" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint" -DCMAKE_BUILD_TYPE=%build_type%
cmake .. -G "Visual Studio 16 2019" -A x64 -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%\usr\local" -DCMAKE_INSTALL_PREFIX=".\CrealityPrint" -DCMAKE_BUILD_TYPE=%build_type% -DWIN10SDK_PATH="%WindowsSdkDir%Include\%WindowsSDKLibVersion%"

if "%compile%"=="ON" (
rem cmake --build . --config %build_type% --target ALL_BUILD -- -m
rem cd ..
rem call run_gettext.bat
cd %build_dir%
cmake --build . --target install --config %build_type%
)
exit /b 0

:Help
echo -------------------------Help-------------------------
echo Cmd Example should be below:
echo orca_build_release.bat	 
echo orca_build_release.bat compile
echo orca_build_release.bat debug
echo orca_build_release.bat compile debug
goto End