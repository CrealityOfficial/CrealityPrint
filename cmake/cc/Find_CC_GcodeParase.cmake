# This sets the following variables:
# gcodeparase target
if(NOT TARGET gcodeparase)
	__search_target_components(gcodeparase
							INC gcodeparase/gcodeparase.h
							DLIB gcodeparase
							LIB gcodeparase
							PRE gcodeparase
							)
	
	__test_import(gcodeparase lib)
endif()