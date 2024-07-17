set(LIBS BasicKernel)

file(GLOB SRCS *.h *.cpp)
add_kernel_plugin(adaptive_layer)
