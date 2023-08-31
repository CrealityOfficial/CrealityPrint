find_package(Qt5 COMPONENTS Core SerialPort LinguistTools Svg WebSockets REQUIRED)

if(WIN32)
	#message(STATUS "Using windres")
if(NOT RES_FILES)
	set(RES_FILES "Creative3D.rc")
endif()
	ENABLE_LANGUAGE(RC)
	if(NOT MSVC)
		SET(CMAKE_RC_COMPILER_INIT windres)
		SET(CMAKE_RC_COMPILE_OBJECT
			"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>"
			)
	endif()
endif(WIN32)

set(SRCS src/main.cpp
		 )
		 
if (WIN32)
    list(APPEND SRCS ${RES_FILES})
endif(WIN32)

if(NOT TS_FILES)
	SET(TS_FILES
	../locales/en.ts
	../locales/zh_CN.ts
	../locales/zh_TW.ts
	)
endif()
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/res/)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scence3d.qrc ${CMAKE_CURRENT_BINARY_DIR}/scence3d.qrc COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/main.qml ${CMAKE_CURRENT_BINARY_DIR}/res/main.qml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/MainWindow.qml ${CMAKE_CURRENT_BINARY_DIR}/res/MainWindow.qml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/Creative3D.ico ${CMAKE_CURRENT_BINARY_DIR}/res/Creative3D.ico COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/splashLogo.png ${CMAKE_CURRENT_BINARY_DIR}/res/splashLogo.png COPYONLY)
if(SPLASH_URL)
	configure_file(${SPLASH_URL} ${CMAKE_CURRENT_BINARY_DIR}/res/splash.png COPYONLY)
else()
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/splash.png ${CMAKE_CURRENT_BINARY_DIR}/res/splash.png COPYONLY)
endif()
if(LOGO_URL)
	configure_file(${LOGO_URL} ${CMAKE_CURRENT_BINARY_DIR}/res/logo.png COPYONLY)
else()
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/logo.png ${CMAKE_CURRENT_BINARY_DIR}/res/logo.png COPYONLY)
endif()

qt5_add_resources(QT_QRC ${CMAKE_CURRENT_BINARY_DIR}/scence3d.qrc)

find_package(Qt5QuickCompiler)
qtquick_compiler_add_resources(RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/scence3d.qrc)

if(GEN_TRANSLATIONS_TS)
    message(STATUS "GEN_TRANSLATIONS_TS")
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR}/plugins ${CMAKE_SOURCE_DIR}/kernel ${CMAKE_SOURCE_DIR}/Creative3D ${TS_FILES})
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
endif()

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE Creative3D.icns)
	if(NOT myApp_ICON)
    	set(myApp_ICON ${CMAKE_CURRENT_SOURCE_DIR}/Creative3D.icns)
	endif()
    set_source_files_properties(${myApp_ICON} PROPERTIES
           MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND SRCS ${myApp_ICON})
endif()

configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
qt5_add_resources(MAIN_QRC ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)

if(CC_BC_LINUX)
	set(AppDir ${CMAKE_INSTALL_PREFIX})
	
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/default.desktop DESTINATION ${AppDir})
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/default.png DESTINATION ${AppDir})
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources" DESTINATION ${AppDir})
	#install openssl
	install(FILES /lib/x86_64-linux-gnu/libssl.so.1.1 DESTINATION ${AppDir}/lib)
	install(FILES  /lib/x86_64-linux-gnu/libcrypto.so.1.1 DESTINATION ${AppDir}/lib)
endif()


set(LIBS BasicKernel cxkernel Qt5::Svg Qt5::WebSockets)
if(CC_BC_LINUX)
list(APPEND LIBS -pthread)
endif()
__add_real_target(${PROJECT_NAME} exe SOURCE ${SRCS} ${MAIN_QRC} ${QT_QRC}
											  LIB ${LIBS} 
											  INC ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
											  MAC_OUTPUT_NAME ${BUNDLE_NAME}
											  MAC_GUI_IDENTIFIER com.creality.crealityprint
											  QML_PLUGINS CrealityUI
											  DEPLOYQT
											  )

list(APPEND QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../plugins")
set(QML_IMPORT_PATH ${QML_IMPORT_PATH}
    CACHE STRING "Qt Creator extra qml import paths"
    FORCE
)

	
if(CC_BC_WIN)	
	target_sources(${PROJECT_NAME} PRIVATE dpi-aware.manifest)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME}
             PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
	endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    			COMMAND ${CMAKE_COMMAND} -E copy_directory
    			${CMAKE_SOURCE_DIR}/resources/
    			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/"
    			)
elseif(CC_BC_MAC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources/
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/resources/"
            )
endif()

foreach(plugin ${Creative3D_plugins})
	add_dependencies(${PROJECT_NAME} ${plugin})
endforeach()
