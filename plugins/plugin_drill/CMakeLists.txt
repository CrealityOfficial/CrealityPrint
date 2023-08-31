set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB QRC_SRC *.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})
file(GLOB SRCS *.h *.cpp)

list(APPEND LIBS ${CLibs} BasicKernel qhullWrapper)

set(DEFS _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS _CRT_SECURE_NO_WARNINGS)
__add_real_target(plugin_drill dll SOURCE ${SRCS} ${RCC_SRC}
									LIB ${LIBS}
									INC ${vcg_INCLUDE_DIRS} ${eigen_INCLUDE_DIRS}
									DEF ${DEFS}
									FOLDER plugin
									)


if(WIN32)
	set_target_properties( plugin_drill PROPERTIES COMPILE_FLAGS "/EHa" )
endif(WIN32)
