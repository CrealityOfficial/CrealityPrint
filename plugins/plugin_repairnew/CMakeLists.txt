file(GLOB SRCS *.h *.cpp)

__cc_find(cmesh_noigl)
__assert_target(cmesh_noigl)

list(APPEND LIBS cmesh_noigl)

add_kernel_plugin(repairnew)