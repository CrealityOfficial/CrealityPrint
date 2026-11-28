# Boost 1.84's Boost.Locale passes every header to
#   source_group(TREE ".../include/boost/locale" ...)
# including include/boost/locale.hpp, which is outside that tree. CMake 4.x
# rejects this with "source_group ... is not a prefix of file" and the Boost
# configure step fails. Keep only the headers inside include/boost/locale/ for
# that call; locale.hpp is grouped by the source_group() call that follows it.

if(NOT DEFINED BOOST_SOURCE_DIR)
    message(FATAL_ERROR "BOOST_SOURCE_DIR is required")
endif()

set(_locale_cmake "${BOOST_SOURCE_DIR}/libs/locale/CMakeLists.txt")
if(NOT EXISTS "${_locale_cmake}")
    message(FATAL_ERROR "Boost.Locale CMakeLists.txt not found: ${_locale_cmake}")
endif()

file(READ "${_locale_cmake}" _contents)

set(_tree_call
    "source_group(TREE \"\${CMAKE_CURRENT_SOURCE_DIR}/include/boost/locale\" PREFIX \"headers\" FILES \${sources_inc})")
set(_filter
    "list(FILTER sources_inc INCLUDE REGEX \"(^|/)include/boost/locale/\")")

string(FIND "${_contents}" "${_filter}" _filter_pos)
if(NOT _filter_pos EQUAL -1)
    message(STATUS "Boost.Locale source_group patch already applied")
    return()
endif()

string(FIND "${_contents}" "${_tree_call}" _tree_call_pos)
if(_tree_call_pos EQUAL -1)
    message(STATUS "Boost.Locale source_group call not found, nothing to patch")
    return()
endif()

string(REPLACE "${_tree_call}" "${_filter}\n${_tree_call}" _contents "${_contents}")
file(WRITE "${_locale_cmake}" "${_contents}")
