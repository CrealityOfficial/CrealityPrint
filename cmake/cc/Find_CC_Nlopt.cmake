# This sets the following variables:
# nlopt target

if(NOT TARGET nlopt)
	__search_target_components(nlopt
							INC nlopt.hpp
							DLIB nlopt
							LIB nlopt
							PRE nlopt
							)
	
	__test_import(nlopt dll)
endif()