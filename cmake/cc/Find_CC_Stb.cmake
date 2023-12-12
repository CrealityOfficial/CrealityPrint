# This sets the following variables:
# STB_INCLUDE_DIRS
# STB_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "Eigen Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(STB_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/vcglib/)
elseif(CXSTB_INSTALL_ROOT)
	message(STATUS "****Specified CXSTB_INSTALL_ROOT : ${CXSTB_INSTALL_ROOT}")
	set(STB_INSTALL_ROOT ${CXSTB_INSTALL_ROOT})
endif()
	
find_path(STB_INCLUDE_DIRS stb/stb.h
			HINTS "${STB_INSTALL_ROOT}"
			PATHS "/usr/include/" "/usr/local/include/" 
					"/usr/include/stb" "/usr/local/include/stb/"
					"$ENV{USR_INSTALL_ROOT}/include/" "$ENV{USR_INSTALL_ROOT}/include/stb"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
				
if(STB_INCLUDE_DIRS)
	set(STB_FOUND 1)
	message(STATUS "STB_INCLUDE_DIRS : ${STB_INCLUDE_DIRS}")
else()
	message(STATUS "Find stb Failed")
endif()