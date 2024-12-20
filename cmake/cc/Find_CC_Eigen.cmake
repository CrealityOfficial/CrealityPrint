# This sets the following variables:
# Eigen_INCLUDE_DIRS
# Eigen_FOUND

if(NOT EIGEN_INCLUDE_DIRS)
	if(HEADER_INSTALL_ROOT)
		message(STATUS "Eigen Specified HEADER_INSTALL_ROOT : ${HEADER_INSTALL_ROOT}")
		set(EIGEN_INSTALL_ROOT ${HEADER_INSTALL_ROOT}/include/eigen/)
	elseif(CXEIGEN_INSTALL_ROOT)
		set(EIGEN_INSTALL_ROOT ${CXEIGEN_INSTALL_ROOT})
	endif()
	
	find_path(EIGEN_INCLUDE_DIRS Eigen/Cholesky
				HINTS "${EIGEN_INSTALL_ROOT}"
				PATHS "/usr/local/include/eigen/" "/usr/include/eigen/"
					"/usr/local/include/" "/usr/include/"
					"$ENV{USR_INSTALL_ROOT}/include/" "$ENV{USR_INSTALL_ROOT}/include/eigen"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
		
	if(EIGEN_INCLUDE_DIRS)
		set(Eigen_FOUND 1)
		message(STATUS "EIGEN_INCLUDE_DIRS : ${EIGEN_INCLUDE_DIRS}")
	else()
		message(STATUS "Find Eigen Failed")
	endif()
endif()