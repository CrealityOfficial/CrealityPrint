# This sets the following variables:
# bcd

if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(bcd_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/)
	set(bcd_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
	__search_target_components(bcd
							   INC bcd/core/Denoiser.h
							   DLIB bcd
							   LIB bcd
							   )
else()
endif()

__test_import(bcd lib)