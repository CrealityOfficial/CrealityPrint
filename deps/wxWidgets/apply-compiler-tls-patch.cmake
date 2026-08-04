if(NOT DEFINED wx_SOURCE_DIR)
    message(FATAL_ERROR "wx_SOURCE_DIR is required")
endif()

if(NOT DEFINED wx_PATCH_FILE)
    message(FATAL_ERROR "wx_PATCH_FILE is required")
endif()

set(_setup_h "${wx_SOURCE_DIR}/build/cmake/setup.h.in")
set(_patch_file "${wx_PATCH_FILE}")

file(READ "${_setup_h}" _setup_h_content)
if(_setup_h_content MATCHES "wxUSE_COMPILER_TLS")
    message(STATUS "wxWidgets compiler TLS setup patch already applied")
    return()
endif()

execute_process(
    COMMAND git init
    WORKING_DIRECTORY "${wx_SOURCE_DIR}"
    RESULT_VARIABLE _git_init_result
)
if(NOT _git_init_result EQUAL 0)
    message(FATAL_ERROR "Failed to initialize temporary git metadata for wxWidgets patch")
endif()

execute_process(
    COMMAND git apply --verbose --ignore-space-change --whitespace=fix "${_patch_file}"
    WORKING_DIRECTORY "${wx_SOURCE_DIR}"
    RESULT_VARIABLE _patch_result
)
if(NOT _patch_result EQUAL 0)
    message(FATAL_ERROR "Failed to apply wxWidgets compiler TLS setup patch")
endif()
