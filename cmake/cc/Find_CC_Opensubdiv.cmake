# This sets the following variables:
# openvdb target

if(OPENSUBDIV_INSTALL_ROOT)
	message(STATUS "Specified OPENSUBDIV_INSTALL_ROOT : ${OPENSUBDIV_INSTALL_ROOT}")
	set(opensubdiv_INCLUDE_ROOT ${OPENSUBDIV_INSTALL_ROOT}/include/)
	set(opensubdiv_LIB_ROOT ${OPENSUBDIV_INSTALL_ROOT}/lib/)
	__search_target_components(opensubdiv
							   INC opensubdiv/version.h
							   DLIB opensubdiv
							   LIB opensubdiv
							   )
else()
endif()

__test_import(opensubdiv lib)