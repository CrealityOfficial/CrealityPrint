cmake_minimum_required (VERSION 3.1)


project(Creative3D LANGUAGES CXX C)
set(BUNDLE_NAME "Creality_Print")
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

set(CC_GLOBAL_PACKAGE_INSTALL 1)
if(BUILD_OS_WIN64 OR BUILD_OS_WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
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

add_subdirectory(msbase)

add_subdirectory(gcode)
add_subdirectory(crslice)
add_subdirectory(topomesh)

add_subdirectory(cadcore)

__enable_qt5()
add_subdirectory(qtuser)
add_subdirectory(qtuser_3d)
add_subdirectory(shader_entity)

add_subdirectory(qcxffmpeg)
add_subdirectory(qcxchart)

#add_subdirectory(cxcloud)
add_subdirectory(cxgcode)
add_subdirectory(cxkernel)

add_subdirectory(kernel)

set(Creative3D_plugins  CrealityUI
					    plugin_cx3d_exporter
					    plugin_info
					    plugin_letter
					    plugin_pick_bottom
						plugin_simple_support
						plugin_split
                        plugin_repairnew
						plugin_photo_mesh
						plugin_laser
						plugin_localnet
						plugin_usb_print
						plugin_hollow
						plugin_drill
						plugin_distance_measure
						)

add_subdirectory(plugins)

add_subdirectory(Creative3D)

if(BUILD_OS_WIN64 OR BUILD_OS_WIN32)
	add_subdirectory(dumptool)
endif()

add_subdirectory(scripts/cmake/)
