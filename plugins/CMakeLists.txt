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

foreach(plugin ${Creative3D_plugins})
	add_subdirectory(${plugin})
endforeach()
