@REM CrealityPrint/C3DSlicer Windows ARM64 build script for Visual Studio 2022.
@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

set "WP=%~dp0"
if "%WP:~-1%"=="\" set "WP=%WP:~0,-1%"
set "_START_TIME=%TIME%"

set "VS_GENERATOR=Visual Studio 17 2022"
set "VS_PLATFORM=ARM64"
set "VSROOT=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "RUN_DEPS=1"
set "RUN_SLICER=1"
set "debug=OFF"
set "debuginfo=OFF"

for %%a in (%*) do (
    if /I "%%a"=="deps" (
        set "RUN_DEPS=1"
        set "RUN_SLICER=0"
    )
    if /I "%%a"=="slicer" (
        set "RUN_DEPS=0"
        set "RUN_SLICER=1"
    )
    if /I "%%a"=="debug" set "debug=ON"
    if /I "%%a"=="debuginfo" set "debuginfo=ON"
)

if "%debug%"=="ON" (
    set "build_type=Debug"
    set "build_dir=build-arm64-dbg"
) else (
    if "%debuginfo%"=="ON" (
        set "build_type=RelWithDebInfo"
        set "build_dir=build-arm64-dbginfo"
    ) else (
        set "build_type=Release"
        set "build_dir=build-arm64"
    )
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake not found in PATH.
    exit /b 1
)

if /I not "%VSCMD_ARG_TGT_ARCH%"=="arm64" (
    set "VSINSTALLDIR="
    set "VCINSTALLDIR="
    set "VCToolsInstallDir="
    set "VSCMD_ARG_TGT_ARCH="
    set "VSCMD_ARG_HOST_ARCH="
    set "VCVARS=!VSROOT!\VC\Auxiliary\Build\vcvarsall.bat"
    if not exist "!VCVARS!" (
        set "VSROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
        set "VCVARS=!VSROOT!\VC\Auxiliary\Build\vcvarsall.bat"
    )
    if not exist "!VCVARS!" (
        set "VSROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise"
        set "VCVARS=!VSROOT!\VC\Auxiliary\Build\vcvarsall.bat"
    )
    if not exist "!VCVARS!" (
        echo Error: VS 2022 vcvarsall.bat not found.
        exit /b 1
    )
    call "!VCVARS!" x64_arm64
    if errorlevel 1 echo Warning: vcvarsall returned an error; checking compiler tools directly.
)

set "ARM64_CL="
if defined VCToolsInstallDir if exist "!VCToolsInstallDir!bin\Hostx64\arm64\cl.exe" set "ARM64_CL=!VCToolsInstallDir!bin\Hostx64\arm64\cl.exe"
if not defined ARM64_CL (
    for /f "delims=" %%v in ('dir /b /ad /o-n "!VSROOT!\VC\Tools\MSVC" 2^>nul') do (
        if not defined ARM64_CL if exist "!VSROOT!\VC\Tools\MSVC\%%v\bin\Hostx64\arm64\cl.exe" set "ARM64_CL=!VSROOT!\VC\Tools\MSVC\%%v\bin\Hostx64\arm64\cl.exe"
    )
)
if not defined ARM64_CL (
    echo Error: ARM64 compiler cl.exe not found.
    echo Install "MSVC v143 - VS 2022 C++ ARM64 build tools" first.
    exit /b 1
)

where msbuild >nul 2>nul
if errorlevel 1 (
    echo Error: msbuild not found in PATH.
    exit /b 1
)

set "DEPS=%WP%\deps\%build_dir%\OrcaSlicer_dep"

echo Build settings:
echo   Generator: %VS_GENERATOR%
echo   Platform:  %VS_PLATFORM%
echo   Type:      %build_type%
echo   Dir:       %build_dir%
echo.

if "%RUN_DEPS%"=="1" (
    if not exist "%WP%\deps\CR_TPMS\cr_tpms\lib\winARM64\cr_tpms_library.lib" (
        echo Error: missing Windows ARM64 CR_TPMS library:
        echo   "%WP%\deps\CR_TPMS\cr_tpms\lib\winARM64\cr_tpms_library.lib"
        echo Build cannot link TPMS without cr_tpms_library.lib/.dll.
        exit /b 1
    )
    powershell -ExecutionPolicy Bypass -File "%WP%\scripts\prepare_win_arm64_binary_deps.ps1" || exit /b 1

    echo building ARM64 deps...
    if not exist "%WP%\deps\%build_dir%" mkdir "%WP%\deps\%build_dir%"
    cd /d "%WP%\deps\%build_dir%" || exit /b 1

    echo on
    cmake .. -G "%VS_GENERATOR%" -A %VS_PLATFORM% -DDESTDIR="%DEPS%" -DCMAKE_BUILD_TYPE=%build_type% -DDEP_DEBUG=%debug% -DORCA_INCLUDE_DEBUG_INFO=%debuginfo% -DENABLE_BREAKPAD=OFF
    if errorlevel 1 exit /b 1
    cmake --build . --config %build_type% --target deps -- /m
    if errorlevel 1 exit /b 1
    @echo off
)

if "%RUN_SLICER%"=="1" (
    if not exist "%DEPS%\usr\local" (
        echo Error: deps prefix not found: "%DEPS%\usr\local"
        echo Run build_release_vs2022_arm64.bat deps first.
        exit /b 1
    )

    echo building CrealityPrint ARM64...
    if not exist "%WP%\%build_dir%" mkdir "%WP%\%build_dir%"
    cd /d "%WP%\%build_dir%" || exit /b 1

    echo on
    cmake .. -G "%VS_GENERATOR%" -A %VS_PLATFORM% -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%\usr\local" -DCMAKE_INSTALL_PREFIX="./CrealityPrint" -DCMAKE_BUILD_TYPE=%build_type% -DWIN10SDK_PATH="C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0" -DENABLE_BREAKPAD=OFF
    if errorlevel 1 exit /b 1
    cmake --build . --config %build_type% --target ALL_BUILD -- /m
    if errorlevel 1 exit /b 1
    @echo off

    cd /d "%WP%" || exit /b 1
    call run_gettext.bat
    if errorlevel 1 exit /b 1

    cd /d "%WP%\%build_dir%" || exit /b 1
    cmake --build . --target install --config %build_type%
    if errorlevel 1 exit /b 1
)

for /f "tokens=1-3 delims=:.," %%a in ("%_START_TIME: =0%") do set /a "_start_s=%%a*3600+%%b*60+%%c"
for /f "tokens=1-3 delims=:.," %%a in ("%TIME: =0%") do set /a "_end_s=%%a*3600+%%b*60+%%c"
set /a "_elapsed=_end_s - _start_s"
if %_elapsed% lss 0 set /a "_elapsed+=86400"
set /a "_hours=_elapsed / 3600"
set /a "_remainder=_elapsed - _hours * 3600"
set /a "_mins=_remainder / 60"
set /a "_secs=_remainder - _mins * 60"
echo.
echo Build completed in %_hours%h %_mins%m %_secs%s
