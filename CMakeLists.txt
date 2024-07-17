cmake_minimum_required (VERSION 3.5)
project(CrealityPrint LANGUAGES CXX C)
if(NOT DEFINED BUNDLE_NAME)
        set(BUNDLE_NAME "Creality_Print")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/scripts/cmake/")
include(BuildPlatformDetection)

set(APP_TYPE 7)
set(BUILD_OS "xxxx")
if(BUILD_OS_WIN64)
	set(BUILD_OS "win64")
elseif(BUILD_OS_WIN32)
	set(BUILD_OS "win32")
elseif(BUILD_OS_LINUX)
	set(BUILD_OS "linux")
elseif(BUILD_OS_OSX)
	set(BUILD_OS "macx")
endif()

include(ConfigureAllModules)

__enable_openmp()

set(CC_GLOBAL_PACKAGE_INSTALL 1)
if(BUILD_OS_WIN64 OR BUILD_OS_WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()
if(CC_BC_MAC)
	set(BUNDLE_NAME "Creality_Print")
endif()
if(CC_BC_MAC)
	set(CMAKE_CXX_FLAGS "--sysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS}")	
endif()

set(CMAKE_CXX_STANDARD 14)
if (WIN32)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	add_definitions(-DWIN32DLL)
else()
    set(CMAKE_CXX_FLAGS "-fPIC")
    set(CMAKE_C_FLAGS "-fPIC")
endif()

set(QML_ENTRY_DIR ${CMAKE_SOURCE_DIR}/kernel/BasicKernel/res/qml/)

include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(CPACK_ORGANIZATION "Creality")

if(DEFINED PROJECT_VERSION_EXTRA)
	if(${PROJECT_VERSION_EXTRA} STREQUAL "Release")
		set(DUMPTOOL_OPTION_SCENE_FILE "false")
	endif()
endif()

__configure_build_info_header(
	customized
	dumptool
)

__enable_spdlog()

option(GEN_TRANSLATIONS_TS "create TS file" OFF)
option(BASICKERNEL_USE_UTEST "build BasicKernel utest" OFF)

add_subdirectory(msbase)
set(MODELMODULE_USE_UTEST OFF)
add_subdirectory(modelModule)
add_subdirectory(nestplacer)
add_subdirectory(crslice)
add_subdirectory(crslice2/crslice)
add_subdirectory(common_3mf)

add_subdirectory(cadcore)
add_subdirectory(topomesh)

__enable_qt5()
add_subdirectory(qtuser)
add_subdirectory(qtuser_3d)
add_subdirectory(shader_entity)

add_subdirectory(qcxffmpeg)
add_subdirectory(qcxchart)

add_subdirectory(cxcloud)
add_subdirectory(cxgcode)
add_subdirectory(cxkernel)

add_subdirectory(spread)
add_subdirectory(kernel)

set(Creative3D_plugins
	CrealityUI
	plugin_localnet
)

option(BUILD_LASER_PLUGIN "enable laser plugin" ON)
if(BUILD_LASER_PLUGIN)
	LIST(APPEND Creative3D_plugins plugin_laser)
endif()

option(BUILD_TOOL_PLUGIN "enable tools plugins" ON)
if(BUILD_TOOL_PLUGIN)
	LIST(APPEND Creative3D_plugins
		plugin_info
		plugin_letter
		plugin_pick_bottom
		plugin_split
		plugin_repairnew
		plugin_photo_mesh
		plugin_usb_print
		plugin_hollow
		plugin_drill
		plugin_distance_measure
		plugin_color
		plugin_support_painting
		plugin_seam_painting
		plugin_adaptive_layer
	)
endif()

add_subdirectory(plugins)
add_subdirectory(Creative3D)

if(BUILD_OS_WIN64 OR BUILD_OS_WIN32)
	add_subdirectory(dumptool)
endif()


set(UNIT_TEST_TOOL_ENABLED "true")
if(DEFINED PROJECT_VERSION_EXTRA)
	if(${PROJECT_VERSION_EXTRA} STREQUAL "Release")
		set(UNIT_TEST_TOOL_ENABLED "false")
	endif()
endif()
if(UNIT_TEST_TOOL_ENABLED)
	add_subdirectory(unittesttool)
endif()

add_subdirectory(scripts/cmake/)
