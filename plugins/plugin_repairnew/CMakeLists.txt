
#add_definitions(-DUSE_CGAL_REPAIR_MESH)
add_definitions(-DCGAL_DISABLE_GMP)


file(GLOB SRCS *.h *.cpp)
set(LIBS ${QtQmlQ3dLibs} trimesh2 ${qtuser} mmesh cmesh BasicKernel OpenMP::OpenMP_CXX)

__add_real_target(plugin_repairnew dll SOURCE ${SRCS} ${REPAIR_SRCS} LIB ${LIBS} FOLDER plugin)