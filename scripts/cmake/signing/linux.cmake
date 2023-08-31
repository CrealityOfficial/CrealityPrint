find_package(GPG REQUIRED)
find_package(SHA1Sum REQUIRED)

set(PACKAGE_FILE "Ultimaker_Cura-${CURA_VERSION}.AppImage")

add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND ${GPG_EXECUTABLE} --yes --armor --detach-sig ${PACKAGE_FILE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating signature..."
)

add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND ${SHA1SUM_EXECUTABLE} ${PACKAGE_FILE} > ${PACKAGE_FILE}.sha1.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating SHA-1 sum..."
)
