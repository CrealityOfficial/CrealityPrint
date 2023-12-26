set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
file(GLOB SRCS *.h *.cpp)

__cc_find(ovdbutil)
__assert_target(ovdbutil)

list(APPEND LIBS ovdbutil)
list(APPEND LIBS BasicKernel)
add_kernel_plugin(hollow)