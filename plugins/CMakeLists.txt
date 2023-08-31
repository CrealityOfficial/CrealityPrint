set(INCS)
set(LIBS ${QtQmlQ3dLibs} BasicKernel )

macro(add_kernel_plugin plugin)
	__add_real_target(plugin_${plugin} dll
							SOURCE ${SRCS}
							LIB ${LIBS}
							SOURCE_FOLDER
							FOLDER plugin
							)	
endmacro()

foreach(plugin ${Creative3D_plugins})
	add_subdirectory(${plugin})
endforeach()
