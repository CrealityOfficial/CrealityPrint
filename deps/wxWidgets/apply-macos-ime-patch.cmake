if(NOT DEFINED wx_SOURCE_DIR)
    message(FATAL_ERROR "wx_SOURCE_DIR is required")
endif()

if(NOT DEFINED wx_PATCH_FILE)
    message(FATAL_ERROR "wx_PATCH_FILE is required")
endif()

set(_glcanvas_mm "${wx_SOURCE_DIR}/src/osx/cocoa/glcanvas.mm")
file(READ "${_glcanvas_mm}" _glcanvas_content)
if(_glcanvas_content MATCHES "wxNSCustomOpenGLView : NSOpenGLView <NSTextInputClient>")
    message(STATUS "wxWidgets macOS GLCanvas IME patch already applied")
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
    COMMAND git apply --verbose --ignore-space-change --whitespace=fix "${wx_PATCH_FILE}"
    WORKING_DIRECTORY "${wx_SOURCE_DIR}"
    RESULT_VARIABLE _patch_result
)
if(NOT _patch_result EQUAL 0)
    message(FATAL_ERROR "Failed to apply wxWidgets macOS GLCanvas IME patch")
endif()
