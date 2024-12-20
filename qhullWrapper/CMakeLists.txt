__CC_find(Trimesh2)

__assert_target(trimesh2)
if(CC_BC_LINUX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
set(SRCS)
set(LIBS trimesh2)
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/include/)

include(qhull.cmake)
include(wrapper.cmake)

__add_platform_library(qhullWrapper)
					
if(CC_INSTALL_QHULLWRAPPER)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/include/ qhullWrapper)
endif()
