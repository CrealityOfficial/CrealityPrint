set(QRCS info.qrc)
set(LIBS BasicKernel)
file(GLOB SRCS *.h *.cpp)
add_kernel_plugin(info)
