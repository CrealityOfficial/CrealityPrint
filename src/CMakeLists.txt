cmake_minimum_required(VERSION 3.13)
project(CrealityPrint-native)

include("../version.inc")
add_subdirectory(build-utils)
add_subdirectory(admesh)
# add_subdirectory(avrdude)
# boost/nowide
add_subdirectory(boost)
add_subdirectory(clipper)
add_subdirectory(clipper2)
add_subdirectory(miniz)
add_subdirectory(minilzo)
add_subdirectory(glu-libtess)
add_subdirectory(qhull)
add_subdirectory(Shiny)
add_subdirectory(semver)
add_subdirectory(libigl)
add_subdirectory(hints)
add_subdirectory(mcut)
add_subdirectory(qoi)

# Adding libnest2d project for bin packing...
add_subdirectory(libnest2d)

add_subdirectory(libslic3r)

if (SLIC3R_GUI)
    add_subdirectory(imgui)
    add_subdirectory(imguizmo)
    add_subdirectory(hidapi)
    include_directories(hidapi/include)

    if(WIN32)
        message(STATUS "WXWIN environment set to: $ENV{WXWIN}")
    elseif(UNIX)
        set(wxWidgets_USE_UNICODE ON)
        if(SLIC3R_STATIC)
            set(wxWidgets_USE_STATIC ON)
        else()
            set(wxWidgets_USE_STATIC OFF)
        endif()
    endif()

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set (wxWidgets_CONFIG_OPTIONS "--toolkit=gtk${SLIC3R_GTK}")
        if (SLIC3R_WX_STABLE)
            find_package(wxWidgets 3.0 REQUIRED COMPONENTS base core adv html gl aui net media webview)
        else ()
            find_package(wxWidgets 3.1 REQUIRED COMPONENTS base core adv html gl aui net media webview)
        endif ()
    else ()
        find_package(wxWidgets 3.1 REQUIRED COMPONENTS html adv gl core base webview aui net media)
    endif ()

    if(UNIX)
        message(STATUS "wx-config path: ${wxWidgets_CONFIG_EXECUTABLE}")
    endif()

    include(${wxWidgets_USE_FILE})

    find_package(JPEG QUIET)
    find_package(TIFF QUIET)

    string(REGEX MATCH "wxpng" WX_PNG_BUILTIN ${wxWidgets_LIBRARIES})
    if (PNG_FOUND AND NOT WX_PNG_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX png)
        list(APPEND wxWidgets_LIBRARIES ${PNG_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxtiff" WX_TIFF_BUILTIN ${wxWidgets_LIBRARIES})
    if (TIFF_FOUND AND NOT WX_TIFF_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX tiff)
        list(APPEND wxWidgets_LIBRARIES ${TIFF_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxjpeg" WX_JPEG_BUILTIN ${wxWidgets_LIBRARIES})
    if (JPEG_FOUND AND NOT WX_JPEG_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX jpeg)
        list(APPEND wxWidgets_LIBRARIES ${JPEG_LIBRARIES})
    endif ()

    string(REGEX MATCH "wxexpat" WX_EXPAT_BUILTIN ${wxWidgets_LIBRARIES})
    if (EXPAT_FOUND AND NOT WX_EXPAT_BUILTIN)
        list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX expat)
        list(APPEND wxWidgets_LIBRARIES ${EXPAT_LIBRARIES})
    endif ()
    
    # This is an issue in the new wxWidgets cmake build, doesn't deal with librt
    find_library(LIBRT rt)
    if(LIBRT)
        list(APPEND wxWidgets_LIBRARIES ${LIBRT})
    endif()
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        find_library(LIBOSS alibabacloud-oss-cpp-sdkd)
    else()
        find_library(LIBOSS alibabacloud-oss-cpp-sdk)
    endif()
    if(LIBOSS)
        list(APPEND wxWidgets_LIBRARIES ${LIBOSS} OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    # This fixes a OpenGL linking issue on OSX. wxWidgets cmake build includes
    # wrong libs for opengl in the link line and it does not link to it by himself.
    # libslic3r_gui will link to opengl anyway, so lets override wx
    list(FILTER wxWidgets_LIBRARIES EXCLUDE REGEX OpenGL)

#    list(REMOVE_ITEM wxWidgets_LIBRARIES oleacc)
    message(STATUS "wx libs: ${wxWidgets_LIBRARIES}")

    add_subdirectory(slic3r)
endif()

if(ORCA_TOOLS)
    # CrealityPrint_profile_validator
    add_executable(CrealityPrint_profile_validator CrealityPrint_profile_validator.cpp)
    target_link_libraries(CrealityPrint_profile_validator libslic3r boost_headeronly libcurl OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(CrealityPrint_profile_validator PRIVATE -DBOOST_ALL_NO_LIB -DBOOST_USE_WINAPI_VERSION=0x602 -DBOOST_SYSTEM_USE_UTF8)
    if(WIN32)
        target_link_libraries(CrealityPrint_profile_validator bcrypt.lib)
    endif()
endif()

# Create a slic3r executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/msw/CrealityPrint.rc.in ${CMAKE_CURRENT_BINARY_DIR}/CrealityPrint.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/msw/CrealityPrint.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/CrealityPrint.manifest @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/osx/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
set(PROJECT_DLL CrealityPrint_Slicer)
if(WIN32)
set(PROJECT_DLL CrealityPrint_Slicer)
elseif(APPLE)
set(PROJECT_DLL CrealityPrint)
endif()
if (WIN32)
    add_library(${PROJECT_DLL} SHARED CrealityPrint.cpp CrealityPrint.hpp BaseException.cpp BaseException.h StackWalker.cpp StackWalker.h CxMiniDump.h CxMiniDump.cpp)
else ()
    add_executable(${PROJECT_DLL} CrealityPrint.cpp CrealityPrint.hpp)
endif ()

if (MINGW)
    target_link_options(${PROJECT_DLL} PUBLIC "-Wl,-allow-multiple-definition")
    set_target_properties(${PROJECT_DLL} PROPERTIES PREFIX "")
endif (MINGW)

if (NOT WIN32 AND NOT APPLE)
    # Binary name on unix like systems (Linux, Unix)
    set_target_properties(${PROJECT_DLL} PROPERTIES OUTPUT_NAME "${PROCESS_NAME}")
    set(SLIC3R_APP_CMD "${PROCESS_NAME}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/platform/unix/BuildLinuxImage.sh.in ${CMAKE_CURRENT_BINARY_DIR}/BuildLinuxImage.sh @ONLY)
endif ()

target_link_libraries(${PROJECT_DLL} libslic3r cereal::cereal)
if (APPLE)
#    add_compile_options(-stdlib=libc++)
#    add_definitions(-DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE)
    # -liconv: boost links to libiconv by default
    target_link_libraries(${PROJECT_DLL} "-liconv -framework IOKit" "-framework CoreFoundation" "-framework AVFoundation" "-framework AVKit" "-framework CoreMedia" "-framework VideoToolbox" -lc++)
elseif (MSVC)
    # Manifest is provided through CrealityPrint.rc, don't generate your own.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
else ()
    # Boost on Raspberry-Pi does not link to pthreads explicitely.
    target_link_libraries(${PROJECT_DLL} ${CMAKE_DL_LIBS} -lstdc++ Threads::Threads pangoft2-1.0)
endif ()

# Add the Slic3r GUI library, libcurl, OpenGL and GLU libraries.
if (SLIC3R_GUI)
#    target_link_libraries(CrealityPrint ws2_32 uxtheme setupapi libslic3r_gui ${wxWidgets_LIBRARIES})
target_link_libraries(${PROJECT_DLL} libslic3r_gui)
    if (MSVC)
        # Generate debug symbols even in release mode.
        target_link_options(${PROJECT_DLL} PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
        target_link_libraries(${PROJECT_DLL} user32.lib Setupapi.lib)
    elseif (MINGW)
        target_link_libraries(${PROJECT_DLL} ws2_32 uxtheme setupapi)
    elseif (APPLE)
        target_link_libraries(${PROJECT_DLL} "-framework OpenGL")
    else ()
        target_link_libraries(${PROJECT_DLL} -ldl)
    endif ()
    #if (WIN32)
    #    find_library(PSAPI_LIB NAMES Psapi)
    #    target_link_libraries(CrealityPrint ${PSAPI_LIB})
    #endif ()
endif ()

# On Windows, a shim application is required to produce a console / non console version of the Slic3r application.
# Also the shim may load the Mesa software OpenGL renderer if the default renderer does not support OpenGL 2.0 and higher.
if (WIN32)
    if (MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    endif()

    add_executable(${STARTUP_PROJECT_NAME} WIN32 CrealityPrint_app_msvc.cpp ${CMAKE_CURRENT_BINARY_DIR}/CrealityPrint.rc)
    # Generate debug symbols even in release mode.
    if(MSVC)
        target_link_options(${STARTUP_PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:/DEBUG>")
    endif()
    target_compile_definitions(${STARTUP_PROJECT_NAME} PRIVATE -DSLIC3R_WRAPPER_NOCONSOLE)
    add_dependencies(${STARTUP_PROJECT_NAME} ${PROJECT_DLL})
    set_target_properties(${STARTUP_PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROCESS_NAME}")
#    target_link_libraries(${STARTUP_PROJECT_NAME} PRIVATE boost_headeronly)
endif ()

# Link the resources dir to where Slic3r GUI expects it
set(output_dlls_Release "")
set(output_dlls_Debug "")
set(output_dlls_RelWithDebInfo "")

if (WIN32)
    # This has to be a separate target due to the windows command line lenght limits
    add_custom_target(COPY_DLLS ALL DEPENDS ${PROJECT_DLL})

    if (CMAKE_CONFIGURATION_TYPES)
        foreach (CONF ${CMAKE_CONFIGURATION_TYPES})
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}" WIN_CONF_OUTPUT_DIR)
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}/resources" WIN_RESOURCES_SYMLINK)
            message(STATUS "WIN_RESOURCES_SYMLINK=${WIN_RESOURCES_SYMLINK}")
            message(STATUS "SLIC3R_RESOURCES_DIR_WIN=${SLIC3R_RESOURCES_DIR_WIN}")
            add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
                COMMAND if exist "${WIN_CONF_OUTPUT_DIR}" "("
                        if not exist "${WIN_RESOURCES_SYMLINK}" "("
                            mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_DIR_WIN}"
                        ")"
                    ")"
                COMMENT "Symlinking the resources directory into the build tree"
                VERBATIM
            )
        endforeach ()

        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            crealityprint_copy_dlls(COPY_DLLS "Debug" "d" output_dlls_Debug)
        elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
            crealityprint_copy_dlls(COPY_DLLS "RelWithDebInfo" "" output_dlls_Release)
        else()
            crealityprint_copy_dlls(COPY_DLLS "Release" "" output_dlls_Release)
        endif()
    else ()
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/resources" WIN_RESOURCES_SYMLINK)
        message(STATUS "SLIC3R_RESOURCES_DIR_WIN222=${SLIC3R_RESOURCES_DIR_WIN}")
        add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
            COMMAND if not exist "${WIN_RESOURCES_SYMLINK}" "(" mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_DIR_WIN}" ")"
            COMMENT "Symlinking the resources directory into the build tree"
            VERBATIM
        )
    endif ()


else ()
    if (APPLE AND NOT CMAKE_MACOSX_BUNDLE)
        # On OSX, the name of the binary matches the name of the Application.
        add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
            COMMAND ln -sf ${PROJECT_DLL} CrealityPrint-slicer
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrealityPrint>"
            VERBATIM)
    else ()
        add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_DLL}>"
            VERBATIM)
    endif ()
    if (XCODE)
        # Because of Debug/Release/etc. configurations (similar to MSVC) the slic3r binary is located in an extra level
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")
        set(BIN_CONF_DIR "Debug")
    else ()
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/../resources")
    endif ()
    if (CMAKE_MACOSX_BUNDLE)
        if (CMAKE_CONFIGURATION_TYPES)
            set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/CrealityPrint.app/Contents/Resources")
        else()
            set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/CrealityPrint.app/Contents/Resources")
        endif()
        set(MACOSX_BUNDLE_ICON_FILE Icon.icns)
        set(MACOSX_BUNDLE_NAME "CrealityPrint")
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${CREALITYPRINT_VERSION})
        set(MACOSX_BUNDLE_COPYRIGHT "Copyright(C) 2022-2024 Creality All Rights Reserved")
        
    endif()

    message("SLIC3R_RESOURCES_DIR_Mac=${SLIC3R_RESOURCES_DIR}")
    message("BIN_RESOURCES_DIR=${BIN_RESOURCES_DIR}")
    if(APPLE)
        add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_DLL}>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${SLIC3R_RESOURCES_DIR}" "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_DLL}>/Resources"
            COMMENT "Copying resources to .app bundle"
            )
    else()
        add_custom_command(TARGET ${PROJECT_DLL} POST_BUILD
        COMMAND ln -sfn "${SLIC3R_RESOURCES_DIR}" "${BIN_RESOURCES_DIR}"
        COMMENT "Symlinking the resources directory into the build tree"
        VERBATIM)
    endif()
endif ()

# Slic3r binary install target. Default build type is release in case no CMAKE_BUILD_TYPE is provided.
if( ("${CMAKE_BUILD_TYPE}" STREQUAL "Release") OR ("${CMAKE_BUILD_TYPE}" STREQUAL "") )
    set (build_type "Release")
    set(CMAKE_BUILD_POSTFIX "")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set (build_type "RelWithDebInfo")
    set(CMAKE_BUILD_POSTFIX "")
else()
    set (build_type "Debug")
    set(CMAKE_BUILD_POSTFIX "d")
endif()
message(STATUS "libslic3r-CMAKE_BUILD_TYPE: ${build_type}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
if (WIN32)
    install(TARGETS ${PROJECT_DLL} RUNTIME DESTINATION ".")
    if (MSVC)
        install(TARGETS ${STARTUP_PROJECT_NAME} RUNTIME DESTINATION ".")
    endif ()
    install(FILES ${output_dlls_${build_type}} DESTINATION ".")
    install(FILES ${CMAKE_SOURCE_DIR}/package/runtime/MicrosoftEdgeWebView2RuntimeInstallerX64.exe DESTINATION ".")
    install(FILES ${CMAKE_SOURCE_DIR}/package/runtime/vcredist_x64.exe DESTINATION ".")
else ()
    set(CMAKE_INSTALL_BINDIR "./")
    message(STATUS "CMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}")
    install(TARGETS ${PROJECT_DLL} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()
