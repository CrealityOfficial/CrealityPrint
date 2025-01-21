set build_dir=build
set build_type=Debug
set DEPS=%CD%/deps/build-dbg/OrcaSlicer_dep


@REM example: VS_PATH=D:\Microsoft Visual Studio\2022\Enterprise in system PATH
set VSENV="%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if not "%VS_PATH%" == "" (
  if exist %VSENV% (
    call %VSENV%
    goto :build
  ) else (echo "not find vs in" %VSENV%)
) else (echo "not find vs in" %VSENV%)

set VSENV="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
  call %VSENV%
  goto :build
) else (echo "not find vs in" %VSENV%)

set VSENV="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (
  call %VSENV%
  goto :build
) else (echo "not find vs in" %VSENV%)

set VSENV="D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VSENV% (call %VSENV% ) else (exit /B)

:build
mkdir %build_dir%
cd %build_dir%
echo cmake .. -G "Ninja" -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%/usr/local" -DCMAKE_INSTALL_PREFIX="./OrcaSlicer" -DCMAKE_BUILD_TYPE=%build_type%
cmake .. -G "Ninja"  -DBBL_RELEASE_TO_PUBLIC=1 -DCMAKE_PREFIX_PATH="%DEPS%/usr/local" -DCMAKE_INSTALL_PREFIX="./OrcaSlicer" -DCMAKE_BUILD_TYPE=%build_type% -DWIN10SDK_PATH="C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0"
rem cmake --build . --config %build_type% --target libslic3r_gui