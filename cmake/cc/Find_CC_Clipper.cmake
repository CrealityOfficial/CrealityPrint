# This sets the following variables:
# clipper target

if(THIRD1_INSTALL_ROOT)
	message(STATUS "Specified THIRD1_INSTALL_ROOT : ${THIRD1_INSTALL_ROOT}")
	set(clipper_INCLUDE_ROOT ${THIRD1_INSTALL_ROOT}/include/)
	set(clipper_LIB_ROOT ${THIRD1_INSTALL_ROOT}/lib/)
elseif(CXCLIPPER_INSTALL_ROOT)
	set(clipper_INCLUDE_ROOT ${CXCLIPPER_INSTALL_ROOT}/include/)
	set(clipper_LIB_ROOT ${CXCLIPPER_INSTALL_ROOT}/lib/)
endif()

__search_target_components(clipper
						   INC clipper/clipper.hpp
						   DLIB clipper
						   LIB clipper
						   PRE clipper
						   )

__test_import(clipper lib)