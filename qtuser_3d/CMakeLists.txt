set(CMAKE_CXX_STANDARD 14)

set(BUILD_SHADER_TOOLS 1)
if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()

__cc_find(TriMesh2)
__assert_target(trimesh2)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/qtuser3d/ SRCS)

set(LIBS)
set(ILIBS Qt5::3DRender Qt5::3DExtras Qt5::3DRenderPrivate Qt5::Qml Qt5::Quick trimesh2) 

find_package(OpenGL)
if(TARGET OpenGL::GL)
	list(APPEND LIBS OpenGL::GL)
endif()

__add_common_library(qtuser_3d FOLDER render)

if(CC_INSTALL_QTUSER3D)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/ qtuser_3d)
endif()

if(CC_BC_WIN AND BUILD_SHADER_TOOLS)
	add_subdirectory(tools)
endif()


# if(Python3_FOUND)
# 	set(Script ${PYTHON_MODULES}/ShaderBinarization.py)

# 	add_custom_command(	
# 		TARGET qtuser_3d PRE_BUILD
# 		COMMAND "${Python3_EXECUTABLE}" ${Script} ${CMAKE_BINARY_DIR}
# 	)
# endif()

