@echo off

call "%~dp0\find-msvcinfo.bat"
if not %errorlevel%==0 (exit /b 1)

if not defined DevEnvDir (	
	echo "call vcvars64.bat"
	call "%VSDir%\VC\Auxiliary\Build\vcvars64.bat"
)

REM 从注册表查询 win10 SDK 安装目录，主要用于判断注册表查询是否被禁，如果被禁 vcvars64.bat 将无法正常配置环境，需要手动配置环境依赖
call "%~dp0\check-vcvars64.bat"

:build
echo "build"

cd %~dp0

cmake -S ../../ -B ../../win32-build/build -DCC_BUILD_TEST=ON
cd ../../