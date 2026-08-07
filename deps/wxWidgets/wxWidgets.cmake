set(_wx_toolkit "")
set(_wx_debug_postfix "")
set(_wx_shared -DwxBUILD_SHARED=OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_gtk_ver 2)

    if (DEP_WX_GTK3)
        set(_gtk_ver 3)
    endif ()

    set(_wx_toolkit "-DwxBUILD_TOOLKIT=gtk${_gtk_ver}")
    if (FLATPAK)
        set(_wx_debug_postfix "d")
        set(_wx_shared -DwxBUILD_SHARED=ON -DBUILD_SHARED_LIBS:BOOL=ON)
    endif ()
endif()

if (MSVC)
    set(_wx_edge "-DwxUSE_WEBVIEW_EDGE=ON")
    set(_wx_patch_command ${CMAKE_COMMAND}
        -Dwx_SOURCE_DIR=<SOURCE_DIR>
        -Dwx_PATCH_FILE=${CMAKE_CURRENT_LIST_DIR}/0001-define-compiler-tls-in-cmake-setup.patch
        -P ${CMAKE_CURRENT_LIST_DIR}/apply-compiler-tls-patch.cmake)
    set(CREALITYPRINT_MSGFMT_EXECUTABLE "" CACHE FILEPATH
        "Exact msgfmt executable used by wxWidgets")
    set(CREALITYPRINT_MSGMERGE_EXECUTABLE "" CACHE FILEPATH
        "Exact msgmerge executable used by wxWidgets")
    if (NOT "${CREALITYPRINT_MSGFMT_EXECUTABLE}" STREQUAL "" OR
        NOT "${CREALITYPRINT_MSGMERGE_EXECUTABLE}" STREQUAL "")
        if ("${CREALITYPRINT_MSGFMT_EXECUTABLE}" STREQUAL "" OR
            "${CREALITYPRINT_MSGMERGE_EXECUTABLE}" STREQUAL "")
            message(FATAL_ERROR "Both CrealityPrint gettext executable paths must be supplied together")
        endif ()
        get_filename_component(_wx_msgfmt "${CREALITYPRINT_MSGFMT_EXECUTABLE}" REALPATH)
        get_filename_component(_wx_msgmerge "${CREALITYPRINT_MSGMERGE_EXECUTABLE}" REALPATH)
        if (NOT EXISTS "${_wx_msgfmt}" OR NOT EXISTS "${_wx_msgmerge}")
            message(FATAL_ERROR "The configured CrealityPrint gettext executable path does not exist")
        endif ()
    else ()
        unset(_wx_msgfmt CACHE)
        unset(_wx_msgmerge CACHE)
        find_program(_wx_msgfmt msgfmt.exe PATHS
            "C:/Program Files/Poedit/GettextTools/bin"
            "C:/Program Files/Git/usr/bin"
            NO_DEFAULT_PATH)
        find_program(_wx_msgmerge msgmerge.exe PATHS
            "C:/Program Files/Poedit/GettextTools/bin"
            "C:/Program Files/Git/usr/bin"
            NO_DEFAULT_PATH)
    endif ()
    if (_wx_msgfmt AND _wx_msgmerge)
        set(_wx_gettext_args
            -DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${_wx_msgfmt}
            -DGETTEXT_MSGMERGE_EXECUTABLE:FILEPATH=${_wx_msgmerge})
    endif ()
else ()
    set(_wx_edge "-DwxUSE_WEBVIEW_EDGE=OFF")
    set(_wx_patch_command "")
    set(_wx_gettext_args "")
endif ()

orcaslicer_add_cmake_project(
    wxWidgets
    # The wxWidgets fork uses git submodules for PCRE and other bundled
    # libraries.  GitHub source archives contain only empty gitlink
    # directories, so use the immutable superproject commit and let Git check
    # out the exact submodule commits recorded by it.
    GIT_REPOSITORY "https://github.com/SoftFever/Orca-deps-wxWidgets"
    GIT_TAG db1005db3dea2c37a46fb455a9a02e37aa360751
    GIT_SUBMODULES
        3rdparty/libwebp
        3rdparty/lunasvg
        3rdparty/pcre
    GIT_SUBMODULES_RECURSE ON
    # Scintilla contains paths longer than the legacy Windows Git limit.  Set
    # this only for the ExternalProject clone; do not require global Git state.
    GIT_CONFIG core.longpaths=true
    PATCH_COMMAND ${_wx_patch_command}
    DEPENDS ${PNG_PKG} ${ZLIB_PKG} ${EXPAT_PKG} ${JPEG_PKG}
    CMAKE_ARGS
        -DwxBUILD_PRECOMP=ON
        ${_wx_toolkit}
        "-DCMAKE_DEBUG_POSTFIX:STRING=${_wx_debug_postfix}"
        -DwxBUILD_DEBUG_LEVEL=0
        -DwxBUILD_SAMPLES=OFF
        -DwxBUILD_TESTS=OFF
        ${_wx_shared}
        -DwxUSE_MEDIACTRL=ON
        -DwxUSE_DETECT_SM=OFF
        -DwxUSE_PRIVATE_FONTS=ON
        -DwxUSE_OPENGL=ON
        -DwxUSE_GLCANVAS_EGL=ON
        -DwxUSE_WEBREQUEST=ON
        -DwxUSE_WEBVIEW=ON
        ${_wx_edge}
        -DwxUSE_WEBVIEW_IE=OFF
        -DwxUSE_REGEX=builtin
        ${_wx_gettext_args}
        -DwxUSE_LIBSDL=OFF
        -DwxUSE_XTEST=OFF
        -DwxUSE_STC=OFF
        -DwxUSE_AUI=ON
        -DwxUSE_LIBPNG=sys
        -DwxUSE_ZLIB=sys
        -DwxUSE_LIBJPEG=sys
        -DwxUSE_LIBTIFF=OFF
        -DwxUSE_LIBWEBP=builtin
        -DwxUSE_EXPAT=sys
        -DwxUSE_NANOSVG=OFF
)

# wxWidgets 3.3 cmake install doesn't include private headers.
# OrcaSlicer uses some of the private headers (for accessibility support).
# Copy the private headers directory after install.
if(MSVC)
    set(_wx_inc_dest ${DESTDIR}/include/wx)
else()
    set(_wx_inc_dest ${DESTDIR}/include/wx-3.3/wx)
endif()
ExternalProject_Add_Step(dep_wxWidgets copy_private_headers
    DEPENDEES install
    COMMENT "Copying wxWidgets private headers"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include/wx/private
        ${_wx_inc_dest}/private
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include/wx/generic/private
        ${_wx_inc_dest}/generic/private
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include/wx/gtk/private
        ${_wx_inc_dest}/gtk/private
)

if (MSVC)
    add_debug_dep(dep_wxWidgets)
endif ()
