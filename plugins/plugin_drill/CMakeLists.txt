file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
file(GLOB SRCS *.h *.cpp)

add_kernel_plugin(drill)
