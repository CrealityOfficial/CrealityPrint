@echo off

call "%~dp0\find-msvcinfo.bat"
if not %errorlevel%==0 (exit /b 1)

if not defined DevEnvDir (	
	echo "call vcvars64.bat"
	call "%VSDir%\VC\Auxiliary\Build\vcvars64.bat"
)

REM ��ע����ѯ win10 SDK ��װĿ¼����Ҫ�����ж�ע����ѯ�Ƿ񱻽���������� vcvars64.bat ���޷��������û�������Ҫ�ֶ����û�������
call "%~dp0\check-vcvars64.bat"

:build
echo "build"

cd %~dp0

cmake -S ../../ -B ../../win32-build/build -DCC_BUILD_TEST=ON
cd ../../