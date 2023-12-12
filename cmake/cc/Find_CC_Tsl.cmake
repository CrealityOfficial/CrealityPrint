# This sets the following variables:
# TSL_INCLUDE_DIRS
# TSL_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "TSL Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(TSL_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/)
	find_path(TSL_INCLUDE_DIRS tsl/robin_map.h
				HINTS "${TSL_INSTALL_ROOT}"
				PATHS "/usr/local/include/tsl"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
else()
endif()
	
if(TSL_INCLUDE_DIRS)
	set(TSL_FOUND 1)
	message(STATUS "TSL_INCLUDE_DIRS : ${TSL_INCLUDE_DIRS}")
else()
	message(STATUS "Find Fmt Failed")
endif()