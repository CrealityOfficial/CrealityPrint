if(NOT PROJECT_NAME OR ${PROJECT_NAME} STREQUAL "Project")
	message(FATAL_ERROR "project name null. Please add cmake command project(XXXX), XXXX must not be Project")
else()
	message(STATUS "Project name : ${PROJECT_NAME}")
endif()

include(ConfigureTarget)
include(ConfigureQt)
include(FileUtil)
include(ConfigureProperty)
include(FindUtil)
include(Boost)
include(CXX)
include(Warning)

#Version 
set(PROJECT_VERSION_MAJOR)
set(PROJECT_VERSION_MINOR)
set(PROJECT_VERSION_PATCH)
set(PROJECT_BUILD_ID)
set(PROJECT_VERSION_EXTRA)
if(NOT (${PROJECT_VERSION_MAJOR} MATCHES "^[0-9]*$"))
	set(PROJECT_VERSION_MAJOR 0)
	set(PROJECT_VERSION_MINOR 0)
	set(PROJECT_VERSION_PATCH 1)
	set(PROJECT_BUILD_ID 0)
	set(PROJECT_VERSION_EXTRA "Alpha")
endif()

message(STATUS "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

__build_info_header()
include_directories(${CMAKE_BINARY_DIR})

#install
if(WIN32)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/../install/" CACHE PATH "..." FORCE)
	endif()
endif()

message(STATUS "install : ${CMAKE_INSTALL_PREFIX}")
