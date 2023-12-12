# glew target

__conan_import(glew lib)
set_property(TARGET glew PROPERTY INTERFACE_COMPILE_DEFINITIONS GLEW_STATIC GLEW_NO_GLU)

find_package (OpenGL REQUIRED)
target_link_libraries(glew INTERFACE ${OPENGL_LIBRARIES})