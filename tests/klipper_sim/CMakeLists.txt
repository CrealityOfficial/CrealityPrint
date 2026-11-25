cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(klipper_sim_regressions LANGUAGES CXX)
    enable_testing()
endif()

set(KLSIM_DIR "${CMAKE_CURRENT_LIST_DIR}/../../src/libslic3r/GCode/KlipperSim")
add_library(klipper_sim_test_support STATIC
    ${KLSIM_DIR}/KlipperToolhead.cpp
    ${KLSIM_DIR}/KlipperSimulationSession.cpp
    ${KLSIM_DIR}/KlipperGCodeStream.cpp
    ${KLSIM_DIR}/KlipperStreamingEarlyProbe.cpp
    ${KLSIM_DIR}/KlipperOnlineStepGeneration.cpp
    ${KLSIM_DIR}/KlipperTrapQ.cpp
    ${KLSIM_DIR}/KlipperItersolve.cpp
    ${KLSIM_DIR}/KlipperStepCompress.cpp
    ${KLSIM_DIR}/KlipperSteppersync.cpp
    ${KLSIM_DIR}/KlipperExtruderPA.cpp
    ${KLSIM_DIR}/KlipperClockSync.cpp
    ${KLSIM_DIR}/KlipperMCU.cpp
    ${KLSIM_DIR}/KlipperStepper.cpp
    ${KLSIM_DIR}/KlipperHostDispatch.cpp
    ${KLSIM_DIR}/KlipperDispatchAnalysis.cpp
    ${KLSIM_DIR}/KlipperSimMain.cpp
    ${KLSIM_DIR}/../PathologicalSegmentProbe.cpp)
target_compile_features(klipper_sim_test_support PUBLIC cxx_std_17)
target_compile_definitions(klipper_sim_test_support PRIVATE KLSIM_DISABLE_TBB)
target_include_directories(klipper_sim_test_support PUBLIC ${KLSIM_DIR})
target_include_directories(klipper_sim_test_support PRIVATE ${KLSIM_DIR}/test_stubs)
find_package(Threads REQUIRED)
target_link_libraries(klipper_sim_test_support PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(klipper_sim_test_support PUBLIC /utf-8 /fp:strict /fp:except-)
endif()

add_executable(klipper_toolhead_safety_tests toolhead_safety_tests.cpp)
target_link_libraries(klipper_toolhead_safety_tests PRIVATE klipper_sim_test_support)
add_test(NAME klipper_toolhead_safety_tests COMMAND klipper_toolhead_safety_tests)
set_tests_properties(klipper_toolhead_safety_tests PROPERTIES TIMEOUT 30)

add_executable(klipper_streaming_phase_tests ${KLSIM_DIR}/streaming_phase_tests.cpp)
target_link_libraries(klipper_streaming_phase_tests PRIVATE klipper_sim_test_support)
add_test(NAME klipper_streaming_phase_tests COMMAND klipper_streaming_phase_tests
    ${KLSIM_DIR}/testdata/streaming_phase0)
set_tests_properties(klipper_streaming_phase_tests PROPERTIES TIMEOUT 120)
