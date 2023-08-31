file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
file(GLOB SRCS *.h *.cpp)
set(LIBS BasicKernel cmesh fmesh)

__add_real_target(plugin_letter dll SOURCE ${SRCS} ${RCC_SRC} LIB ${LIBS} FOLDER plugin)
