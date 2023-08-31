file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

file(GLOB SRCS src/*.h src/*.cpp)
set(LIBS ${QtQmlQ3dLibs} Qt5::Xml trimesh2 ${qtuser} BasicKernel quazip zlib mmesh)

__add_real_target(plugin_cx3d_exporter dll SOURCE ${SRCS} ${RCC_SRC} LIB ${LIBS} FOLDER plugin)
