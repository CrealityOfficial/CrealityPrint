# librevenge target
if(CONAN_LIBREVENGE_ROOT_RELEASE)
	set(LIBREVENGE_INCLUDE_DIRS "${CONAN_LIBREVENGE_ROOT_RELEASE}/include/librevenge/")
endif()

__conan_import(librevenge dll)

