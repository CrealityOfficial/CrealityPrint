@echo off

call "%~dp0\find-msvcinfo.bat"
if not %errorlevel%==0 (exit /b 1)
		
call "%VSDir%\VC\Auxiliary\Build\vcvars64.bat"

REM ��ע����ѯ win10 SDK ��װĿ¼����Ҫ�����ж�ע����ѯ�Ƿ񱻽���������� vcvars64.bat ���޷��������û�������Ҫ�ֶ����û�������
call "%~dp0\check-vcvars64.bat"

:build
echo "build"

cd %~dp0

cd ../../

mkdir ninja-build
cd ninja-build
mkdir build
cd build

echo "Debug"
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    ..\..\ || exit /b
	
ninja || exit /b

echo "Release"	
cmake ^
    -G"Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..\..\ || exit /b

rem Build and install the application

ninja || exit /b

ninja install || exit /b

cd ..\..\