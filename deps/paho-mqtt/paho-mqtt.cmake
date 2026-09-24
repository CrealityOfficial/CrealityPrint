#set(patch_command git init && ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/0001-fix-slicer-build.patch)
set(_build_static ON)
# Paho C 1.3.15 does not export its static CMake targets when shared and
# static libraries are enabled together. Keep the C and C++ builds aligned.
set(_build_shared OFF)
set(_build_with_ssl ON)
# Build paho without SSL on Linux: older CMake versions do not define LINUX,
# so use the portable UNIX/APPLE split.
if(UNIX AND NOT APPLE)
    set(_build_with_ssl OFF)
endif()

set(_paho_openssl_args "")
if(APPLE)
  # Keep Paho's compile-time OpenSSL headers in sync with the bundled static
  # libraries used by the final application.  Otherwise a Homebrew OpenSSL 3
  # installation may be detected while Paho is built, producing references
  # such as SSL_get1_peer_certificate that OpenSSL 1.1.1 does not export.
  set(_paho_openssl_args
    -DOPENSSL_ROOT_DIR=${DESTDIR}
    -DOPENSSL_USE_STATIC_LIBS=TRUE
    -DOPENSSL_INCLUDE_DIR=${DESTDIR}/include
    -DOPENSSL_SSL_LIBRARY=${DESTDIR}/lib/libssl.a
    -DOPENSSL_CRYPTO_LIBRARY=${DESTDIR}/lib/libcrypto.a
  )
endif()

if (IN_GIT_REPO)
    set(MQTT_DIRECTORY_FLAG --directory ${BINARY_DIR_REL}/dep_MQTT-prefix/src/dep_MQTT)
endif ()

orcaslicer_add_cmake_project(MQTTC
  # GIT_REPOSITORY https://github.com/aliyun/aliyun-oss-cpp-sdk.git
  # GIT_TAG v1.9.2
  URL https://github.com/eclipse-paho/paho.mqtt.c/archive/refs/tags/v1.3.15.tar.gz
  URL_HASH SHA256=60ce2cfdc146fcb81c621cb8b45874d2eb1d4693105d048f60e31b8f3468be90
  DEPENDS ${OPENSSL_PKG}
  CMAKE_ARGS
    -DPAHO_BUILD_STATIC=${_build_static}
    -DPAHO_BUILD_SHARED=${_build_shared}
    -DPAHO_WITH_SSL=${_build_with_ssl}  
    -Declipse-paho-mqtt-c_DIR:PATH=${DESTDIR}/lib/cmake/eclipse-paho-mqtt-c
    ${_paho_openssl_args}
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
    -DPAHO_BUILD_SHARED=${_build_shared}
    -DPAHO_WITH_SSL=${_build_with_ssl}  
  #PATCH_COMMAND ${patch_command}
)

add_dependencies(dep_MQTT dep_MQTTC dep_CURL)

if (MSVC)
    add_debug_dep(dep_MQTT)
endif ()
