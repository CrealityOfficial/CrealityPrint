# This sets the following variables:
# zlib_INCLUDE_DIRS
# zlib_LIBRARIES_DEBUG zlib_LIBRARIES_RELEASE
# zlib target

if(OIIO_INSTALL_ROOT)
	message(STATUS "Specified OIIO_INSTALL_ROOT : ${OIIO_INSTALL_ROOT}")
	set(OpenImageIO_INCLUDE_ROOT ${OIIO_INSTALL_ROOT}/include/)
	set(OpenImageIO_LIB_ROOT ${OIIO_INSTALL_ROOT}/lib/)
	__search_target_components(OpenImageIO
							   INC OpenImageIO/imageio.h
							   DLIB OpenImageIO
							   LIB OpenImageIO
							   )
else()
endif()

__test_import(OpenImageIO dll)