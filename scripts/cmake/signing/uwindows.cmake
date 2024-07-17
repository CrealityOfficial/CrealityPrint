add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND C:/curl.exe -X POST -F file=@${PROJECT_NAME}.exe http://172.20.180.14:3001/sign
    COMMAND C:/curl.exe -L http://172.20.180.14:3001/exe/${PROJECT_NAME}.exe -O
    COMMAND C:/curl.exe -X POST -F file=@dumptool.exe http://172.20.180.14:3001/sign
    COMMAND C:/curl.exe -L http://172.20.180.14:3001/exe/dumptool.exe -O
    ## Other optional options:
    # /tr timestampServerUrl 
    WORKING_DIRECTORY ${BIN_OUTPUT_DIR}/Release
)