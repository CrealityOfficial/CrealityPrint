set(FULL_JSONS  ${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/fdm_filament_common.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/fdm_machine_common.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/fdm_process_common.json
	)
set(JSONS "fdm_filament_common.json fdm_machine_common.json fdm_process_common.json")

		  
set(WRAPPERS parameter_wrapper.h
			 parameter_wrapper.cpp
			 )
		  
set(Script ${CMAKE_CURRENT_LIST_DIR}/parameter.py)

add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.h
		${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.cpp
	COMMAND
		"${Python3_EXECUTABLE}" ${Script}
		${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/
		${CMAKE_CURRENT_BINARY_DIR}
		${JSONS}
	DEPENDS ${FULL_JSONS}
)

set_source_files_properties(
	${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.h
	${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.cpp
	PROPERTIES GENERATED TRUE
	)
	
list(APPEND BINARY_PARAMETERS ${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.h
							  ${CMAKE_CURRENT_BINARY_DIR}/parameter_wrapper.cpp
							  )