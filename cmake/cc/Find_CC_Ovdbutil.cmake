# This sets the following variables:
# cxbin target
if(NOT TARGET ovdbutil)
	__cc_find(Trimesh2)
	__cc_find(Zlib)
	__cc_find(Tbb)
	__cc_find(Boost)
	__search_target_components(ovdbutil
							INC ovdbutil/hollowing.h
							DLIB ovdbutil
							LIB ovdbutil
							PRE ovdbutil
							)
	
	__test_import(ovdbutil dll)
endif()