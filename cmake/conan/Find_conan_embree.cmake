#embree target

if(TARGET embree)
	return()
endif()

include(${CONAN_EMBREE_ROOT_RELEASE}/lib/cmake/embree-3.13.5/embree-targets.cmake)
set(_IMPORT_PREFIX ${CONAN_EMBREE_ROOT_DEBUG})
include(${CONAN_EMBREE_ROOT_DEBUG}/lib/cmake/embree-3.13.5/embree-targets-debug.cmake)
__copy_find_targets(embree)