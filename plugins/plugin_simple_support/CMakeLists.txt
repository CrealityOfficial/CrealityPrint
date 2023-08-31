file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
file(GLOB SRCS src/*.h src/*.cpp)
set(LIBS BasicKernel)

__add_real_target(plugin_simple_support dll SOURCE ${SRCS} ${RCC_SRC} LIB ${LIBS} FOLDER plugin)

