__cc_find(TriMesh2)
__cc_find(Eigen)
__cc_find(Clipper)

__assert_target(trimesh2)
__assert_target(eigen)
__assert_target(clipper)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/topomesh/ SRCS)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/internal/ Inters)
		 
list(APPEND SRCS ${Inters})
set(ILIBS trimesh2)
set(LIBS clipper eigen)

__assert_target(msbase)
list(APPEND LIBS msbase)

if(CC_BC_WIN)
else()
    find_package (Threads)
    list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

set(INTERFACE_DEFS USE_TOPOMESH)
set(DEFS DLP_USE_UM)

set(TOPOMESH_STATIC 1)
__add_common_library(topomesh OPENMP)

if(CC_INSTALL_TOPOMESH)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/topomesh topomesh)
endif()