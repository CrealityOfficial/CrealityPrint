# This sets the following variables:
# clipper3r target

if(THIRD1_INSTALL_ROOT)
elseif(CXCLIPPER_INSTALL_ROOT)
endif()

if(NOT TARGET clipper3r)
	__search_target_components(clipper3r
							INC clipper3r/clipper.hpp
							DLIB clipper3r
							LIB clipper3r
							PRE clipper3r
							)
	
	__test_import(clipper3r lib)
endif()