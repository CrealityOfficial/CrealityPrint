#openexr target

if(TARGET OpenEXR::OpenEXR)
	return()
endif()

include(${CONAN_OPENEXR_ROOT_RELEASE}/lib/cmake/Imath/ImathTargets.cmake)
set(_IMPORT_PREFIX ${CONAN_OPENEXR_ROOT_DEBUG})
include(${CONAN_OPENEXR_ROOT_DEBUG}/lib/cmake/Imath/ImathTargets-debug.cmake)

include(${CONAN_OPENEXR_ROOT_RELEASE}/lib/cmake/OpenEXR/OpenEXRTargets.cmake)
set(_IMPORT_PREFIX ${CONAN_OPENEXR_ROOT_DEBUG})
include(${CONAN_OPENEXR_ROOT_DEBUG}/lib/cmake/OpenEXR/OpenEXRTargets-debug.cmake)

find_package(Threads)
__cc_find(zlib_static)

__assert_target(Imath::Imath)
__assert_target(OpenEXR::Iex)
__assert_target(OpenEXR::IlmThread)
__assert_target(OpenEXR::OpenEXRCore)
__assert_target(OpenEXR::OpenEXR)
__assert_target(OpenEXR::OpenEXRUtil)