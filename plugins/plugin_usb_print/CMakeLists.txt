file(GLOB QRC_SRC res/*.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
__files_group(${CMAKE_CURRENT_SOURCE_DIR} SRCS)

__cc_find(cserialport)
__assert_target(cserialport)

list(APPEND LIBS BasicKernel cserialport)
if(APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
    list(APPEND LIBS ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif()		
add_kernel_plugin(usb_print)
