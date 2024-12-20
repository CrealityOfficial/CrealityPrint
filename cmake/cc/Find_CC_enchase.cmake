# enchase target

if(NOT TARGET enchase)
	__search_target_components(enchase
							INC enchase/default.h
							DLIB enchase
							LIB enchase
							PRE enchase
							)
	
	__test_import(enchase lib)
endif()
