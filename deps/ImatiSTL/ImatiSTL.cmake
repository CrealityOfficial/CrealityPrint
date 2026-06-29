if (MSVC)
    message(STATUS "Skipping ImatiSTL dependency build on Windows")
    return()
endif()

set(patch_command git init && ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/0001-add-CMakeLists.patch)

set(_imatistl_library "${DESTDIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ImatiSTL${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(_imatistl_include_stamp "${DESTDIR}/include/ImatiSTL/imatistl.h")

ExternalProject_Add(dep_ImatiSTL
    EXCLUDE_FROM_ALL ON
    INSTALL_DIR ${DESTDIR}
    DOWNLOAD_DIR ${DEP_DOWNLOAD_DIR}/ImatiSTL
    URL https://downloads.sourceforge.net/project/imatistl/ImatiSTL-4.2-4.zip
    URL_HASH SHA256=B337C04A3BBD0A88A10EEC0E2E6729FBC0E09EE6B50679CA03049A21F1506710
    PATCH_COMMAND ${patch_command}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:STRING=${DESTDIR}
        -DCMAKE_PREFIX_PATH:STRING=${DESTDIR}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
        -DCMAKE_TOOLCHAIN_FILE:STRING=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DBUILD_SHARED_LIBS:BOOL=OFF
        ${_cmake_osx_arch}
        ${DEP_CMAKE_OPTS}
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release -- -j${NPROC}
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E make_directory ${DESTDIR}/include/ImatiSTL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DESTDIR}/include/TMesh
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DESTDIR}/include/Kernel
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DESTDIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include/ImatiSTL ${DESTDIR}/include/ImatiSTL
        COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include/TMesh ${DESTDIR}/include/TMesh
        COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include/Kernel ${DESTDIR}/include/Kernel
        COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ImatiSTL${CMAKE_STATIC_LIBRARY_SUFFIX} ${_imatistl_library}
    BUILD_BYPRODUCTS
        ${_imatistl_library}
        ${_imatistl_include_stamp}
)

add_dependencies(dep_ImatiSTL dep_CGAL)
