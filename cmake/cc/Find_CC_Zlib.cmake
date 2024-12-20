# This sets the following variables:
# zlib target

if(NOT TARGET zlib)
	__search_target_components(zlib
							INC zlib.h
							DLIB zlib
							LIB zlib
							PRE zlib
							)
	
	__test_import(zlib dll INTERFACE_DEF Z_PREFIX)
endif()