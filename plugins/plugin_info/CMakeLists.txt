set(QRCS info.qrc)

set(SRCS infoplugin.h
		 infoplugin.cpp
		 infoprovider.h
		 infoprovider.cpp
		 zsliderinfo.h
		 zsliderinfo.cpp
		 )

add_kernel_plugin(info)
