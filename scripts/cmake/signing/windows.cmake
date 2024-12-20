# Signing the Installer and Cura.exe with "signtool.exe"
# Location on my desktop:
# -> C:\Program Files (x86)\Windows Kits\10\bin\x86\signtool.exe
# * Manual: https://msdn.microsoft.com/de-de/library/8s9b9yaz(v=vs.110).aspx

#find_package(Signtool REQUIRED)
set(WINDOWS_IDENTITIY_PFX_THUMBPRINT $ENV{PFX_THUMBPRINT})
set(WINDOWS_IDENTITIY_PFX_PASSWORD $ENV{PFX_PASSWORD})

set(signtool_OPTIONS /dig sha256 /tp  ${WINDOWS_IDENTITIY_PFX_THUMBPRINT} /p ${WINDOWS_IDENTITIY_PFX_PASSWORD} /hide /c /tr http://timestamp.digicert.com /file)

if(${WINDOWS_IDENTITIY_PFX_PASSWORD})
    message(WARNING "USE WITH CAUTION: Password for the PFX file has been set!")
endif()

# Signing Creative3D.exe and dumptool.exe
add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND C:/Windows/curl.exe http://127.0.0.1:3000/sign?name=${BIN_OUTPUT_DIR}/Release/${PROJECT_NAME}.exe
    COMMAND C:/Windows/curl.exe http://127.0.0.1:3000/sign?name=${BIN_OUTPUT_DIR}/Release/dumptool.exe
    ## Other optional options:
    # /tr timestampServerUrl 
    WORKING_DIRECTORY ${BIN_OUTPUT_DIR}/Release
)

# Signing the installer
add_custom_target(signing-installer) # Sadly "TARGET package POST_BUILD" can't be used in the following add_custom_command()
set(C3D_INSTALLER_NAME ${SIGN_CPACK_PACKAGE_FILE_NAME}.exe)

add_custom_command(
    TARGET signing-installer PRE_BUILD
    COMMAND ${SIGNTOOL_EXECUTABLE} sign ${signtool_OPTIONS} ${C3D_INSTALLER_NAME}
    ## Other optional options:
    # /tr timestampServerUrl 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
