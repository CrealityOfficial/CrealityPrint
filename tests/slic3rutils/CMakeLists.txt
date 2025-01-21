get_filename_component(_TEST_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
# find_package(OpenSSL REQUIRED)
add_executable(${_TEST_NAME}_tests
    ${_TEST_NAME}_tests_main.cpp
    )

target_link_libraries(${_TEST_NAME}_tests test_common libslic3r_gui libslic3r)
if (MSVC)
    target_link_libraries(${_TEST_NAME}_tests Setupapi.lib)
endif ()
set_property(TARGET ${_TEST_NAME}_tests PROPERTY FOLDER "tests")

if (WIN32)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        crealityprint_copy_dlls(COPY_DLLS "Debug" "d" "")
    else()
        crealityprint_copy_dlls(COPY_DLLS "Release" "" "")
    endif()
endif()

# catch_discover_tests(${_TEST_NAME}_tests TEST_PREFIX "${_TEST_NAME}: ")
add_test(${_TEST_NAME}_tests ${_TEST_NAME}_tests "${CATCH_EXTRA_ARGS} exclude:[NotWorking]")
