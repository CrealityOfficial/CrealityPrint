# glew target

if(NOT TARGET glew)
	__search_target_components(glew
							INC GL/glew.h
							DLIB glew
							LIB glew
							PRE glew
							)
	
	__test_import(glew lib)
endif()


set_property(TARGET glew PROPERTY INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC GLEW_NO_GLU)

find_package (OpenGL REQUIRED)
target_link_libraries(glew INTERFACE ${OPENGL_LIBRARIES})