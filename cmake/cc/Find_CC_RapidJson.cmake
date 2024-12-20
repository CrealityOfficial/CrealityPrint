# This sets the following variables:
# RapidJson_INCLUDE_DIRS
# RapidJson_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "RapidJson Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(RAPIDJSON_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/)
	find_path(RAPIDJSON_INCLUDE_DIRS rapidjson/rapidjson.h
				HINTS "${RAPIDJSON_INSTALL_ROOT}"
				PATHS "/usr/local/include/rapidjson/rapidjson.h"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
elseif(CXRAPIDJSON_INSTALL_ROOT)
	message(STATUS "****Specified CXRAPIDJSON_INSTALL_ROOT : ${CXRAPIDJSON_INSTALL_ROOT}")
	set(RAPIDJSON_INSTALL_ROOT ${CXRAPIDJSON_INSTALL_ROOT})
endif()

find_path(RAPIDJSON_INCLUDE_DIRS rapidjson/rapidjson.h
			HINTS "${RAPIDJSON_INSTALL_ROOT}"
			PATHS "$ENV{USR_INSTALL_ROOT}/include/rapidjson" "$ENV{USR_INSTALL_ROOT}/include/" 
			"/usr/include/" "/usr/local/include/" 
					"/usr/include/rapidjson" "/usr/local/include/rapidjson/"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
	
if(RAPIDJSON_INCLUDE_DIRS)
	set(RapidJson_FOUND 1)
	message(STATUS "RAPIDJSON_INCLUDE_DIRS : ${RAPIDJSON_INCLUDE_DIRS}")
else()
	message(STATUS "Find RapidJson Failed")
endif()

__add_include_interface(rapidjson INTERFACE ${RAPIDJSON_INCLUDE_DIRS})