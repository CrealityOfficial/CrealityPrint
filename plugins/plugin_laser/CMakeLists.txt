file(GLOB SRCS *.h *.cpp)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(QPotrace)
add_subdirectory(laserclip)

find_package(Qt5 COMPONENTS Svg REQUIRED)
set(LIBS ${QtQmlQ3dLibs} ${qtuser} trimesh2 BasicKernel QPotrace laserclip Qt5::Svg fmesh clipper)
set(INCS )

__add_real_target(plugin_laser dll SOURCE ${SRCS} LIB ${LIBS} INC ${INCS} FOLDER plugin)
