set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT TS_FILES)
	SET(TS_FILES
    ../locales/en.ts
    ../locales/zh_CN.ts
    ../locales/zh_TW.ts
	)
endif()

qt5_add_translation(QM_FILES ${TS_FILES})

configure_file(resource.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(main.qml ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

set(SRCS
  main.cpp
  dumptool.h
  dumptool.cpp
  wizardui.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/resource.qrc
  ../plugins/CrealityUI/qml.qrc
)

set(LIBS
  Qt5::Core
  Qt5::Quick
  Qt5::Qml
  Qt5::OpenGL
  AliyunOss
  rapidjson
  zlib
  quazip
  boost_filesystem
)

set(DEFS
  BOOST_ALL_NO_LIB
)

__add_real_target(dumptool exe
  SOURCE ${SRCS}
  LIB ${LIBS}
  DEF ${DEFS}
)