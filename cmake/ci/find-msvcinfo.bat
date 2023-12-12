@echo off

set VSMajorVerNum=
set VSMinorVerNum=
set VSVerName=
set VSDriver=
set VSDir=Program Files (x86)\Microsoft Visual Studio
set VSIncludeDir=

for /f "Skip=1" %%i IN ('Wmic Logicaldisk Where "DriveType=3" Get Name') DO (
	if EXIST "%%i\%VSDir%\" (
		for /f %%j IN ('dir "%%i\%VSDir%\" /b /ad-h /on') DO (
			for /f %%k IN ('dir "%%i\%VSDir%\%%j\" /b /ad-h /on') DO (
				if  EXIST "%%i\%VSDir%\%%j\%%k\VC\Auxiliary\Build\vcvars64.bat" (
					set VSDriver=%%i
					set VSMajorVerNum=%%j
					set VSVerName=%%k
					
					goto foundVSDir
				)
			)
		)
	)
)

echo Failed to find VS Dir
exit /b 1

:foundVSDir

set VSDir=%VSDriver%\%VSDir%\%VSMajorVerNum%\%VSVerName%
echo VSDir: %VSDir%

exit /b 0
