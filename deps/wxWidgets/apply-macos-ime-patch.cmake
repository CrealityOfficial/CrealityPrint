if(NOT DEFINED wx_SOURCE_DIR)
    message(FATAL_ERROR "wx_SOURCE_DIR is required")
endif()

if(NOT DEFINED wx_IME_PATCH_FILE)
    message(FATAL_ERROR "wx_IME_PATCH_FILE is required")
endif()

if(NOT DEFINED wx_COCOA_INIT_PATCH_FILE)
    message(FATAL_ERROR "wx_COCOA_INIT_PATCH_FILE is required")
endif()

set(_glcanvas_mm "${wx_SOURCE_DIR}/src/osx/cocoa/glcanvas.mm")
file(READ "${_glcanvas_mm}" _glcanvas_content)
execute_process(
    COMMAND git init
    WORKING_DIRECTORY "${wx_SOURCE_DIR}"
    RESULT_VARIABLE _git_init_result
)
if(NOT _git_init_result EQUAL 0)
    message(FATAL_ERROR "Failed to initialize temporary git metadata for wxWidgets patch")
endif()

if(_glcanvas_content MATCHES "wxNSCustomOpenGLView : NSOpenGLView <NSTextInputClient>")
    message(STATUS "wxWidgets macOS GLCanvas IME patch already applied")
else()
    execute_process(
        COMMAND git apply --verbose --ignore-space-change --whitespace=fix "${wx_IME_PATCH_FILE}"
        WORKING_DIRECTORY "${wx_SOURCE_DIR}"
        RESULT_VARIABLE _ime_patch_result
    )
    if(NOT _ime_patch_result EQUAL 0)
        message(FATAL_ERROR "Failed to apply wxWidgets macOS GLCanvas IME patch")
    endif()
endif()

set(_appunix_cpp "${wx_SOURCE_DIR}/src/unix/appunix.cpp")
file(READ "${_appunix_cpp}" _appunix_content)
if(_appunix_content MATCHES "if \\( !IsGUI\\(\\) \\)")
    message(STATUS "wxWidgets macOS deferred Cocoa initialization patch already applied")
else()
    execute_process(
        COMMAND git apply --verbose --ignore-space-change --whitespace=fix "${wx_COCOA_INIT_PATCH_FILE}"
        WORKING_DIRECTORY "${wx_SOURCE_DIR}"
        RESULT_VARIABLE _cocoa_init_patch_result
    )
    if(NOT _cocoa_init_patch_result EQUAL 0)
        message(FATAL_ERROR "Failed to apply wxWidgets macOS deferred Cocoa initialization patch")
    endif()
endif()
