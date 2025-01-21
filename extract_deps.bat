@echo off
if [%1] == [] (
	set PWD=%CD%
) else (
	set PWD=%1
)
REM 设置下载URL和目标文件路径
set "url=http://172.20.180.14/soft/OrcaDepsLib.zip"
set "zipfile=%PWD%\OrcaDepsLib.zip"
set "unzipdir=%PWD%"
if [%2] == [Debug] (
	set "url=http://172.20.180.14/soft/OrcaDepsLib_Debug.zip"
	set "zipfile=%PWD%\OrcaDepsLib_Debug.zip"
)
REM 1. download zip file
echo downloading... %zipfile%
powershell -Command "(New-Object Net.WebClient).DownloadFile('%url%', '%zipfile%')"
REM 2. check download result
echo %zipfile%
if not exist "%zipfile%" (
    echo download failed！
    exit /b 1
)

REM 3. mkdir destdir
mkdir "%unzipdir%"

REM 4. unzip
echo unziping...
powershell -Command "Expand-Archive -LiteralPath '%zipfile%' -DestinationPath '%unzipdir%' -Force"

REM 5. check unzip result
if errorlevel 1 (
    echo unzip failed！
    exit /b 1
)
del "%zipfile%"
echo done.
