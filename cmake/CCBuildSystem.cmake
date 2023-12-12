#Build Case Flag
#CC_BC_WIN   
#CC_BC_ANDROID
#CC_BC_MAC
#CC_BC_IOS
#CC_BC_GNU 
#CC_BC_EMCC

set(CC_BC_WIN 0)
set(CC_BC_ANDROID 0)
set(CC_BC_MAC 0)
set(CC_BC_IOS 0)
set(CC_BC_LINUX 0)
set(CC_BC_EMCC 0)

message(STATUS "CMAKE_SYSTEM_NAME ---> ${CMAKE_SYSTEM_NAME}")
if(CMAKE_TOOLCHAIN_FILE)
	message(STATUS "CMake Build Cross Compile [${CMAKE_TOOLCHAIN_FILE}]")
	set(CC_CROSS_COMPILE 1)
endif()

if(CC_CROSS_COMPILE)
endif()

if(WIN32 AND NOT CC_CROSS_COMPILE)
	set(CC_BC_WIN 1)
endif()

if(CC_CROSS_COMPILE AND CMAKE_SYSTEM_NAME)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
		set(CC_BC_EMCC 1)
	endif()
endif()

if(APPLE AND NOT CC_CROSS_COMPILE)
	set(CC_BC_MAC 1)
endif()

if(CC_CROSS_COMPILE AND ANDROID)
	set(CC_BC_ANDROID 1)
endif()

if(UNIX AND NOT APPLE AND NOT CC_CROSS_COMPILE)
	set(CC_BC_LINUX 1)
endif()

if(CC_BUILD_IPHONE_PLATFORM AND CC_CROSS_COMPILE)
	set(CC_BC_IOS 1)
endif()

if(CC_BC_WIN)
	message(STATUS "CC Build Case [ WIN ]")
	add_definitions(-DCC_SYSTEM_WIN)
endif()
if(CC_BC_ANDROID)
	message(WARNING "CC Build Case [ ANDROID ]")
	add_definitions(-DCC_SYSTEM_ANDROID)
endif()
if(CC_BC_MAC)
	message(STATUS "CC Build Case [ MAC ]")
	add_definitions(-DCC_SYSTEM_MAC)
endif()
if(CC_BC_IOS)
	message(STATUS "CC Build Case [ IOS ]")
	add_definitions(-DCC_SYSTEM_IOS)
endif()
if(CC_BC_LINUX)
	message(STATUS "CC Build Case [ LINUX ]")
	add_definitions(-DCC_SYSTEM_LINUX)
endif()
if(CC_BC_EMCC)
	message(STATUS "CC Build Case [ EMCC ]")
	add_definitions(-DCC_SYSTEM_EMCC)
endif()

#Build System
#CC_BS_MSVC
#CC_BS_NINJA
#CC_BS_MAKE

set(CC_BS_MSVC 0)
set(CC_BS_NINJA 0)
set(CC_BS_MAKE 0)

if(CC_BS_MSVC)
	message(STATUS "CC Build System [ MSVC ]")
endif()
if(CC_BS_NINJA)
	message(STATUS "CC Build System [ NINJA ]")
endif()
if(CC_BS_MAKE)
	message(STATUS "CC Build System [ MAKE ]")
endif()

macro(__use_build_install_prefix)
	if(DEFINED ENV{USR_INSTALL_ROOT})
		SET(CMAKE_INSTALL_PREFIX $ENV{USR_INSTALL_ROOT})
		message(STATUS "CMAKE_INSTALL_PREFIX Changed to -> [${CMAKE_INSTALL_PREFIX}].")
	endif()
endmacro()
