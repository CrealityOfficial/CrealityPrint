file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

file(GLOB SRCS *.h *.cpp)
list(APPEND LIBS BasicKernel cpr_static)

add_kernel_plugin(localnet)