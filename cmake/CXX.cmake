macro(__enable_cxx17)
	if ( CMAKE_SYSTEM_NAME MATCHES "Windows" )
		set( my_std_pre "/std:" )
	else()
		set( my_std_pre "-std=" )
	endif()

	set( basic_cxx17 "c++17" )
	set( str_cxx17 "${my_std_pre}${basic_cxx17}" )
	
	include( CheckCXXCompilerFlag )
	check_cxx_compiler_flag( "${str_cxx17}" _cpp_17_flag_supported )
	if ( _cpp_17_flag_supported )
		set( CMAKE_CXX_STANDARD 17 )
	endif()
endmacro()

macro(__enable_cxx14)
	if ( CMAKE_SYSTEM_NAME MATCHES "Windows" )
		set( my_std_pre "/std:" )
	else()
		set( my_std_pre "-std=" )
	endif()

	set( basic_cxx14 "c++14" )
	set( str_cxx14 "${my_std_pre}${basic_cxx14}" )
	
	include( CheckCXXCompilerFlag )
	check_cxx_compiler_flag( "${str_cxx14}" _cpp_14_flag_supported )
	if ( _cpp_14_flag_supported )
		set( CMAKE_CXX_STANDARD 14 )
	endif()
endmacro()

macro(__enable_mem_leak_check)
	if(WIN32)
		add_definitions(-DCXX_MEMORY_LEAK_CHECK)
	else()
	endif()
endmacro()

if(NOT WIN32)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release")
		message(STATUS "Default Build Type ${CMAKE_BUILD_TYPE}")
	endif()
	
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_definitions(-DNDEBUG)
	else()
		add_definitions(-DDEBUG)
		add_definitions(-D_DEBUG)
	endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	
    #include_directories("/usr/local/include/")
    #	link_directories("/usr/local/lib/")
endif()

__enable_cxx14()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include_directories(${CMAKE_BINARY_DIR})

if(WIN32)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<CONFIG:Release>:NDEBUG>)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
		$<$<CONFIG:Debug>:CXX_CHECK_MEMORY_LEAKS>)
	if(NOT DISABLE_DEBUG_DEF)
		set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
			$<$<CONFIG:Debug>:_DEBUG>)
		set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
			$<$<CONFIG:Debug>:DEBUG>)
	endif()
	
	#add_definitions(-DUNICODE -D_UNICODE)
endif()

macro(__enable_vld)
	if(WIN32)
		message(STATUS "__enable_vld --------> : ${CMAKE_SOURCE_DIR}")
		include_directories(${CMAKE_SOURCE_DIR}/cmake/vld)
		link_directories(${CMAKE_SOURCE_DIR}/cmake/vld)
		add_custom_target(__vld ALL COMMENT "memory leak check!")
		__set_target_folder(__vld CMakePredefinedTargets) 
		add_custom_command(TARGET __vld PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_OUTPUT_DIR}/Debug/
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/vld/vld_x64.dll ${BIN_OUTPUT_DIR}/Debug/
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/vld/dbghelp.dll ${BIN_OUTPUT_DIR}/Debug/
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/vld/Microsoft.DTfW.DHL.manifest ${BIN_OUTPUT_DIR}/Debug/
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/vld/vld.ini ${BIN_OUTPUT_DIR}/Debug/
			)
		set(CXX_VLD_ENABLED "ON")
	endif()
endmacro()

if(CXX_VLD)
	__enable_vld()
endif()

if(RENDER_DOC)
	include(RenderDoc)
endif()

macro(__enable_cxdef)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cxdef)
		message(FATAL_ERROR "not exist cxdef ${CMAKE_CURRENT_SOURCE_DIR}/cxdef")
	endif()
	
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cxdef)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cxdef/interface)
endmacro()

macro(__remove_MDd)
	include(AdjustToolFlags)
	IF(MSVC)
		# Change warning level to something saner
		# Force to always compile with W0
		if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
			string(REGEX REPLACE "/W[0-4]" "/W0" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
		endif()
	
		# Base settings
	
		# Disable incremental linking, because LTCG is currently triggered by
		# linking with a pre-built lib and then we'll see a warning:
		AdjustToolFlags(
					CMAKE_EXE_LINKER_FLAGS_DEBUG
					CMAKE_MODULE_LINKER_FLAGS_DEBUG
					CMAKE_SHARED_LINKER_FLAGS_DEBUG
				ADDITIONS "/INCREMENTAL:NO"
				REMOVALS "/INCREMENTAL(:YES|:NO)?")
	
		# Always link with the release runtime DLL:
		AdjustToolFlags(CMAKE_C_FLAGS CMAKE_CXX_FLAGS "/MD")
		AdjustToolFlags(
					CMAKE_C_FLAGS_DEBUG
					CMAKE_C_FLAGS_RELEASE
					CMAKE_C_FLAGS_MINSIZEREL
					CMAKE_C_FLAGS_RELWITHDEBINFO
					CMAKE_CXX_FLAGS_DEBUG
					CMAKE_CXX_FLAGS_RELEASE
					CMAKE_CXX_FLAGS_MINSIZEREL
					CMAKE_CXX_FLAGS_RELWITHDEBINFO
				REMOVALS "/MDd? /MTd? /RTC1 /D_DEBUG")
	ENDIF(MSVC)
endmacro()

macro(__enable_spycc)
	add_definitions(-DUSE_SPYCC)
	
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/spycc)
		message(STATUS "__enable_spycc ----> use source to compile.")
		add_subdirectory(spycc)
	else()
		message(STATUS "__enable_spycc ----> use precompiled.")
		__cc_find(Spycc)
	endif()
	__assert_target(spycc)
	
	set(SPYCC_LIB spycc)
endmacro()

if(CXX_SPYCC)
	__enable_spycc()
endif()

macro(__enable_bigobj)
	if(WIN32)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	endif()
endmacro()

macro(__enable_gprof)
	if(CC_BC_LINUX)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
	endif()
endmacro()

macro(__execute)
	message(STATUS "__execute ${ARGV} :")
	
	execute_process(COMMAND ${ARGV}
					OUTPUT_VARIABLE ret)
	message(STATUS ${ret})
endmacro()