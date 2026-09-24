if (MSVC)
    set(_use_IPP "-DWITH_IPP=ON")
    set(_build_zlib "-DBUILD_ZLIB=ON")
else ()
    set(_use_IPP "-DWITH_IPP=OFF")
    set(_build_zlib "-DBUILD_ZLIB=OFF")
endif ()

if (IN_GIT_REPO)
    set(OpenCV_DIRECTORY_FLAG --directory ${BINARY_DIR_REL}/dep_OpenCV-prefix/src/dep_OpenCV)
endif ()

set(_opencv_bundled_image_args "")
set(_opencv_image_deps "")
set(_opencv_build_jpeg ON)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Keep OpenCV's static image codecs on the dependency prefix. Otherwise its
    # exported target records host library paths and adds runtime dependencies.
    # Share JPEG with wxWidgets: OpenCV's bundled ABI 62 can otherwise override
    # the ABI 80 symbols used by wxWidgets in the statically linked application.
    set(_opencv_build_jpeg OFF)
    set(_opencv_bundled_image_args
        -DCMAKE_PREFIX_PATH:STRING=${DESTDIR}
        -DJPEG_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DJPEG_LIBRARY:FILEPATH=${DESTDIR}/lib/libjpeg.a
        -DJPEG_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libjpeg.a
        -DWITH_JPEG=ON
        -DPNG_PNG_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DPNG_LIBRARY:FILEPATH=${DESTDIR}/lib/libpng16.a
        -DPNG_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libpng16.a
        -DZLIB_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DZLIB_LIBRARY:FILEPATH=${DESTDIR}/lib/libz.a
        -DZLIB_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libz.a
        -DTIFF_INCLUDE_DIR:PATH=${DESTDIR}/include
        -DTIFF_LIBRARY:FILEPATH=${DESTDIR}/lib/libtiff.a
        -DTIFF_LIBRARY_RELEASE:FILEPATH=${DESTDIR}/lib/libtiff.a
        -DWITH_PNG=ON
        -DWITH_TIFF=ON
    )
    set(_opencv_image_deps ${TIFF_PKG} ${JPEG_PKG})
endif ()

orcaslicer_add_cmake_project(OpenCV
    URL https://github.com/opencv/opencv/archive/refs/tags/4.6.0.tar.gz
    URL_HASH SHA256=1ec1cba65f9f20fe5a41fda1586e01c70ea0c9a6d7b67c9e13edf0cfe2239277
    DEPENDS ${PNG_PKG} ${ZLIB_PKG} ${_opencv_image_deps}
    PATCH_COMMAND git apply ${OpenCV_DIRECTORY_FLAG} --verbose --ignore-space-change --whitespace=fix ${CMAKE_CURRENT_LIST_DIR}/0001-vs2022.patch  ${CMAKE_CURRENT_LIST_DIR}/0002-clang19-macos.patch
    CMAKE_ARGS
       ${_opencv_bundled_image_args}
       -DBUILD_SHARED_LIBS=0
       -DBUILD_PERE_TESTS=OFF
       -DBUILD_TESTS=OFF
       -DBUILD_opencv_python_tests=OFF
       -DBUILD_EXAMPLES=OFF
       -DBUILD_JASPER=OFF
       -DBUILD_JAVA=OFF
       -DBUILD_JPEG=${_opencv_build_jpeg}
       -DBUILD_APPS_LIST=version
       -DBUILD_opencv_apps=OFF
       -DBUILD_opencv_java=OFF
       -DBUILD_OPENEXR=OFF
       -DBUILD_PNG=OFF
       -DBUILD_TBB=OFF
       -DBUILD_WEBP=OFF
       ${_build_zlib}
       -DWITH_1394=OFF
       -DWITH_CUDA=OFF
       -DWITH_EIGEN=OFF
       ${_use_IPP}
       -DWITH_ITT=OFF
       -DWITH_FFMPEG=OFF
       -DWITH_GPHOTO2=OFF
       -DWITH_GSTREAMER=OFF
       -DOPENCV_GAPI_GSTREAMER=OFF
       -DWITH_GTK_2_X=OFF
       -DWITH_JASPER=OFF
       -DWITH_LAPACK=OFF
       -DWITH_MATLAB=OFF
       -DWITH_MFX=OFF
       -DWITH_DIRECTX=OFF
       -DWITH_DIRECTML=OFF
       -DWITH_OPENCL=OFF
       -DWITH_OPENCL_D3D11_NV=OFF
       -DWITH_OPENCLAMDBLAS=OFF
       -DWITH_OPENCLAMDFFT=OFF
       -DWITH_OPENEXR=OFF
       -DWITH_OPENJPEG=OFF
       -DWITH_QUIRC=OFF
       -DWITH_VTK=OFF
       -DWITH_WEBP=OFF
       -DENABLE_PRECOMPILED_HEADERS=OFF
       -DINSTALL_TESTS=OFF
       -DINSTALL_C_EXAMPLES=OFF
       -DINSTALL_PYTHON_EXAMPLES=OFF
       -DOPENCV_GENERATE_SETUPVARS=OFF
       -DOPENCV_INSTALL_FFMPEG_DOWNLOAD_SCRIPT=OFF
       -DBUILD_opencv_python2=OFF
       -DBUILD_opencv_python3=OFF
       -DWITH_OPENVINO=OFF
       -DWITH_INF_ENGINE=OFF
       -DWITH_NGRAPH=OFF
       -DBUILD_WITH_STATIC_CRT=OFF#set /MDd /MD
       -DBUILD_LIST=core,imgcodecs,imgproc,world
       -DBUILD_opencv_highgui=OFF
       -DWITH_ADE=OFF
       -DBUILD_opencv_world=ON
       -DWITH_PROTOBUF=OFF
       -DWITH_WIN32UI=OFF
       -DHAVE_WIN32UI=FALSE
)

if (MSVC)
    add_debug_dep(dep_OpenCV)
endif()
