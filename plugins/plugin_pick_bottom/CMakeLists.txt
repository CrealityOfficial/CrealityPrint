file(GLOB SRCS *.h *.cpp)
set(LIBS ${QtQmlQ3dLibs} trimesh2 ${qtuser} BasicKernel mmesh qhullWrapper)

__add_real_target(plugin_pick_bottom dll SOURCE ${SRCS} LIB ${LIBS} FOLDER plugin)