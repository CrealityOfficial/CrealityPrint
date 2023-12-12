# This sets the following variables:
# quazip_INCLUDE_DIRS
# quazip_LIBRARIES_DEBUG quazip_LIBRARIES_RELEASE
# quazip target

if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(zlib_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/zlib/)
	set(zlib_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
elseif(CXQUAZIP_INSTALL_ROOT)
	set(quazip_INCLUDE_ROOT ${CXQUAZIP_INSTALL_ROOT}/include/)
	set(quazip_LIB_ROOT ${CXQUAZIP_INSTALL_ROOT}/lib/)
endif()

__search_target_components(quazip
						   INC quazip/quazip.h
						   DLIB quazip
						   LIB quazip
						   )

__test_import(quazip dll)