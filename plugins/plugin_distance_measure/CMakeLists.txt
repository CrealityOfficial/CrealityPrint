
__files_group(${CMAKE_CURRENT_SOURCE_DIR} SRCS)

find_package(Qt5 COMPONENTS Widgets)

list(APPEND LIBS ${CLibs} BasicKernel Qt5::Gui Qt5::Widgets qcxutil)

__add_real_target(plugin_distance_measure dll SOURCE ${SRCS} LIB ${LIBS} FOLDER plugin)
