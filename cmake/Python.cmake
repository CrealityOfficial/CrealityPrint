macro(__use_python)
	find_package(Python3 COMPONENTS Interpreter Development)

	if(Python3_FOUND AND Python3_Development_FOUND)
		message(STATUS "Find Python3 ${Python3_VERSION}")
		message(STATUS "INCLUDES : ${Python3_INCLUDE_DIRS}")
		message(STATUS "LIBRARIES : ${Python3_LIBRARIES}")
		message(STATUS "RUNTIME LIBRARIES : ${Python3_RUNTIME_LIBRARIES}")
		message(STATUS "LIBRARY DIRS : ${Python3_LIBRARY_DIRS}")
		message(STATUS "Python3_EXECUTABLE : ${Python3_EXECUTABLE}")
		if(NOT Python3_EXECUTABLE)
    		message(WARNING "Python3_EXECUTABLE Must be Set.")
		endif()
		set(PYTHON ${Python3_EXECUTABLE})

		set(PYTHON_INCLUDE_DIR ${Python3_INCLUDE_DIRS})
		set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
		list(GET Python3_LIBRARIES 0 Python3_LIBRARIES_RELEASE)
		list(LENGTH Python3_LIBRARIES LIB_LEN)
		if(${LIB_LEN} GREATER 1)
			list(GET Python3_LIBRARIES 1 Python3_LIBRARIES_RELEASE)
		endif()
		set(Python3_LIBRARIES_DEBUG ${Python3_LIBRARIES_RELEASE})
		if(${LIB_LEN} GREATER 3)
			list(GET Python3_LIBRARIES 3 Python3_LIBRARIES_DEBUG)
		endif()
		message(STATUS "Python3 Debug LIBRARIES : ${Python3_LIBRARIES_DEBUG}")
		message(STATUS "Python3 Release LIBRARIES : ${Python3_LIBRARIES_RELEASE}")
		#__import_target(Python3 dll)
		set(PYTHON_LIBRARY Python3)
		
		set(PYTHON_MODULES "${CMAKE_SOURCE_DIR}/cmake/pmodules/")
	else()
		message(STATUS "Can't find Python3.")
	endif()
endmacro()

macro(__copy_python_pyc)
	message(STATUS ${PYTHON_ROOT})

	add_custom_target(__copy_python_pyc ALL COMMENT "copy third party dll!")
	__set_target_folder(__copy_python_pyc CMakePredefinedTargets)

	add_custom_command(TARGET __copy_python_pyc PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PYTHON_ROOT}/PYC/"
			"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
		)

endmacro()

macro(__wrap_python_target target)
	set_target_properties(${target} PROPERTIES DEBUG_POSTFIX "_d")
	set_target_properties(${target} PROPERTIES SUFFIX ".pyd")
endmacro()
