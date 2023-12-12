# This sets the following variables:
# cxbin target
if(NOT TARGET cxbin)
	__cc_find(ColladaDom)
	__cc_find(StringUtil)
	__cc_find(Zlib)
	__search_target_components(cxbin
							INC cxbin/load.h
							DLIB cxbin
							LIB cxbin
							PRE cxbin
							)
	
	__test_import(cxbin dll)
endif()