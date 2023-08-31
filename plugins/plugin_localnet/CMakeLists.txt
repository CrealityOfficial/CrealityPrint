file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

file(GLOB SRCS *.h *.cpp)
set(LIBS ${QtQmlQ3dLibs} ${qtuser} trimesh2 BasicKernel cxmdns curl cpr)

set(DEFS BOOST_ALL_NO_LIB)

__add_real_target(plugin_localnet dll SOURCE ${SRCS} ${RCC_SRC} LIB ${LIBS} DEF ${DEFS} INC ${INCS} FOLDER plugin)
