function(__sub_build_info_line)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n" PARENT_SCOPE)
endfunction()

function(__sub_build_info_text text)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n${text}" PARENT_SCOPE)
endfunction()

function(__sub_build_info_commit commit_message)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}// ${commit_message}" PARENT_SCOPE)
endfunction()

function(__sub_build_info_macro macro_name)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n#define ${macro_name}" PARENT_SCOPE)
endfunction()

function(__sub_build_info_boolean macro_name boolean_value)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n#define ${macro_name} ${boolean_value}" PARENT_SCOPE)
endfunction()

function(__sub_build_info_number macro_name number_value)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n#define ${macro_name} ${number_value}" PARENT_SCOPE)
endfunction()

function(__sub_build_info_string macro_name string_value)
  set(__SUB_BUILD_INFO_DEFINE "${__SUB_BUILD_INFO_DEFINE}\n#define ${macro_name} \"${string_value}\"" PARENT_SCOPE)
endfunction()

macro(__sub_build_info_begin_group group_name)
  __sub_build_info_line()
  __sub_build_info_commit("${group_name} [begin]")
endmacro()

macro(__sub_build_info_end_group group_name)
  __sub_build_info_line()
  __sub_build_info_commit("${group_name} [end]")
  __sub_build_info_line()
endmacro()

macro(__configure_build_info_header)
  set(__SUB_BUILD_INFO_DEFINE "")
  foreach(dir ${ARGV})
    set(__SUB_BUILD_INFO_DIR ${CMAKE_SOURCE_DIR}/${dir})
    __sub_build_info_begin_group(${dir})
    if(EXISTS ${__SUB_BUILD_INFO_DIR}/module_info.cmake)
      include(${__SUB_BUILD_INFO_DIR}/module_info.cmake)
    endif()
    if(EXISTS ${__SUB_BUILD_INFO_DIR}/build_info.cmake)
      include(${__SUB_BUILD_INFO_DIR}/build_info.cmake)
    endif()
    __sub_build_info_end_group(${dir})
  endforeach()

  if(NOT APP_TYPE)
    set(APP_TYPE 0)
  endif()

  string(TIMESTAMP BUILD_TIME "%y-%m-%d %H:%M")
	set(BUILD_INFO_HEAD "BUILDINFO_H_  // ${PROJECT_NAME} ${BUILD_TIME}")
	set(DEBUG_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Debug/resources/")
	set(RELEASE_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Release/resources/")

	__get_main_git_hash(MAIN_GIT_HASH)

  set(SHIPPING_LEVEL 2)
  if(${PROJECT_VERSION_EXTRA} STREQUAL "Release")
	set(SHIPPING_LEVEL 2)
  elseif(${PROJECT_VERSION_EXTRA} STREQUAL "Beta")
	set(SHIPPING_LEVEL 1)
  elseif(${PROJECT_VERSION_EXTRA} STREQUAL "Alpha")
	set(SHIPPING_LEVEL 0)
  else()
	set(SHIPPING_LEVEL 0)
  endif()
  
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.h.in)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.h.in
                   ${CMAKE_BINARY_DIR}/buildinfo.h)
  endif()
endmacro()

# old
macro(__build_info_header)
  if(NOT APP_TYPE)
    set(APP_TYPE 0)
  endif()

  string(TIMESTAMP BUILD_TIME "%y-%m-%d %H:%M")
  set(BUILD_INFO_HEAD "BUILDINFO_H_  // ${PROJECT_NAME} ${BUILD_TIME}")
  set(DEBUG_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Debug/resources/")
  set(RELEASE_RESOURCES_DIR "${BIN_OUTPUT_DIR}/Release/resources/")

  __get_main_git_hash(MAIN_GIT_HASH)

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.h.prebuild)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.h.prebuild
                   ${CMAKE_BINARY_DIR}/buildinfo.h)
  endif()
endmacro()
