__enable_cxx17()
include(StandardProjectSettings.cmake)

if(USE_CACHE_ON)
	set(USE_CACHE 1)
else()
	set(USE_CACHE 0)
endif()

__build_crslice_info_header()

__cc_find(trimesh2)
__assert_target(trimesh2)

__cc_find(polyclipping)
__assert_target(polyclipping)

__cc_find(stb)
__assert_target(stb)

__cc_find(rapidjson)
__assert_target(rapidjson)
			
set(ILIBS trimesh2) 
set(LIBS polyclipping stb rapidjson)

__cc_find(cxlog)
if(TARGET cxlog)
	list(APPEND LIBS cxlog)
endif() 

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/crslice/ SRCS1)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/src/ SRCS2)

set(SRCS ${SRCS1} ${SRCS2})		 
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}
		 ${CMAKE_CURRENT_SOURCE_DIR}/src
		 )
set(DEFS BOOST_ALL_NO_LIB)
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR})

include(impl/cura5.0/cura.cmake)
include(Slice3rBase.cmake)

#binary
if(Python3_FOUND)
	set(Script ${PYTHON_MODULES}/textlizer.py)
	
	set(JSONS base.json
			  blackmagic.json
			  command_line_settings.json
			  cooling.json
			  dual.json
			  experimental.json
			  infill.json
			  machine.json
			  material.json
			  meshfix.json
			  platform_adhesion.json
			  resolution.json
			  shell.json
			  special.json
			  speed.json
			  support.json
			  travel.json
			  )
	
	set(BINARY_JSONS)
	foreach(json ${JSONS})
		set(binary_json ${json}.h)
		
		add_custom_command(
			OUTPUT
				${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			COMMAND
				"${Python3_EXECUTABLE}" ${Script}
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/${json}
				${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parameter/base/${json}
		)
	
		set_source_files_properties(
			${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			PROPERTIES GENERATED TRUE
			)
			
		list(APPEND BINARY_JSONS ${CMAKE_CURRENT_BINARY_DIR}/${binary_json})
	endforeach()
		
	set(KEY_JSONS extruder_keys.json
				  machine_keys.json
				  material_keys.json
				  profile_keys.json
			  )	
	foreach(json ${KEY_JSONS})
		set(binary_json ${json}.h)
		
		add_custom_command(
			OUTPUT
				${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			COMMAND
				"${Python3_EXECUTABLE}" ${Script}
				${CMAKE_CURRENT_SOURCE_DIR}/parameter/keys/${json}
				${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parameter/keys/${json}
		)
	
		set_source_files_properties(
			${CMAKE_CURRENT_BINARY_DIR}/${binary_json}
			PROPERTIES GENERATED TRUE
			)
			
		list(APPEND BINARY_JSONS ${CMAKE_CURRENT_BINARY_DIR}/${binary_json})
	endforeach()
	
	list(APPEND DEFS USE_BINARY_JSON)
endif()

__add_common_library(crslice FOLDER slice OPENMP
							 GENERATED_SOURCE ${BINARY_JSONS}
							 )

use_threads(crslice)
enable_sanitizers(crslice)

if(CC_INSTALL_CRSLICE)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/crslice crslice)
endif()
								