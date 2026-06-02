#set(patch_command git init && ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/0001-fix-slicer-build.patch)
set(_build_static ON)
set(_build_with_ssl ON)
# Build paho WITHOUT SSL on Linux: otherwise paho compiles against the system
# OpenSSL 3.x headers but links the static OpenSSL 1.1.1 from ./deps, an ABI
# mismatch. Use "UNIX AND NOT APPLE" because the LINUX variable only exists in
# CMake >= 3.25 (the 22.04 image had 3.22, where if(LINUX) silently never fired).
if(UNIX AND NOT APPLE)
    set(_build_with_ssl OFF)
endif()

if (IN_GIT_REPO)
    set(MQTT_DIRECTORY_FLAG --directory ${BINARY_DIR_REL}/dep_MQTT-prefix/src/dep_MQTT)
endif ()

orcaslicer_add_cmake_project(MQTTC
  # GIT_REPOSITORY https://github.com/aliyun/aliyun-oss-cpp-sdk.git
  # GIT_TAG v1.9.2
  URL https://github.com/eclipse-paho/paho.mqtt.c/archive/refs/tags/v1.3.15.tar.gz
  URL_HASH SHA256=60ce2cfdc146fcb81c621cb8b45874d2eb1d4693105d048f60e31b8f3468be90
  CMAKE_ARGS
    -DPAHO_BUILD_STATIC=${_build_static} 
    -DPAHO_WITH_SSL=${_build_with_ssl} 
  #PATCH_COMMAND ${patch_command}
)
orcaslicer_add_cmake_project(MQTT
  # GIT_REPOSITORY https://github.com/aliyun/aliyun-oss-cpp-sdk.git
  # GIT_TAG v1.9.2
  URL https://github.com/eclipse-paho/paho.mqtt.cpp/archive/refs/tags/v1.4.0.zip
  URL_HASH SHA256=c165960f64322de21697eb06efdca3d74cce90f45ff5ff0efdd968708e13ba0c
  PATCH_COMMAND git apply ${MQTT_DIRECTORY_FLAG} --verbose --ignore-space-change --whitespace=fix ${CMAKE_CURRENT_LIST_DIR}/0001-openssl.patch
  CMAKE_ARGS
    -DPAHO_BUILD_STATIC=${_build_static} 
    -DPAHO_WITH_SSL=${_build_with_ssl}  
  #PATCH_COMMAND ${patch_command}
)

add_dependencies(dep_MQTT dep_MQTTC dep_CURL)

if (MSVC)
    add_debug_dep(dep_MQTT)
endif ()