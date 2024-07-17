file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

file(GLOB SRCS *.h *.cpp)
list(APPEND SRCS ${RCC_SRC})
list(APPEND LIBS modelModule)

add_kernel_plugin(split)