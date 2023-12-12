# This sets the following variables:
# zip target

if(NOT TARGET zip)
	__search_target_components(zip
							INC zip.h
							DLIB zip
							LIB zip
							PRE zip
							)
	
	__test_import(zip dll)
endif()