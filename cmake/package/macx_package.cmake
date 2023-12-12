SET(CPACK_GENERATOR "DragNDrop")
set(CPACK_SYSTEM_NAME "macx")

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version" FORCE)
set(OSX_CODESIGN_IDENTITY "Developer ID Application" CACHE STRING "Identity to use for code signing")
message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET -> 10.12")
