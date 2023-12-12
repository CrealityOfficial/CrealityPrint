@echo off

REM 从注册表查询注册表版本号，主要用于判断注册表查询是否被禁，如果被禁 vcvars64.bat 将无法正常配置环境，需要通过 setEnv 手动配置环境依赖
for /F "tokens=1,2*" %%i in ('REG QUERY HKLM\Software\Microsoft\ResKit /v Version') DO (
    if not "%%i"=="" (
        exit /b 0
    )
)

REM 手动配置环境依赖
:setEnv

set VSIncludeDir=%VSToolsInstallDir%\Include

call "%~dp0\find-winsdkinfo.bat"
if not %errorlevel%==0 (exit /b 1)

if not "%WinSDKVer%"=="" (
	set "PATH=%PATH%;%WinSDKDir%\bin\%WinSDKVer%\x64"
	set "INCLUDE=%INCLUDE%;%WinSDKDir%\Include\%WinSDKVer%\cppwinrt;%WinSDKDir%\Include\%WinSDKVer%\shared;%WinSDKDir%\Include\%WinSDKVer%\um;%WinSDKDir%\Include\%WinSDKVer%\winrt;%WinSDKDir%\Include\%WinSDKVer%\ucrt;%VSIncludeDir%"
	set "LIB=%LIB%;%WinSDKDir%\Lib\%WinSDKVer%\um\x64;%WinSDKDir%\Lib\%WinSDKVer%\ucrt\x64"
	set "LIBPATH=%LIBPATH%;%WinSDKDir%\UnionMetadata;%WinSDKDir%\References"
)