# This sets the following variables:
# tbb

if(NOT TARGET tbb)
	__search_target_components(tbb
							INC tbb/tbb.h
							DLIB tbb
							LIB tbb
							PRE tbb
							)
							
	add_definitions(-D__TBB_NO_IMPLICIT_LINKAGE)
	__test_import(tbb dll)
endif()