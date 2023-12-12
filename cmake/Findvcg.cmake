
# Find vcg
#
# This sets the following variables:
# vcg_INCLUDE_DIRS
# vcg_FOUND

find_path(vcg_INCLUDE_DIR vcg/complex/base.h
    HINTS "$ENV{CX_THIRDPARTY_ROOT}/include/vcglib/"
	PATHS "/usr/local/include/vcglib/"
	NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
	)
	
if(vcg_INCLUDE_DIR)
	set(vcg_INCLUDE_DIRS ${vcg_INCLUDE_DIR})
	set(vcg_FOUND "True")
endif()