set(OSX_CODESIGN_IDENTITY "Mac Developer" CACHE STRING "Identity to use for code signing")

add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/fix_codesign_issues.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

add_custom_command(
    TARGET signing PRE_BUILD
    COMMAND codesign -s ${OSX_CODESIGN_IDENTITY} --deep "Ultimaker Cura.app"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

install(DIRECTORY "${CMAKE_BINARY_DIR}/build/Ultimaker Cura.app" DESTINATION "." USE_SOURCE_PERMISSIONS)

set(CPACK_PACKAGE_NAME "Ultimaker_Cura")
set(CPACK_PACKAGE_VENDOR "Ultimaker")
set(CPACK_PACKAGE_VERSION_MAJOR ${CURA_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${CURA_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${CURA_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${CURA_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cura 3D Printing Software")
set(CPACK_PACKAGE_CONTACT "Arjen Hiemstra <a.hiemstra@ultimaker.com>")

set(CPACK_GENERATOR "DragNDrop")

include(CPack)
