cmake_minimum_required(VERSION 3.15)
project(preset_sync_tests LANGUAGES CXX)
enable_testing()
find_package(Boost REQUIRED)
add_executable(preset_sync_tests preset_sync_tests.cpp)
target_compile_features(preset_sync_tests PRIVATE cxx_std_17)
target_compile_definitions(preset_sync_tests PRIVATE BOOST_ALL_NO_LIB)
target_include_directories(preset_sync_tests PRIVATE ../../src ${Boost_INCLUDE_DIRS})
if(MSVC)
    target_compile_options(preset_sync_tests PRIVATE /utf-8)
endif()
add_test(NAME preset_sync_tests COMMAND preset_sync_tests)

# Exercise the real sync functions without starting wxWidgets or reaching the cloud.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CURL REQUIRED)
set(SYNC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SYNC_FUNCTIONS "${CMAKE_CURRENT_BINARY_DIR}/sync_workflow_functions.inc")
add_custom_command(OUTPUT "${SYNC_FUNCTIONS}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/extract_sync_workflow.py"
        "${SYNC_SOURCE_ROOT}" "${SYNC_FUNCTIONS}"
    DEPENDS extract_sync_workflow.py
        "${SYNC_SOURCE_ROOT}/src/libslic3r/Preset.cpp"
        "${SYNC_SOURCE_ROOT}/src/slic3r/GUI/SyncUserPresets.cpp"
        "${SYNC_SOURCE_ROOT}/src/slic3r/GUI/CommunicateWithCXCloud.cpp"
    VERBATIM)
add_executable(sync_workflow_tests sync_workflow_tests.cpp "${SYNC_FUNCTIONS}")
target_compile_features(sync_workflow_tests PRIVATE cxx_std_17)
target_compile_definitions(sync_workflow_tests PRIVATE BOOST_ALL_NO_LIB)
target_include_directories(sync_workflow_tests PRIVATE
    "${SYNC_SOURCE_ROOT}/src" "${CMAKE_CURRENT_BINARY_DIR}" ${Boost_INCLUDE_DIRS})
target_link_libraries(sync_workflow_tests PRIVATE CURL::libcurl)
if(MSVC)
    target_compile_options(sync_workflow_tests PRIVATE /utf-8)
endif()
add_test(NAME sync_workflow_tests COMMAND sync_workflow_tests)
