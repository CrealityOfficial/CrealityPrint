__enable_cxx17()

__cc_find(trimesh2)

__assert_target(trimesh2)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/msbase/ SRCS)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/internal/ Inters)
		 
list(APPEND SRCS ${Inters})
set(ILIBS trimesh2)

if(CC_BC_WIN)
else()
    find_package (Threads)
    list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

set(MSBASE_STATIC 1)
set(INTERFACE_DEFS USE_MSBASE)
__add_common_library(msbase OPENMP)

if(CC_INSTALL_MSBASE)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/msbase msbase)
endif()