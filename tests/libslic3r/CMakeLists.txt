get_filename_component(_TEST_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
add_executable(${_TEST_NAME}_tests 
	${_TEST_NAME}_tests.cpp
	test_3mf.cpp
	test_aabbindirect.cpp
	test_arrange.cpp
	test_clipper_offset.cpp
	test_clipper_utils.cpp
	test_infill_config.cpp
	test_config.cpp
	test_cloud_preset_serialization.cpp
	test_creality_version.cpp
	test_elephant_foot_compensation.cpp
	test_filament_change_topology.cpp
	test_filament_colour_state.cpp
	test_geometry.cpp
	test_placeholder_parser.cpp
	test_gcode_processor.cpp
	test_zaa_preview.cpp
	test_preset_config_cache.cpp
	test_preset_normalize.cpp
	test_project_variant_restore.cpp
	test_preset_project_reset.cpp
	test_polygon.cpp
	test_mutable_polygon.cpp
	test_mutable_priority_queue.cpp
	test_stl.cpp
	test_meshboolean.cpp
	test_marchingsquares.cpp
	test_timeutils.cpp
	test_voronoi.cpp
    test_optimizers.cpp
    test_png_io.cpp
    test_timeutils.cpp
    test_indexed_triangle_set.cpp
    ../libnest2d/printer_parts.cpp
	)

if (TARGET OpenVDB::openvdb)
    target_sources(${_TEST_NAME}_tests PRIVATE test_hollowing.cpp)
endif()
    
target_link_libraries(${_TEST_NAME}_tests test_common libslic3r OpenSSL::SSL OpenSSL::Crypto Bcrypt)
set_property(TARGET ${_TEST_NAME}_tests PROPERTY FOLDER "tests")

if (WIN32)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        crealityprint_copy_dlls(COPY_DLLS "Debug" "d" "")
    else()
        crealityprint_copy_dlls(COPY_DLLS "Release" "" "")
    endif()
endif()

# catch_discover_tests(${_TEST_NAME}_tests TEST_PREFIX "${_TEST_NAME}: ")
add_test(${_TEST_NAME}_tests ${_TEST_NAME}_tests ${CATCH_EXTRA_ARGS})

# Small runner for ZAA material planning and opt-in external project export.
add_executable(zaa_material_motion_tests
    ../fff_print/fff_print_tests.cpp
    test_zaa_material_motion.cpp
    test_zaa_preview.cpp)
find_package(assimp CONFIG REQUIRED)
target_link_libraries(zaa_material_motion_tests test_common libslic3r
    OpenSSL::SSL OpenSSL::Crypto Bcrypt assimp::assimp ${CP_TPMS_Gradual})
set_property(TARGET zaa_material_motion_tests PROPERTY FOLDER "tests")
add_test(NAME zaa_material_motion COMMAND zaa_material_motion_tests)
