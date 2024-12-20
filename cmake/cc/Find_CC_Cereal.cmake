# This sets the following variables:
# CEREAL_INCLUDE_DIRS
# Cereal_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "Cereal Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(CEREAL_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/)
	find_path(CEREAL_INCLUDE_DIRS cereal/cereal.hpp
				HINTS "${CEREAL_INSTALL_ROOT}"
				PATHS "/usr/local/include/"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
else()
endif()
	
if(CEREAL_INCLUDE_DIRS)
	set(Cereal_FOUND 1)
	message(STATUS "CEREAL_INCLUDE_DIRS : ${CEREAL_INCLUDE_DIRS}")
else()
	message(STATUS "Find Cereal Failed")
endif()