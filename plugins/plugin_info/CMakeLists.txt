set(QRC_SRC info.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC})

set(SRCS infoplugin.h
		 infoplugin.cpp
		 zsliderinfo.h
		 zsliderinfo.cpp
		 ${RCC_SRC}
		 )
set(LIBS BasicKernel)

__add_real_target(plugin_info dll SOURCE ${SRCS}
								  LIB ${LIBS}
								  FOLDER plugin
								  )
