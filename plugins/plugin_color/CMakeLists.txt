set(LIBS BasicKernel)

file(GLOB SRCS *.h *.cpp)
add_kernel_plugin(color)
