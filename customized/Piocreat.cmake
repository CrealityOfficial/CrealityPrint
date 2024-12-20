message(STATUS "build Piocreat3D project")

AppendCustomizedMarco(CUSTOM_MACHINE_LIST)
AppendCustomizedMarco(CUSTOM_KEY)
AppendCustomizedMarco(CUSTOM_MATERIAL_LIST)
AppendCustomizedMarco(CUSTOM_UN_UPGRADEABLE)
AppendCustomizedMarco(CUSTOM_UN_FEEDBACKABLE)
AppendCustomizedMarco(CUSTOM_UN_TEACHABLE)

set(PROJECT_NAME "Piocreat3D")
#set(CPACK_ORGANIZATION "Piocreat")
set(BUNDLE_NAME "Piocreat Slicer")
#set(DESKTOP_LINK_NAME "Piocreat Slicer")
set(myApp_ICON ${CMAKE_SOURCE_DIR}/customized/Piocreat/Creative3D.icns)
set(RES_FILES ${CMAKE_SOURCE_DIR}/customized/Piocreat/Creative3D.rc)
set(PACKAGE_CONF ${CMAKE_SOURCE_DIR}/customized/Piocreat/Package.cmake)
set(SPLASH_URL ${CMAKE_SOURCE_DIR}/customized/Piocreat/splash.png)
set(LOGO_URL ${CMAKE_SOURCE_DIR}/customized/Piocreat/logo.png)
set(MACHINE_LIST ${CMAKE_SOURCE_DIR}/customized/Piocreat/machineList_custom.json)
set(MATERIAL_LIST ${CMAKE_SOURCE_DIR}/customized/Piocreat/materials_custom.json)

set(PROFILE_KEY ${CMAKE_SOURCE_DIR}/customized/Piocreat/keys_custom)
set(PROFILE_KEY_SHELL ${CMAKE_SOURCE_DIR}/customized/Piocreat/keys-shell_custom)
set(TS_FILES
  ${CMAKE_SOURCE_DIR}/locales/en.ts
  ${CMAKE_SOURCE_DIR}/locales/zh_CN.ts
  ${CMAKE_SOURCE_DIR}/locales/zh_TW.ts
)
