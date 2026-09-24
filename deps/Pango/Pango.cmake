# Keep this GTK-compatible runtime private to AppImage packaging, so its
# headers and pkg-config files don't change wxWidgets dependency discovery.
find_program(MESON_EXECUTABLE meson)
if (NOT MESON_EXECUTABLE)
    message(FATAL_ERROR "Building the Linux Pango runtime requires meson (install the meson package).")
endif ()

set(_pango_patch_directory "")
if (IN_GIT_REPO)
    set(_pango_patch_directory --directory ${BINARY_DIR_REL}/dep_Pango-prefix/src/dep_Pango)
endif ()

ExternalProject_Add(dep_Pango
    EXCLUDE_FROM_ALL ON
    URL https://download.gnome.org/sources/pango/1.52/pango-1.52.2.tar.xz
    URL_HASH SHA256=d0076afe01082814b853deec99f9349ece5f2ce83908b8e58ff736b41f78a96b
    DOWNLOAD_DIR ${DEP_DOWNLOAD_DIR}/Pango
    INSTALL_DIR ${DESTDIR}/lib/creality-pango
    PATCH_COMMAND ${GIT_EXECUTABLE} apply ${_pango_patch_directory} --verbose --ignore-space-change
        ${CMAKE_CURRENT_LIST_DIR}/0001-guard-missing-font-family.patch
    CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup <BINARY_DIR> <SOURCE_DIR>
        --prefix=<INSTALL_DIR> --libdir=lib --buildtype=release
        --default-library=shared --wrap-mode=nodownload
        -Dintrospection=disabled -Dgtk_doc=false -Dinstall-tests=false
        -Dsysprof=disabled -Dxft=enabled
    BUILD_COMMAND ${MESON_EXECUTABLE} compile -C <BINARY_DIR> -j ${NPROC}
    INSTALL_COMMAND ${MESON_EXECUTABLE} install -C <BINARY_DIR>
        COMMAND ${CMAKE_COMMAND} -E make_directory <INSTALL_DIR>/share/licenses/pango
        COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/COPYING
            <INSTALL_DIR>/share/licenses/pango/COPYING
)
