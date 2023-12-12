# This sets the following variables:
# cxnd target

if(NOT TARGET cxnd)
	__search_target_components(cxnd
							INC cxnd/algrithm/a3d.h
							DLIB cxnd
							LIB cxnd
							PRE cxnd
							)
	
	__test_import(cxnd dll)
endif()