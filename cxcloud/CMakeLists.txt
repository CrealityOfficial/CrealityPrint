__enable_cxx17()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

__cc_find(aliyunoss_static)
__cc_find(boost_static)
__cc_find(curl_static)
__cc_find(cpr_static)
__cc_find(libqrencode_static)
__cc_find(osswrapper_static)
__cc_find(rapidjson)
__cc_find(qtuser)

__assert_target(cpr_static)
__assert_target(qtuser_core)

file(GLOB_RECURSE SRCS
  cxcloud/*.h
  cxcloud/*.hpp
  cxcloud/*.cpp
  cxcloud/*.inl
)

set(LIBS
  Qt5::Gui
  Qt5::Network
  Qt5::Quick

  aliyunoss_static
  boost_filesystem
  curl_static
  cpr_static
  libqrencode_static
  osswrapper_static
  rapidjson
)

set(ILIBS
  Qt5::Core

  qtuser_core
)

set(DEFS
  BOOST_ALL_NO_LIB
)

set(INTERFACE_DEFS USE_CXCLOUD)

__add_common_library(cxcloud)

if(CC_INSTALL_CXCLOUD)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/cxcloud cxcloud)
endif()
