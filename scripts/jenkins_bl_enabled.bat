@echo off

set TestDir=%~dp0\..\UnitTest
if not exist %TestDir% (
    mkdir %TestDir%
    cd %TestDir%\..\
    git clone "http://172.20.180.12:8050/UnitTest"
)
cd %TestDir%
git checkout version-0.0.2
git pull
@echo end
