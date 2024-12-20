# This sets the following variables:
# opennurbs target

if(OPENNURBS_INSTALL_ROOT)
	message(STATUS "Specified OPENNURBS_INSTALL_ROOT : ${OPENNURBS_INSTALL_ROOT}")
	set(opennurbs_INCLUDE_ROOT ${OPENNURBS_INSTALL_ROOT}/include/opennurbs/opennurbs/)
	set(opennurbs_LIB_ROOT ${OPENNURBS_INSTALL_ROOT}/lib/)
endif()

__search_target_components(opennurbs
						   INC opennurbs.h
						   DLIB opennurbs
						   LIB opennurbs
						   PRE opennurbs/opennurbs/
						   )
							   
__test_import(opennurbs dll)