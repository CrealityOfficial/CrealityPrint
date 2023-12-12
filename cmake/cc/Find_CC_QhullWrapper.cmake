# This sets the following variables:
# qhullWrapper target

__cc_find(Cxnd)
if(NOT TARGET qhullWrapper)
	__search_target_components(qhullWrapper
							INC qhullWrapper/interface.h
							DLIB qhullWrapper
							LIB qhullWrapper
							PRE qhullWrapper
							)
	
	__test_import(qhullWrapper dll)
endif()