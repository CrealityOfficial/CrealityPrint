# This sets the following variables:
# CLIPPER2_INCLUDE_DIRS
# Clipper2_FOUND

if(HEADER_INSTALL_ROOT)
	message(STATUS "Clipper2 Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
	set(CLIPPER2_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/)
	find_path(CLIPPER2_INCLUDE_DIRS clipper2/clipper.h
				HINTS "${CLIPPER2_INSTALL_ROOT}"
				PATHS "/usr/local/include/"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
else()
endif()
	
if(CLIPPER2_INCLUDE_DIRS)
	set(Clipper2_FOUND 1)
	message(STATUS "CLIPPER2_INCLUDE_DIRS : ${CLIPPER2_INCLUDE_DIRS}")
else()
	message(STATUS "Find Clipper2 Failed")
endif()
