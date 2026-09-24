cmake_minimum_required(VERSION 3.15)

# Can run independently without building the GUI or slicing engine.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(file_downloader_tests LANGUAGES CXX)
    enable_testing()
    find_package(Boost REQUIRED)
    set(ZLIB_USE_STATIC_LIBS ON)
    find_package(CURL REQUIRED)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(file_downloader_test_client
    client.cpp
    ../../src/slic3r/GUI/FileDownloader.cpp)
target_compile_features(file_downloader_test_client PRIVATE cxx_std_17)
target_include_directories(file_downloader_test_client PRIVATE ../../src ${Boost_INCLUDE_DIRS})
target_compile_definitions(file_downloader_test_client PRIVATE BOOST_ALL_NO_LIB)
target_link_libraries(file_downloader_test_client PRIVATE CURL::libcurl)
if(MSVC)
    target_compile_options(file_downloader_test_client PRIVATE /utf-8)
endif()
add_test(NAME file_downloader_regression
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/regression.py
        $<TARGET_FILE:file_downloader_test_client>)
set_tests_properties(file_downloader_regression PROPERTIES TIMEOUT 90)
