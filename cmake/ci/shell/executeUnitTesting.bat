@echo off

if [%1] == [] (
	echo "specify execute directory :"
	exit /b -1
)

echo "start executeUnitTesting ...."
echo "working directory : %1"

rem dir /a-d/b/s "%1"\*.exe
for /f "delims=" %%a in ('dir /a-d/b/s "%1"\*.exe') do %%a

echo "end executeUnitTesting"

if %errorlevel% == -1 (
	exit /b -1
)