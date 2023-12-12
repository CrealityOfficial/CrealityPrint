# This sets the following variables:
# FMT_INCLUDE_DIRS
# FMT_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "Eigen Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(FMT_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/)
	find_path(FMT_INCLUDE_DIRS fmt/format.h
				HINTS "${FMT_INSTALL_ROOT}"
				PATHS "/usr/local/include/fmt"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
else()
endif()
	
if(FMT_INCLUDE_DIRS)
	set(FMT_FOUND 1)
	message(STATUS "FMT_INCLUDE_DIRS : ${FMT_INCLUDE_DIRS}")
else()
	message(STATUS "Find Fmt Failed")
endif()