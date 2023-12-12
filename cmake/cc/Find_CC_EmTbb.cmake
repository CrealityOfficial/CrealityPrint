# This sets the following variables:
# tbb
message(STATUS "TBB Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(tbb_INCLUDE_DIRS ${THIRD0_INSTALL_ROOT}/include/)
	set(tbb_LIBRARIES_DEBUG ${THIRD0_INSTALL_ROOT}/lib/Debug/libtbb_debug.a)
	set(tbb_LIBRARIES_RELEASE ${THIRD0_INSTALL_ROOT}/lib/Release/libtbb.a)
else()
endif()

__search_target_components(tbb
						   INC tbb/tbb.h
						   DLIB tbb
						   LIB tbb
						   PRE tbb
						   )
						   
add_definitions(-D__TBB_NO_IMPLICIT_LINKAGE)
__test_import(tbb lib)