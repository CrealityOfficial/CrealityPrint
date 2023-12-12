# This sets the following variables:
# freeImage target

if(NOT TARGET freeImage)
 if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(freeImage_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/freeImage/)
	set(freeImage_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
 else()
 endif()

 __search_target_components(freeImage
							    INC FreeImage.h
							    DLIB freeImage
							    LIB freeImage
								PRE freeImage
							    )
 __test_import(freeImage dll)

endif()