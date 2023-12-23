__enable_cxx17()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

__cc_find(AliyunOss)
__cc_find(cpr)
__cc_find(curl)
__cc_find(libqrencode_static)
__cc_find(OssWrapper)
__cc_find(RapidJson)
__cc_find(boost_static)
__cc_find(qtuser)

__assert_target(qtuser_core)
__assert_target(cpr)

file(GLOB_RECURSE SRCS
  cxcloud/*.h
  cxcloud/*.hpp
  cxcloud/*.cpp
)

set(LIBS
  Qt5::Gui
  Qt5::Network
  Qt5::Quick

  AliyunOss
  boost_filesystem
  libqrencode_static
  ossWrapper
  rapidjson

  qtuser_core
)

set(ILIBS
  Qt5::Core
  
  cpr
  curl
)

set(DEFS
  BOOST_ALL_NO_LIB
  BOOST_ALL_DYN_LINK
)

set(INTERFACE_DEFS USE_CXCLOUD)

__add_common_library(cxcloud)

if(CC_INSTALL_CXCLOUD)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/cxcloud cxcloud)
endif()