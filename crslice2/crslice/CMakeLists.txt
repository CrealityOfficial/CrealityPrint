__enable_cxx17()
include(StandardProjectSettings.cmake)

if(USE_CACHE_ON)
	set(USE_CACHE 1)
else()
	set(USE_CACHE 0)
endif()

__build_crslice_info_header()
__enable_openmp()

__cc_find(trimesh2)
__assert_target(trimesh2)

__cc_find(polyclipping)
__assert_target(polyclipping)

__cc_find(stb)
__assert_target(stb)

__cc_find(rapidjson)
__assert_target(rapidjson)
			
set(ILIBS trimesh2) 
set(LIBS polyclipping stb rapidjson)

__cc_find(cxlog)
if(TARGET cxlog)
	list(APPEND LIBS cxlog)
endif() 

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/crslice2/ SRCS1)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/src/ SRCS2)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/wrapper/ SRCS3)

set(SRCS ${SRCS1} ${SRCS2} ${SRCS3})		 
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}
		 ${CMAKE_CURRENT_SOURCE_DIR}/src
		 )
set(DEFS BOOST_ALL_NO_LIB)
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(qhull)
add_subdirectory(glu-libtess)

if(CC_BC_MAC)
find_library(CoreFoundationLib CoreFoundation)
find_library(FOUNDATION_LIBRARY Foundation)
find_library(ModelIO_LIBRARY ModelIO)
#	add_compile_options(-fno-aligned-allocation)
endif()

add_subdirectory(tbb)
add_subdirectory(imgui)
add_subdirectory(libslic3r)

#binary
set(BINARY_JSONS)
set(BINARY_PARAMETERS)
if(Python3_FOUND)
	include(parameter/binary.cmake)
	#include(parameter/parameter.cmake)
endif()

list(APPEND DEFS _UNICODE UNICODE NOMINMAX BOOST_ALL_NO_LIB _USE_MATH_DEFINES)
if(USE_BENCH_DEBUG)
	list(APPEND DEFS BENCH_DEBUG) 
endif()

if(TARGET libslic3r)
	list(APPEND LIBS libslic3r)
endif()
if(CC_BC_WIN)
	list(APPEND LIBS Bcrypt.lib)
endif()
if(CC_BC_MAC)
	list(APPEND LIBS ${CoreFoundationLib} ${FOUNDATION_LIBRARY} ${ModelIO_LIBRARY})
endif()

# 查找Windows SDK
find_package(WindowsSDK 10.0)
if(WINDOWSSDK_FOUND)
	message(STATUS "WINDOWSSDK_LATEST_DIR ${WINDOWSSDK_LATEST_DIR} ")
	message(STATUS "WINDOWSSDK_LATEST_NAME ${WINDOWSSDK_LATEST_NAME} ")
	#message(STATUS "WINDOWSSDK_PREFERRED_NAME ${WINDOWSSDK_PREFERRED_NAME} ")
	#message(STATUS "WINDOWSSDK_FOUND_PREFERENCE ${WINDOWSSDK_FOUND_PREFERENCE} ")
	#message(STATUS "WINDOWSSDK_PREFERRED_DIR ${WINDOWSSDK_PREFERRED_DIR} ")
	#
	#message(STATUS "WINDOWSSDK_DIRS ${WINDOWSSDK_DIRS} ")
	#message(STATUS "WINDOWSSDK_PREFERRED_FIRST_DIRS ${WINDOWSSDK_PREFERRED_FIRST_DIRS} ")
	
	get_windowssdk_include_dirs_all(${WINDOWSSDK_LATEST_DIR} WINSDK_INCS)
	#message(STATUS "WINDOWSSDK_INCS ${WINSDK_INCS} ")
	
	if(NOT "${WINSDK_INCS}" STREQUAL "")
		list(APPEND DEFS HAS_WIN10SDK)
		list(APPEND INCS ${WINSDK_INCS})
	endif()
endif()

__add_common_library(crslice2 FOLDER slice OPENMP
							 DEF ${DEFS}
							 INC ${INCS}
							 LIB ${LIBS}
							 ILIB ${ILIBS}
							 GENERATED_SOURCE ${BINARY_JSONS} ${BINARY_PARAMETERS}
							 )

use_threads(crslice2)
enable_sanitizers(crslice2)

if(CC_INSTALL_CRSLICE2)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/crslice2 crslice2)
endif()
								
