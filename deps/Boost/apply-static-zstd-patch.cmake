if(NOT DEFINED BOOST_SOURCE_DIR)
    message(FATAL_ERROR "BOOST_SOURCE_DIR is required")
endif()

set(_iostreams_cmake "${BOOST_SOURCE_DIR}/libs/iostreams/CMakeLists.txt")
if(NOT EXISTS "${_iostreams_cmake}")
    message(FATAL_ERROR "Boost.Iostreams CMakeLists.txt not found: ${_iostreams_cmake}")
endif()

file(READ "${_iostreams_cmake}" _contents)

set(_shared_target
    "zstd_FOUND zstd::libzstd_shared src/zstd.cpp")
set(_static_target
    "zstd_FOUND zstd::libzstd_static src/zstd.cpp")

string(FIND "${_contents}" "${_static_target}" _static_target_pos)
if(NOT _static_target_pos EQUAL -1)
    message(STATUS "Boost.Iostreams already uses the static zstd target")
    return()
endif()

string(FIND "${_contents}" "${_shared_target}" _shared_target_pos)
if(_shared_target_pos EQUAL -1)
    message(FATAL_ERROR
        "Unsupported Boost.Iostreams CMakeLists.txt: expected zstd target was not found")
endif()

string(REPLACE "${_shared_target}" "${_static_target}" _contents "${_contents}")
file(WRITE "${_iostreams_cmake}" "${_contents}")
message(STATUS "Patched Boost.Iostreams to use zstd::libzstd_static")
