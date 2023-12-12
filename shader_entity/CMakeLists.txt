set(CMAKE_CXX_STANDARD 14)

if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()

__cc_find(TriMesh2)
__assert_target(trimesh2)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/entity/ SRCS1)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/renderpass/ SRCS2)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/effect/ SRCS3)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/utils/ SRCS4)

list(APPEND SRCS ${SRCS1} ${SRCS2} ${SRCS3} ${SRCS4} ${CMAKE_CURRENT_SOURCE_DIR}/shader_entity_export.h)

set(QRCS images.qrc)

set(ILIBS Qt5::3DRender Qt5::3DExtras Qt5::3DRenderPrivate Qt5::Qml Qt5::Quick trimesh2 qtuser_3d) 

__add_common_library(shader_entity FOLDER render QTQRC ${QRCS})



