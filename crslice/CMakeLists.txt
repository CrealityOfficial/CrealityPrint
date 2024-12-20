__enable_cxx17()
include(StandardProjectSettings.cmake)

if(USE_CACHE_ON)
	set(USE_CACHE 1)
else()
	set(USE_CACHE 0)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
 if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/crsliceinfo.h")
 	__build_crslice_info_header()
 endif()
else()
	__build_crslice_info_header()
endif()


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

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/crslice/ SRCS1)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/src/ SRCS2)

set(SRCS ${SRCS1} ${SRCS2})		 
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}
		 ${CMAKE_CURRENT_SOURCE_DIR}/src
		 )
set(DEFS BOOST_ALL_NO_LIB)
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR})

include(gcode/gcode.cmake)
include(impl/cura5.0/cura.cmake)
include(Slice3rBase.cmake)

#binary
set(BINARY_JSONS)
set(BINARY_PARAMETERS)
if(Python3_FOUND)
	include(parameter/binary.cmake)
	include(parameter/parameter.cmake)
endif()

__add_common_library(crslice FOLDER slice OPENMP
							 GENERATED_SOURCE ${BINARY_JSONS} ${BINARY_PARAMETERS}
							 )

use_threads(crslice)
enable_sanitizers(crslice)

if(CC_INSTALL_CRSLICE)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/crslice crslice)
endif()
								