if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()

__cc_find(glew)
__assert_target(glew)
set(SRCS include/qcustomplot.h
		 include/qcxchart.h
		 src/qcustomplot.cpp
		 src/qcxchart.cpp
		 )

set(DEFS QCXCHART_DLL QCUSTOMPLOT_USE_OPENGL)
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set(LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Quick Qt5::PrintSupport glew)

find_package(Qt5 COMPONENTS PrintSupport REQUIRED)
__add_real_target(qcxchart dll SOURCE ${SRCS}
								DEF ${DEFS}
								INC ${INCS}
								LIB ${LIBS}
								INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
								)
								
#if(CC_INSTALL_QCXCHART)
#	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/include/ qcxchart)
#endif()
			
