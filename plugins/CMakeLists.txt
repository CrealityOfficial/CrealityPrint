set(INCS)
set(LIBS BasicKernel)
set(QRCS)

macro(add_kernel_plugin plugin)
	__add_real_target(plugin_${plugin} dll
							SOURCE ${SRCS}
							LIB ${LIBS}
							SOURCE_FOLDER
							FOLDER plugin
							QTQRC ${QRCS}
							)	
endmacro()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins.in
               ${CMAKE_BINARY_DIR}/plugins.h)
				   
foreach(plugin ${Creative3D_plugins})
	add_subdirectory(${plugin})
endforeach()
