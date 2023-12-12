set APP_ID=%1
set APP_NAME=%2
set VERSION=%3
set EXE_NAME=%4
set BUILD_TYPE=%5	
set EXE_PATH=%6
set EXT=%7
set OS=%8
set PATH=%9
cnpm run upload %APP_ID% %APP_NAME% %BUILD_TYPE% %EXT% %EXE_NAME% %EXE_PATH% %VERSION% %OS%

