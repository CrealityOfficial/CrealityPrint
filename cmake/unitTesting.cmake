macro(__build_unit_testing_from_directory directory)
	file(GLOB  ALL_TESTS "${directory}/*.cpp")
	
	if(CC_BC_WIN)
		set(SHELL ${CMAKE_SOURCE_DIR}/cmake/ci/shell/executeUnitTesting.bat)
	elseif(CC_BC_LINUX)
	endif()
	
	set(EXECUTE_DIR $<$<CONFIG:Release>:${BIN_OUTPUT_DIR}/Release/>$<$<CONFIG:Debug>:${BIN_OUTPUT_DIR}/Debug/>)
	
	add_custom_target(__unit_testing ALL 
		COMMAND ${SHELL} ${EXECUTE_DIR}
		COMMENT "execute all unit testing.")
	__set_target_folder(__unit_testing unitTesting)
	#message(STATUS "global_cache_libs : ${global_cache_libs}")
	#message(STATUS "ALL_TESTS : ${ALL_TESTS}")
	
	foreach(TEST ${ALL_TESTS})
		STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" RELATIVE_TEST ${TEST})
		#message(STATUS "build unit testing : ${RELATIVE_TEST}")
		__add_real_target(${RELATIVE_TEST} exe SOURCE ${TEST}
												LIB ${global_cache_libs}
												FOLDER unitTesting
												)
		add_dependencies(__unit_testing ${RELATIVE_TEST})
	endforeach()
endmacro()

macro(__python_build_unit_test_from_directory directory)
	set(SHELL ${CMAKE_SOURCE_DIR}/cmake/ci/shell/run_unit_test_from_directory.py)
	set(EXECUTE_DIR $<$<CONFIG:Release>:${BIN_OUTPUT_DIR}/Release/>$<$<CONFIG:Debug>:${BIN_OUTPUT_DIR}/Debug/>)
	
	add_custom_target(__unit_testing ALL 
		COMMAND ${SHELL} ${EXECUTE_DIR}
		COMMENT "execute all unit testing.")
	__set_target_folder(__unit_testing unit_test)
	#message(STATUS "global_cache_libs : ${global_cache_libs}")
	#message(STATUS "ALL_TESTS : ${ALL_TESTS}")
	
	file(GLOB  ALL_TESTS "${directory}/*.cpp")
	foreach(TEST ${ALL_TESTS})
		STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" RELATIVE_TEST ${TEST})
		#message(STATUS "build unit testing : ${RELATIVE_TEST}")
		__add_real_target(${RELATIVE_TEST} exe SOURCE ${TEST}
												LIB ${global_cache_libs}
												FOLDER unit_test
												)
		add_dependencies(__unit_testing ${RELATIVE_TEST})
	endforeach()
endmacro()

macro(__enable_unit_testing)
	if(CC_UNIT_TESTING)
		__cc_find(Gtest)
		__cc_find(Boost)
		__assert_target(gtest)
		__assert_target(gtest_main)
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unitTesting/)
			#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/unitTesting/)
			__build_unit_testing_from_directory(${CMAKE_CURRENT_SOURCE_DIR}/unitTesting/)
		elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit_test/)
			__python_build_unit_test_from_directory(${CMAKE_CURRENT_SOURCE_DIR}/unit_test/)
		else()
			message(FATAL_ERROR "CC_UNIT_TESTING is enabled, but ${CMAKE_CURRENT_SOURCE_DIR}/unitTesting/ or ${CMAKE_CURRENT_SOURCE_DIR}/unit_test/ not exists!")
		endif()
	endif()
endmacro()

include(Python)
__use_python()
macro(__add_testing_target target)
	message(STATUS "[cmake add testing] ---> ${target}")
	__enable_gprof()

	__assert_target(system_support)
	__add_real_target(${target} exe SOURCE ${SRCS}
			LIB ${LIBS} system_support
			FOLDER testing
			INC ${INCS}
			DEF ${DEFS}
		)

	set(python_script ${CMAKE_SOURCE_DIR}/cmake/python/test/unitTesting.py)

	if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/unitTesting.h.in)
		set(BUILD_INFO_HEAD "${target}")
		configure_file(${CMAKE_SOURCE_DIR}/cmake/unitTesting.h.in
				${CMAKE_CURRENT_BINARY_DIR}/${target}_data.h)
	endif()
	
	add_custom_command(TARGET ${target}
                   POST_BUILD
                   COMMAND ${PYTHON} ${python_script}
				   					 "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/${target}"
                   COMMENT "auto testing ------> ${target}"
				   )
endmacro()

macro(__add_unit_test target data)
	__conan_find_data(${data})
	
	if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/unitTesting.h.in)
		set(BUILD_INFO_HEAD "${target}")
		configure_file(${CMAKE_SOURCE_DIR}/cmake/unitTesting.h.in
				${CMAKE_CURRENT_BINARY_DIR}/${target}_data.h)
	endif()
	
	__add_real_target(${target} exe SOURCE ${SRCS}
			LIB ${LIBS}
			FOLDER unitTesting
			INC ${INCS}
			DEF ${DEFS}
		)
endmacro()

macro(__add_simple_unit_test target)
	__add_real_target(${target} exe SOURCE ${SRCS}
			LIB ${LIBS}
			FOLDER unittest
			INC ${INCS}
			DEF ${DEFS}
		)
endmacro()
