SET(CMAKE_PREFIX_PATH $ENV{QT5_DIR})

macro(__enable_qt5)
	if(NOT TARGET Qt5::Core)
		set(CMAKE_AUTOMOC ON)
		set(CMAKE_AUTORCC ON)
	
		find_package(Qt5 COMPONENTS Core Widgets Gui Quick Qml Xml QuickWidgets 3DCore 3DRender 3DExtras 3DInput 3DLogic 3DQuick SerialPort Multimedia Concurrent OpenGL WebSockets LinguistTools Svg)
		
		if(TARGET Qt5::Core)
			include(qml)
			set(QT5_ENABLED 1)
			set(QT_VERSION_MAJOR 5)
		endif()
	endif()
endmacro()

macro(__enable_qt5_quick)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	find_package(Qt5 COMPONENTS Core Widgets Gui Quick Qml Xml SerialPort Multimedia Concurrent OpenGL)
	
	if(TARGET Qt5::Core)
		include(qml)
		set(QT5_ENABLED 1)
		set(QT_VERSION_MAJOR 5)
	endif()
endmacro()

macro(__target_copyqt_plugins target)
	if(Qt5Core_DIR)
		set(COPY_SOURCE_DLL_DIR "${Qt5Core_DIR}/../../../plugins/platforms/")
		add_custom_command(TARGET ${target} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
					${COPY_SOURCE_DLL_DIR}
					"$<TARGET_FILE_DIR:${target}>/platforms/"
					)
	else()
		message("Qt5Core_DIR not exits.  __enable_qt5 first")
	endif()
endmacro()

set(QtQmlQ3dLibs Qt5::Widgets Qt5::Quick Qt5::Qml Qt5::3DExtras Qt5::OpenGL)
set(QtQmlLibs Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Qml Qt5::SerialPort Qt5::Multimedia)
set(QtGuiLibs Qt5::Core Qt5::Gui Qt5::Widgets)

macro(__qt5_translate RES DIR)
	file(GLOB TS_FILES ${DIR}/*.ts)
	qt5_add_translation(QM_FILES ${TS_FILES})
	
	configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
	qt5_add_resources(${RES} ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
endmacro()

macro(__remap_qt_debug_2_release)
	set(REMAP_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Xml
		Qt5::Qml Qt5::Quick Qt5::3DCore Qt5::3DRender Qt5::3DCore Qt5::3DExtras
		Qt5::OpenGL Qt5::3DInput Qt5::3DLogic Qt5::3DQuick Qt5::SerialPort Qt5::Multimedia Qt5::Concurrent Qt5::Gamepad)
	
	__remap_target_debug_2_release(REMAP_LIBS)
endmacro()

macro(__deploy_target_qt target)
	include(DeployQt)
	__windeployqt(${target})
endmacro()

macro(__mac_deploy_target_qt target)
	include(DeployQt)
	__macdeployqt(${target})
endmacro()

macro(__linux_deploy_target_qt target)
	include(DeployQt)
	__linuxdeployqt(${target})
endmacro()