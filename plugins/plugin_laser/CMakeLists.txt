file(GLOB SRCS *.h *.cpp)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(QPotrace)
add_subdirectory(laserclip)

find_package(Qt5 COMPONENTS Svg REQUIRED)
list(APPEND LIBS QPotrace laserclip Qt5::Svg)

add_kernel_plugin(laser)
