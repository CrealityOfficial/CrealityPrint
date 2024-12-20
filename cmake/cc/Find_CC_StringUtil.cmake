# This sets the following variables:
# stringutil target

if(NOT TARGET stringutil)
	__search_target_components(stringutil
							INC stringutil/util.h
							DLIB stringutil
							LIB stringutil
							PRE stringutil
							)
	
	__test_import(stringutil dll)
endif()