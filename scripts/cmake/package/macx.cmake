set(App_Name Creality_Print.app)
set(App_Volumn_Name "Creality Print App")
set(App_Scpt CMakeDMGSetup.scpt)
SET(CPACK_GENERATOR "DragNDrop")
set(CPACK_SYSTEM_NAME "macx-${CMAKE_HOST_SYSTEM_PROCESSOR}") 
set(prefix "${App_Name}/Contents")
set(INSTALL_RUNTIME_DIR "${prefix}/MacOS")
set(INSTALL_CMAKE_DIR "${prefix}/Resources")
set(INSTALL_PLUGIN_DIR "${prefix}/PlugIns")
set(INSTALL_LIB_DIR "${prefix}/Frameworks")
macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
    get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
    if(EXISTS "${_qt_plugin_path}")
        get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
        get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
        get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
        set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
        install(FILES "${_qt_plugin_path}"
            DESTINATION "${_qt_plugin_dest}")
        set(${_qt_plugins_var}
            "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
    else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
    endif()
endmacro()

set(LIB_PUGINS)
install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" LIB_PUGINS ${prefix})

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    "[Paths]\nPlugins = ${_qt_plugin_dir}\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    DESTINATION "${INSTALL_CMAKE_DIR}")
install(FILES "${CMAKE_BINARY_DIR}/plugins/CrealityUI/CrealityUIRes.rcc"
    DESTINATION "${INSTALL_CMAKE_DIR}")
INSTALL(FILES  /usr/local/opt/libomp/lib/libomp.dylib DESTINATION "${MACOS_INSTALL_LIB_DIR}")

#INSTALL(TARGETS CrealityUI
#    BUNDLE DESTINATION . COMPONENT Runtime
#    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
#    FRAMEWORK DESTINATION ${INSTALL_CMAKE_DIR}/qml/CrealityUI
#    ARCHIVE DESTINATION ${INSTALL_CMAKE_DIR}/qml/CrealityUI
#    LIBRARY DESTINATION ${INSTALL_CMAKE_DIR}/qml/CrealityUI
#    )

# Append Qt's lib folder which is two levels above Qt5Widgets_DIR
include(InstallRequiredSystemLibraries)
            
#install(CODE "execute_process(COMMAND rm -R \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/resources\")")

install(CODE "execute_process(COMMAND codesign --timestamp --force --options=runtime -s \"${OSX_CODESIGN_IDENTITY}\" --deep \"\${CMAKE_INSTALL_PREFIX}/${BUNDLE_NAME}.app\")")
install(CODE "execute_process(COMMAND bash  ${CMAKE_CURRENT_SOURCE_DIR}/package/macx/Notarized-script.sh \${CMAKE_INSTALL_PREFIX} \"${BUNDLE_NAME}\" $ENV{MACPASS})")

#set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_FILE_NAME "${BUNDLE_NAME}-${PROJECT_VERSION}")
#set(CPACK_DMG_VOLUME_NAME "Creality Print App")
set(CPACK_DMG_VOLUME_NAME ${App_Volumn_Name})
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/package/macx/CMakeDMGBackground.tif")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/package/macx/VolumeIcon.icns")
#set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/package/macx/DS_Store")
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/package/macx/${App_Scpt}")
 # SET(CPACK_PACKAGE_ICON "${ICONS_DIR}/DMG.icns")
 # SET(CPACK_DMG_DS_STORE "${ICONS_DIR}/DMGDSStore")
 # SET(CPACK_DMG_BACKGROUND_IMAGE "${ICONS_DIR}/DMGBackground.png")

