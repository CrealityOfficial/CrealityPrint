file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

file(GLOB SRCS *.h *.cpp)
list(APPEND LIBS quazip_static)

add_kernel_plugin(cx3d_exporter)
