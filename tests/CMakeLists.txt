# TODO Add individual tests as executables in separate directories
# add_subirectory(<testcase>)
message("lisugui good")
set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
file(TO_NATIVE_PATH "${TEST_DATA_DIR}" TEST_DATA_DIR)

add_library(Catch2 INTERFACE)
list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules/Catch2)
target_include_directories(Catch2 INTERFACE ${CMAKE_CURRENT_LIST_DIR})
add_library(Catch2::Catch2 ALIAS Catch2)
if (APPLE)
	# OSX builds targeting OSX 10.9 do not support new std::uncought_exception()
	# see https://github.com/catchorg/Catch2/issues/1218
	target_compile_definitions(Catch2 INTERFACE -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS)
endif()
include(Catch)

set(CATCH_EXTRA_ARGS "" CACHE STRING "Extra arguments for catch2 test suites.")

add_library(test_common INTERFACE)
target_compile_definitions(test_common INTERFACE TEST_DATA_DIR=R"\(${TEST_DATA_DIR}\)" CATCH_CONFIG_FAST_COMPILE)
target_link_libraries(test_common INTERFACE Catch2::Catch2)

if (APPLE)
    target_link_libraries(test_common INTERFACE "-liconv -framework IOKit" "-framework CoreFoundation" -lc++)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(engine)
add_subdirectory(libnest2d)
add_subdirectory(libslic3r)
add_subdirectory(slic3rutils)
add_subdirectory(fff_print)
add_subdirectory(sla_print)
add_subdirectory(cpp17 EXCLUDE_FROM_ALL)    # does not have to be built all the time
add_subdirectory(example)


add_subdirectory(cloud_device_mqtt)

# Toolbar layout policy has no GUI or slicing dependencies.
add_executable(topbar_layout_tests gui/topbar_layout_tests.cpp)
target_compile_features(topbar_layout_tests PRIVATE cxx_std_17)
add_test(NAME topbar_layout_tests COMMAND topbar_layout_tests)

add_executable(preview_layer_range_tests gui/preview_layer_range_tests.cpp)
target_compile_features(preview_layer_range_tests PRIVATE cxx_std_17)
add_test(NAME preview_layer_range_tests COMMAND preview_layer_range_tests)

# Wipe tower gap merging has no GUI or slicing dependencies.
add_executable(wipe_tower_wall_intervals_tests libslic3r/wipe_tower_wall_intervals_tests.cpp)
target_compile_features(wipe_tower_wall_intervals_tests PRIVATE cxx_std_17)
add_test(NAME wipe_tower_wall_intervals_tests COMMAND wipe_tower_wall_intervals_tests)

# Hidden real-control regression for window buttons disappearing during resize.
if (WIN32 AND SLIC3R_GUI)
    find_package(wxWidgets 3.3 CONFIG REQUIRED COMPONENTS core base aui)
    add_executable(topbar_wx_resize_tests gui/topbar_wx_resize_tests.cpp)
    target_compile_features(topbar_wx_resize_tests PRIVATE cxx_std_17)
    target_include_directories(topbar_wx_resize_tests PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(topbar_wx_resize_tests PRIVATE ${wxWidgets_DEFINITIONS})
    target_link_libraries(topbar_wx_resize_tests PRIVATE ${wxWidgets_LIBRARIES})
    add_test(NAME topbar_wx_resize_tests COMMAND topbar_wx_resize_tests)

    add_executable(parameter_window_lifetime_tests gui/parameter_window_lifetime_tests.cpp)
    target_compile_features(parameter_window_lifetime_tests PRIVATE cxx_std_17)
    target_include_directories(parameter_window_lifetime_tests PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(parameter_window_lifetime_tests PRIVATE ${wxWidgets_DEFINITIONS})
    target_link_libraries(parameter_window_lifetime_tests PRIVATE ${wxWidgets_LIBRARIES})
    add_test(NAME parameter_window_lifetime_tests COMMAND parameter_window_lifetime_tests)
endif()

# Exercise message-size fallback across real asynchronous WebSocket connections.
add_executable(websocket_read_limit_tests websocket_proxy/websocket_read_limit_test.cpp)
target_compile_features(websocket_read_limit_tests PRIVATE cxx_std_17)
target_include_directories(websocket_read_limit_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(websocket_read_limit_tests PRIVATE BOOST_ALL_NO_LIB)
find_package(Threads REQUIRED)
target_link_libraries(websocket_read_limit_tests PRIVATE Boost::headers Threads::Threads)
if (WIN32)
    target_link_libraries(websocket_read_limit_tests PRIVATE ws2_32 mswsock)
endif()
add_test(NAME websocket_read_limit_tests COMMAND websocket_read_limit_tests)
set_tests_properties(websocket_read_limit_tests PROPERTIES TIMEOUT 30)
