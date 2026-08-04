cmake_minimum_required(VERSION 3.0)
project(GLEW)

find_package(OpenGL REQUIRED)

# Allow parent project to control EGL usage. Default to OFF because the X11
# backend uses GLX contexts, and GLEW must use glXGetProcAddressARB there.
# Loading functions through EGL while wxGLCanvas uses GLX can leave OpenGL and
# ImGui rendering blank.
option(GLEW_USE_DYNAMIC_GL_BACKEND "Choose EGL or GLX function loading at runtime" OFF)
option(GLEW_USE_EGL "Use EGL instead of GLX for OpenGL function loading" OFF)

if(GLEW_USE_DYNAMIC_GL_BACKEND)
    if(NOT OpenGL_EGL_FOUND)
        message(FATAL_ERROR "GLEW_USE_DYNAMIC_GL_BACKEND requires EGL support")
    endif()
    message(STATUS "Building GLEW with runtime EGL/GLX support")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLEW_DYNAMIC_GL_BACKEND")
elseif(GLEW_USE_EGL)
    message(STATUS "Building GLEW with EGL support")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGLEW_EGL")
else()
    message(STATUS "Building GLEW with GLX support")
endif()

add_library(GLEW src/glew.c)
target_include_directories(GLEW PRIVATE include/)
target_link_libraries(GLEW PUBLIC OpenGL::GL)
if(GLEW_USE_DYNAMIC_GL_BACKEND)
    target_link_libraries(GLEW PUBLIC OpenGL::EGL)
endif()

if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(GLEW PUBLIC GLEW_STATIC)
endif ()

include(GNUInstallDirs)

install( 
    FILES
        ${PROJECT_SOURCE_DIR}/include/GL/glew.h
        ${PROJECT_SOURCE_DIR}/include/GL/wglew.h
        ${PROJECT_SOURCE_DIR}/include/GL/glxew.h
        ${PROJECT_SOURCE_DIR}/include/GL/eglew.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/GL
)

install(TARGETS GLEW GLEW
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)