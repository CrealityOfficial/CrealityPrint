file(GLOB SRCS *.h *.cpp)

list(APPEND LIBS BasicKernel)
add_kernel_plugin(pick_bottom)