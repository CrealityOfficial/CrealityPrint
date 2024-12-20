# This sets the following variables:
# nestplacer target
if(NOT TARGET nestplacer)
	__cc_find(LibNest2D)
	__search_target_components(nestplacer
							INC nestplacer/nestplacer.h
							DLIB nestplacer
							LIB nestplacer
							PRE nestplacer
							)
	
	__test_import(nestplacer dll)
endif()