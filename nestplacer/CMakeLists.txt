set(CMAKE_CXX_STANDARD 14)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/nestplacer/ SRCS)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/impl/ IMPLS)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/svg/ SVGS)

list(APPEND SRCS ${IMPLS} ${SVGS})
__cc_find(trimesh2)
__assert_target(trimesh2)
__cc_find(msbase)

__cc_find(tbb)
__cc_find(clipper3r)
__cc_find(nlopt_static)
__cc_find(boost_header)

__assert_target(clipper3r)
__assert_target(nlopt_static)
__assert_target(boost_header)

set(LIBS clipper3r nlopt_static boost_header msbase)
set(ILIBS trimesh2)
set(DEFS)

include(libnest2d/libnest2d.cmake)

if(CC_BC_WIN)
else()
    find_package (Threads)
    list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

__add_common_library(nestplacer OPENMP)

if(CC_INSTALL_NESTPLACER)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/nestplacer nestplacer)
endif()
