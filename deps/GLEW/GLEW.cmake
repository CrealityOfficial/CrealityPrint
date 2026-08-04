# We have to check for OpenGL to compile GLEW
set(OpenGL_GL_PREFERENCE "LEGACY") # to prevent a nasty warning by cmake
find_package(OpenGL QUIET REQUIRED)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_glew_dynamic_gl_backend ON)
else ()
    set(_glew_dynamic_gl_backend OFF)
endif ()

orcaslicer_add_cmake_project(
  GLEW
  SOURCE_DIR  ${CMAKE_CURRENT_LIST_DIR}/glew
  CMAKE_ARGS
    -DGLEW_USE_DYNAMIC_GL_BACKEND=${_glew_dynamic_gl_backend}
    -DGLEW_USE_EGL=OFF
)

if (MSVC)
    add_debug_dep(dep_GLEW)
endif ()
