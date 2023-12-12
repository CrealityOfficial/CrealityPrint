@echo off

set WinSDKVer=
set WinSDKDir=%ProgramFiles(x86)%\Windows Kits\10
set __check_file=winsdkver.h
if EXIST "%WinSDKDir%\Include\" (
	for /f %%i IN ('dir "%WinSDKDir%\Include\" /b /ad-h /on') DO (
	    @REM Skip if Windows.h|winsdkver (based upon -app_platform configuration) is not found in %%i\um.  
		if EXIST "%WinSDKDir%\Include\%%i\um\%__check_file%" (
			set WinSDKVer=%%i
			goto found
		)
	)
)

echo Failed to find Win SDK Dir
exit /b 1

:found

echo WinSDKVer: %WinSDKVer%
echo WinSDKDir: %WinSDKDir%
exit /b 0