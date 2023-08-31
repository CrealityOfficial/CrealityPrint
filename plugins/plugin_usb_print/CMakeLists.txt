file(GLOB QRC_SRC res/*.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
__files_group(${CMAKE_CURRENT_SOURCE_DIR} SRCS)

find_package(Qt5 COMPONENTS Concurrent REQUIRED)
set(LIBS Qt5::Xml BasicKernel cserialport)
		
__add_real_target(plugin_usb_print dll SOURCE ${SRCS} ${RCC_SRC} LIB ${LIBS} DEF ${DEFS} FOLDER plugin)
