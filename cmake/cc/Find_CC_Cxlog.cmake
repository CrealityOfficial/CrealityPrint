# This sets the following variables:
# cxlog target

if(NOT TARGET cxlog) 
	__search_target_components(cxlog
							INC spdlog/cxlog.h
							DLIB cxlog
							LIB cxlog
							PRE spdlog
							)
	
	__test_import(cxlog dll)
endif()
