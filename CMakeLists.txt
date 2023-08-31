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

__enable_qt5()
__enable_spdlog()
__enable_openmp()

__cc_find(QhullWrapper)
__cc_find(Clipper)
__cc_find(Clipper3r)

__cc_find(OssWrapper)
__cc_find(AliyunOss)

__cc_find(tbb)

__cc_find(CXbin)
__cc_find(NestPlacer)
__cc_find(Quazip)
__cc_find(sensors_analytics_sdk)
__cc_find(SplitSlot)
__cc_find(enchase)
__cc_find(libqrencode)
__cc_find(cpr)
__cc_find(ovdbutil)

__cc_find(Eigen)

__cc_find(opencascade)
__cc_find(freetype)
__cc_find(admesh)

option(GEN_TRANSLATIONS_TS "create TS file" OFF)

if(CC_BC_MAC)
     add_definitions(-DNOUSE_IGL)
endif()

add_subdirectory(cxnd)
add_subdirectory(mmesh)
add_subdirectory(qtuser)

add_subdirectory(slic3rBase)

add_subdirectory(crcommon)
add_subdirectory(crslice)

add_subdirectory(cxmdns)
add_subdirectory(qcxffmpeg)
add_subdirectory(qcxchart)

add_subdirectory(topomesh)
add_subdirectory(QCXUtil)
add_subdirectory(gcode)

if(EXISTS cxcloud)
	add_subdirectory(cxcloud)
endif()

add_subdirectory(cxgcode)
add_subdirectory(CADCore)
add_subdirectory(cxkernel)

add_subdirectory(cxutil)

set(NPLACER_SUPPORT_LIBS clipperz libnest2d nestplacer nlopt)

add_subdirectory(kernel)
add_subdirectory(cserialport)
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
