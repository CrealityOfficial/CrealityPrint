__cc_find(trimesh2)
__assert_target(trimesh2)
__cc_find(boost_header)
__assert_target(boost_header)
__cc_find(expat)
__assert_target(expat)

set(SRCS common_3mf.cpp
		 readwrite.h
		 readwrite.cpp
		 miniz.h
		 miniz.c
		 miniz_extension.h
		 miniz_extension.cpp
		include/common_3mf.h
		)
set(ILIBS trimesh2 expat)
set(LIBS boost_header)

set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set(COMMON_3MF_STATIC 1)
__add_common_library(common_3mf)

if(CC_INSTALL_COMMON_3MF)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/include/ common_3mf)
endif()