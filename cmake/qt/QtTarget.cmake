set(Qt_User_Modules Core Widgets Gui Quick Qml Xml 3DCore 3DRender 3DExtras 3DInput 3DLogic 3DQuick SerialPort Concurrent OpenGL)
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{QT5_DIR})

macro(__setup_qt_targets)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	
	if(TARGET Qt5::Core)
		set(QT5_ENABLED 1)
		set(QT_VERSION_MAJOR 5)
	endif()
	if(TARGET Qt6::Core)
		set(QT6_ENABLED 1)
		set(QT_VERSION_MAJOR 6)
	endif()
	
	set(CC_QT_GUI Qt${QT_VERSION_MAJOR}::Core
				  Qt${QT_VERSION_MAJOR}::Gui
				  Qt${QT_VERSION_MAJOR}::Widgets
				  Qt${QT_VERSION_MAJOR}::OpenGL)  
	message(STATUS "QT ${QT_VERSION_MAJOR} setup.")
endmacro()

macro(__prepare_qt5)
	find_package(Qt5 COMPONENTS ${Qt_User_Modules})
	__setup_qt_targets()
endmacro()

macro(__prepare_core_qt5)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL Network)
	__setup_qt_targets()
endmacro()

macro(__assert_qt_module)
	foreach(module IN ITEMS ${ARGN})
		__assert_target(Qt${QT_VERSION_MAJOR}::${module})
	endforeach()
endmacro()