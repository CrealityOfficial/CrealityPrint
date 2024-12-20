# This sets the following variables:
# dxflib_INCLUDE_DIRS
# dxflib_LIBRARIES_DEBUG dxflib_LIBRARIES_RELEASE
# dxflib target

if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(zlib_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/zlib/)
	set(zlib_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
elseif(CXDXFLIB_INSTALL_ROOT)
	set(dxflib_INCLUDE_ROOT ${CXDXFLIB_INSTALL_ROOT}/include/)
	set(dxflib_LIB_ROOT ${CXDXFLIB_INSTALL_ROOT}/lib/)
endif()

__search_target_components(dxflib
						   INC dxf/dl_attributes.h
						   DLIB dxflib
						   LIB dxflib
						   PRE dxf
						   )

__test_import(dxflib dll)