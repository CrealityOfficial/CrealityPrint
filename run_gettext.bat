@echo off

REM CrealityPrint gettext
REM Created by SoftFever on 27/5/23.

REM Check for --full argument
set FULL_MODE=0
for %%a in (%*) do (
    if "%%a"=="--full" set FULL_MODE=1
)
SET "src_dir=%cd%\src"
set "filter_dir=%cd%\"
SET TEMP_FILE=%cd%\localization\i18n\list.txt
if %FULL_MODE%==1 (
    echo %cd%/src
    
    echo "src_dir: %src_dir%"
    @REM Create a temporary file to store the list of files
    
    if exist %TEMP_FILE% del %TEMP_FILE%

    :: 遍历 src 文件夹及其子文件夹，并查找.cpp 和.hpp 文件，截取src/目录后的相对路径写入list.txt
    for /r "%src_dir%" %%A in (*.cpp *.hpp) do (
        set "fullPath=%%~fA"
        call :GetRelativePath "%%~fA"
    )
    
    echo "Get the list of files to .pot"
    .\tools\xgettext.exe --keyword=L --keyword=_L --keyword=_u8L --keyword=L_CONTEXT:1,2c --keyword=_L_PLURAL:1,2 --add-comments=TRN --from-code=UTF-8 --no-location --debug --boost -f %TEMP_FILE% -o ./localization/i18n/CrealityPrint.pot
    build\\src\\hints\\Release\\hintsToPot ./resources ./localization/i18n
)
REM Print the current directory
echo %cd%
set pot_file="./localization/i18n/CrealityPrint.pot"

REM Run the script for each .po file
echo "Run the script for each .po file"
for /r "./localization/i18n/" %%f in (CrealityPrint*.po) do (
    call :processFile "%%f" "CrealityPrint"
)
set pot_file="./localization/i18n/Community.pot"
echo "Run the script for each .po file"
for /r "./localization/i18n/" %%f in (Community*.po) do (
    call :processFile "%%f" "Community" 1
)
set pot_file="./localization/i18n/DeviceList.pot"
echo "Run the script for each .po file"
for /r "./localization/i18n/" %%f in (DeviceList*.po) do (
    call :processFile "%%f" "DeviceList" 1
)
set pot_file="./localization/i18n/SendPage.pot"
echo "Run the script for each .po file"
for /r "./localization/i18n/" %%f in (SendPage*.po) do (
    call :processFile "%%f" "SendPage" 1
)
goto :eof


:processFile
    echo "prcessFile: %~1"
    set "file=%~1"
    set "dir=%~dp1"
    set "name=%~n1"
    set "lang=%name:*_=%"
    set "part=%~2"
    set web=%~3
    if %FULL_MODE%==1 (
        .\tools\msgmerge.exe -N -o "%file%" "%file%" "%pot_file%"
    )
    if not exist "./resources/i18n/%lang%" mkdir "./resources/i18n/%lang%"
    
    if "%~3"=="1" (
        po2json "%file%" "./resources/i18n/%lang%/%part%.json"
        echo "prcessFile: ./resources/i18n/%lang%/%part%.json"
    )else (
        .\tools\msgfmt.exe --check-format -o "./resources/i18n/%lang%/%part%.mo" "%file%"
        echo "prcessFile: ./resources/i18n/%lang%/%part%.mo"
    )
goto :eof

:GetRelativePath
    echo "Real File_Path: %~1"
    set "inputPath=%~1"
    setlocal enabledelayedexpansion
    set "relativePath=!inputPath:%filter_dir%=!"
    if "!relativePath:~0,1!"=="\" (
        set "relativePath=!relativePath:~1!"
    )
    set "relativePath=!relativePath:\=/!"
    echo !relativePath!>> "%TEMP_FILE%"
    endlocal
    exit /b