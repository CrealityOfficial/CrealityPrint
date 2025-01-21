set(patch_command git init && ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/0001-fix-slicer-build.patch)
orcaslicer_add_cmake_project(OSS
  # GIT_REPOSITORY https://github.com/aliyun/aliyun-oss-cpp-sdk.git
  # GIT_TAG v1.9.2
  URL https://github.com/aliyun/aliyun-oss-cpp-sdk/archive/refs/tags/1.9.2.zip
  URL_HASH SHA256=7d94b9754a8dc5b64dcd7e9f729d37514f2c87f95a6154ab3bb2cf1103dc05c9
  PATCH_COMMAND ${patch_command}
)

add_dependencies(dep_OSS dep_CURL)

if (MSVC)
    add_debug_dep(dep_OSS)
endif ()