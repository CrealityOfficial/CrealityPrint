
# Find zlib
#
# This sets the following variables:
# zlib_INCLUDE_DIRS
# zlib_LIBRARIES
# zlib_LIBRARIE_DIRS
# zlib_FOUND

find_path(zlib_INCLUDE_DIR zlib.h
    HINTS "$ENV{CX_THIRDPARTY_ROOT}/include/zlib"
	PATHS "/usr/include/zlib")
	
if(zlib_INCLUDE_DIR)
	set(zlib_INCLUDE_DIRS ${zlib_INCLUDE_DIR})
endif()

find_library(zlib_LIBRARIES_DEBUG
             NAMES zlib
             HINTS "$ENV{CX_THIRDPARTY_ROOT}/lib/Debug"
			 PATHS "/usr/lib/Debug")
			 
find_library(zlib_LIBRARIES_RELEASE
         NAMES zlib
         HINTS "$ENV{CX_THIRDPARTY_ROOT}/lib/Release"
		 PATHS "/usr/lib/Release")
			 
message("zlib_INCLUDE_DIR  ${zlib_INCLUDE_DIR}")
message("zlib_LIBRARIES_DEBUG  ${zlib_LIBRARIES_DEBUG}")
message("zlib_LIBRARIES_RELEASE  ${zlib_LIBRARIES_RELEASE}")

if(zlib_INCLUDE_DIRS AND zlib_LIBRARIES_DEBUG AND zlib_LIBRARIES_RELEASE)
	set(zlib_FOUND "True")
	__import_target(zlib lib)
endif()
