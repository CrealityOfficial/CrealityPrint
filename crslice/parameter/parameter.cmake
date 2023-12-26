set(JSONS "blackmagic.json command_line_settings.json cooling.json dual.json experimental.json infill.json machine.json material.json meshfix.json platform_adhesion.json resolution.json shell.json special.json speed.json support.json travel.json")
		  
set(FULL_JSONS  ${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/blackmagic.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/command_line_settings.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/cooling.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/dual.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/experimental.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/infill.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/machine.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/material.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/meshfix.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/platform_adhesion.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/resolution.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/shell.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/special.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/speed.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/support.json
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/travel.json
				)
		  
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