# MSVC writes /showIncludes in the console output code page. A background
# build may use the system code page even though Ninja uses UTF-8, silently
# losing header dependencies. Apply the same encoding to every compilation,
# including PCH generation, while retaining existing compiler launchers.
set(SLIC3R_MSVC_UTF8_LAUNCHER "")
if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_GENERATOR MATCHES "Ninja")
    execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" -t wincodepage
        OUTPUT_VARIABLE _slic3r_ninja_codepage
        ERROR_QUIET)
    if (_slic3r_ninja_codepage MATCHES "UTF-8")
        # CMake also chooses the encoding of msvc_deps_prefix from its console.
        # Keep configure and build consistent, including IDE background runs.
        execute_process(COMMAND cmd /d /c "chcp 65001 >nul"
            RESULT_VARIABLE _slic3r_codepage_result)
        if (NOT _slic3r_codepage_result EQUAL 0)
            message(FATAL_ERROR "Cannot select UTF-8 for MSVC/Ninja dependency tracking")
        endif ()
        set(SLIC3R_MSVC_UTF8_LAUNCHER cmd /d /c)
        # /c strips the first pair of quotes when the command starts with a
        # quoted script path. CALL preserves it for checkouts with spaces.
        if (CMAKE_CURRENT_LIST_DIR MATCHES "[ \t&()^]")
            list(APPEND SLIC3R_MSVC_UTF8_LAUNCHER call)
        endif ()
        list(APPEND SLIC3R_MSVC_UTF8_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/msvc_utf8.cmd")
        foreach (_slic3r_lang C CXX)
            set(CMAKE_${_slic3r_lang}_COMPILER_LAUNCHER
                ${SLIC3R_MSVC_UTF8_LAUNCHER} ${CMAKE_${_slic3r_lang}_COMPILER_LAUNCHER})
        endforeach ()
    endif ()
endif ()
