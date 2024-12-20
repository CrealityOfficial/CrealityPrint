# This sets the following variables:
# tiff target

if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(tiff_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/tiff/)
	set(tiff_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
	__search_target_components(tiff
							   INC tiff.h
							   DLIB tiff
							   LIB tiff
							   )
else()
endif()

__test_import(tiff dll)