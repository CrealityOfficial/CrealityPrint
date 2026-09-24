find_package(OpenGL QUIET REQUIRED)

set(_tiff_bundled_codec_args "")
if (NOT WIN32)
    set(_tiff_bundled_codec_args
        -DZLIB_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DZLIB_LIBRARY:FILEPATH=${DESTDIR}/lib/libz.a
        -DZLIB_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libz.a
        -DJPEG_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DJPEG_LIBRARY:FILEPATH=${DESTDIR}/lib/libjpeg.a
        -DJPEG_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libjpeg.a
        -Dlibdeflate:BOOL=OFF
    )
endif()

if (APPLE)
    message(STATUS "Compiling TIFF for macos ${CMAKE_SYSTEM_VERSION}.")
    orcaslicer_add_cmake_project(TIFF
        URL https://gitlab.com/libtiff/libtiff/-/archive/v4.3.0/libtiff-v4.3.0.zip
        URL_HASH SHA256=455abecf8fba9754b80f8eff01c3ef5b24a3872ffce58337a59cba38029f0eca
        DEPENDS ${ZLIB_PKG} ${PNG_PKG} dep_JPEG
        CMAKE_ARGS
            ${_tiff_bundled_codec_args}
            -Dlzma:BOOL=OFF
            -Dwebp:BOOL=OFF
            -Djbig:BOOL=OFF
            -Dzstd:BOOL=OFF
            -Dpixarlog:BOOL=OFF
    )
else()
    orcaslicer_add_cmake_project(TIFF
        # URL https://gitlab.com/libtiff/libtiff/-/archive/v4.1.0/libtiff-v4.1.0.zip
        # URL_HASH SHA256=c56edfacef0a60c0de3e6489194fcb2f24c03dbb550a8a7de5938642d045bd32
        URL https://gitlab.com/libtiff/libtiff/-/archive/v4.3.0/libtiff-v4.3.0.zip
        URL_HASH SHA256=455abecf8fba9754b80f8eff01c3ef5b24a3872ffce58337a59cba38029f0eca
        DEPENDS ${ZLIB_PKG} ${PNG_PKG} dep_JPEG
        CMAKE_ARGS
            ${_tiff_bundled_codec_args}
            -Dlzma:BOOL=OFF
            -Dwebp:BOOL=OFF
            -Djbig:BOOL=OFF
            -Dzstd:BOOL=OFF
            -Dpixarlog:BOOL=OFF
    )

endif()

if (MSVC)
    add_debug_dep(dep_TIFF)
endif()
