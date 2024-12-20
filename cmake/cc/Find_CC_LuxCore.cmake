# This sets the following variables:
# luxcore target

if(LUXCORE_SDK_ROOT)
	message(STATUS "Specified LUXCORE_SDK_ROOT : ${LUXCORE_SDK_ROOT}")
	set(luxcore_INCLUDE_ROOT ${LUXCORE_SDK_ROOT}/include/)
	set(luxcore_LIB_ROOT ${LUXCORE_SDK_ROOT}/lib/)
	__search_target_components(luxcore
							   INC luxcore/luxcore.h
							   DLIB luxcore
							   LIB luxcore
							   )
							   
	file(GLOB LUXCORE_BIN ${LUXCORE_SDK_ROOT}/bin/*.*)
	message(STATUS "LUXCORE_BIN : ${LUXCORE_BIN}")
	__copy_files(_copy_luxcore_bin LUXCORE_BIN LUXCORE_BIN)
else()
endif()

__test_import(luxcore dll)

add_definitions(-DLUXCORE_DLL)
set(LUXCORE_LIBRARY luxcore)