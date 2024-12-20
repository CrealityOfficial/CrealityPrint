list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cc/")

macro(__cc_find)
	if(HAVE_CONAN_CACHE OR CREATE_CONAN_PACKAGE)
		string(TOLOWER ${ARGN} LTARGET)
		__conan_find(${LTARGET})
	elseif(HAVE_CONAN_CACHE)
		message(FATAL_ERROR "__cc_find must use conan, cc is abandoned.")
	endif()
endmacro()

function(__test_import target type)
	if(NOT ${target}_LIBRARIES_DEBUG AND ${target}_LIBRARIES_RELEASE)
		set(${target}_LIBRARIES_DEBUG ${${target}_LIBRARIES_RELEASE})
		message("${target} use release replace debug.")
	endif()
	
	if(${target}_INCLUDE_DIRS AND ${target}_LIBRARIES_DEBUG AND ${target}_LIBRARIES_RELEASE)
		set(${target}_FOUND 1)
		__import_target(${target} ${type} ${ARGN})
		message(STATUS "import ${target} success.")
	else()
		if(CC_BC_LINUX AND ${target}_INCLUDE_DIRS AND ${target}_LIBRARIES_RELEASE)
			set(${target}_FOUND 1)
			__import_target(${target} ${type} ${ARGN})
			message(STATUS "import ${target} success.")			
		else()
			message(STATUS "import ${target} failed.")
		endif()
	endif()
endfunction()

function(__search_target_components target)
	cmake_parse_arguments(search "" "" "INC;DLIB;LIB;PRE;" ${ARGN})
	
	find_path(${target}_INCLUDE_DIRS
			NAMES ${search_INC}
			HINTS "${${target}_INCLUDE_ROOT}"
			PATHS "$ENV{USR_INSTALL_ROOT}/include/" "$ENV{USR_INSTALL_ROOT}/include/${search_PRE}"
			        "/usr/include/" "/usr/include/${target}/"
					"/usr/local/include/" "/usr/local/include/${target}/"
					"/usr/include/${search_PRE}" "/usr/local/include/${search_PRE}/"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)

	find_library(${target}_LIBRARIES_DEBUG
				NAMES ${search_DLIB}
				HINTS "${${target}_LIB_ROOT}/Debug"
				PATHS "$ENV{USR_INSTALL_ROOT}/lib/Debug/" "$ENV{USR_INSTALL_ROOT}/bin/Debug" "/usr/lib/Debug" "/usr/local/lib/Debug" "/usr/lib/${search_PRE}"
					"/usr/bin/Debug" "/usr/local/bin/Debug"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
				
	find_library(${target}_LIBRARIES_RELEASE
			NAMES ${search_LIB}
			HINTS "${${target}_LIB_ROOT}/Release"
			PATHS "$ENV{USR_INSTALL_ROOT}/lib/Release/" "$ENV{USR_INSTALL_ROOT}/bin/Release/" "$ENV{USR_INSTALL_ROOT}/bin/Release/lib/" "/usr/lib/Release" "/usr/lib/${search_PRE}" "/usr/local/lib/Release"
				"/usr/bin/Release" "/usr/local/bin/Release"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
	message("${target}_INCLUDE_DIRS  ${${target}_INCLUDE_DIRS}")
	if(NOT CC_BC_LINUX)
		message("${target}_LIBRARIES_DEBUG  ${${target}_LIBRARIES_DEBUG}")
	endif()
	message("${target}_LIBRARIES_RELEASE  ${${target}_LIBRARIES_RELEASE}")
endfunction()

function(__search_target_components_signle target)
	cmake_parse_arguments(search "" "" "INC;DLIB;LIB;PRE;" ${ARGN})
	find_path(${target}_INCLUDE_DIRS
			NAMES ${search_INC}
			HINTS "${${target}_INCLUDE_ROOT}"
			PATHS "$ENV{USR_INSTALL_ROOT}/include/" "$ENV{USR_INSTALL_ROOT}/include/${search_PRE}" 
			        "/usr/include/" "/usr/include/${target}/"
					"/usr/local/include/" "/usr/local/include/${target}/"
					"/usr/include/${search_PRE}" "/usr/local/include/${search_PRE}/"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
	find_library(${target}_LIBRARIES_DEBUG
				NAMES ${search_DLIB}
				HINTS "${${target}_LIB_ROOT}"
				PATHS "/usr/lib" "/usr/lib/x86_64-linux-gnu" "/usr/lib/${search_PRE}" "/usr/local/lib" "$ENV{USR_INSTALL_ROOT}/lib/"
				    "$ENV{USR_INSTALL_ROOT}/bin/" "$ENV{USR_INSTALL_ROOT}/lib/${search_PRE}"
					"/usr/bin/Debug" "/usr/local/bin"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
				
	find_library(${target}_LIBRARIES_RELEASE
			NAMES ${search_LIB}
			HINTS "${${target}_LIB_ROOT}"
			PATHS  "$ENV{USR_INSTALL_ROOT}/lib/" "$ENV{USR_INSTALL_ROOT}/bin/" "/usr/lib" "/usr/lib/x86_64-linux-gnu" "/usr/lib/${search_PRE}" "/usr/local/lib"
				"/usr/bin" "/usr/local/bin"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
			
	if(WIN32)
		set(${target}_LOC_DEBUG "$ENV{USR_INSTALL_ROOT}/bin/Debug/${target}.dll")
		set(${target}_LOC_RELEASE "$ENV{USR_INSTALL_ROOT}/bin/Release/${target}.dll")
	endif()
	
	message("${target}_INCLUDE_DIRS  ${${target}_INCLUDE_DIRS}")
	message("${target}_LIBRARIES_DEBUG  ${${target}_LIBRARIES_DEBUG}")
	message("${target}_LIBRARIES_RELEASE  ${${target}_LIBRARIES_RELEASE}")
endfunction()

if(CC_BUILD_TEST)
	message(STATUS "CC_BUILD_TEST Enabled.")
endif()
