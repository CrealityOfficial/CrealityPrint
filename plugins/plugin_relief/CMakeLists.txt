list(APPEND LIBS topomesh BasicKernel)

file(GLOB SRCS *.h *.cpp)
add_kernel_plugin(relief)
