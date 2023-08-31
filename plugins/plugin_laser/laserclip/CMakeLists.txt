__files_group(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)
set(LIBS)
set(DEFS)


if(BUILD_TEST)
list(APPEND SRC debug/debug_print.c debug/debug_print.h)
endif()

add_definitions(-DBUILD_LIBRARY_GCORE_GEN)
add_definitions(-DONLY_SUPPORT_MARLIN_PARSER)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
__add_real_target(laserclip lib SOURCE ${SRC} LIB ${LIBS} DEF ${DEFS} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
