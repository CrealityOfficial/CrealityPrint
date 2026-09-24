@echo off
rem Ninja's UTF-8 dependency prefix must match MSVC /showIncludes output.
rem IDE builds without a console otherwise fall back to the system code page.
chcp 65001 >nul
if errorlevel 1 exit /b 1
%*
exit /b %errorlevel%
