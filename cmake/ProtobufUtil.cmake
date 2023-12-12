function(protobuf_generate)
	set(_options APPEND_PATH DESCRIPTORS GRPC)
	set(_singleargs LANGUAGE OUT_VAR EXPORT_MACRO PROTOC_OUT_DIR)
	if(COMMAND target_sources)
		list(APPEND _singleargs TARGET)
	endif()
	set(_multiargs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS)
	
	cmake_parse_arguments(protobuf_generate "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")
	
	if(NOT protobuf_generate_PROTOS AND NOT protobuf_generate_TARGET)
		message(SEND_ERROR "Error: protobuf_generate called without any targets or source files")
		return()
	endif()
	
	if(NOT protobuf_generate_LANGUAGE)
		set(protobuf_generate_LANGUAGE cpp)
	endif()
	string(TOLOWER ${protobuf_generate_LANGUAGE} protobuf_generate_LANGUAGE)
	
	if(NOT protobuf_generate_PROTOC_OUT_DIR)
		set(protobuf_generate_PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
	endif()
	
	if(protobuf_generate_EXPORT_MACRO AND protobuf_generate_LANGUAGE STREQUAL cpp)
		set(_dll_export_decl "dllexport_decl=${protobuf_generate_EXPORT_MACRO}:")
	endif()
	
	if(NOT protobuf_generate_GENERATE_EXTENSIONS)
		if(protobuf_generate_LANGUAGE STREQUAL cpp)
			set(protobuf_generate_GENERATE_EXTENSIONS .pb.h .pb.cc)
		elseif(protobuf_generate_LANGUAGE STREQUAL python)
			set(protobuf_generate_GENERATE_EXTENSIONS _pb2.py)
		else()
			message(SEND_ERROR "Error: protobuf_generate given unknown Language ${LANGUAGE}, please provide a value for GENERATE_EXTENSIONS")
			return()
		endif()
	endif()
	
	if(protobuf_generate_TARGET)
		get_target_property(_source_list ${protobuf_generate_TARGET} SOURCES)
		foreach(_file ${_source_list})
			if(_file MATCHES "proto$")
				list(APPEND protobuf_generate_PROTOS ${_file})
			endif()
		endforeach()
	endif()
	
	if(NOT protobuf_generate_PROTOS)
		message(SEND_ERROR "Error: protobuf_generate could not find any .proto files")
		return()
	endif()
	
	if(protobuf_generate_APPEND_PATH)
		# Create an include path for each file specified
		foreach(_file ${protobuf_generate_PROTOS})
			get_filename_component(_abs_file ${_file} ABSOLUTE)
			get_filename_component(_abs_path ${_abs_file} PATH)
			list(FIND _protobuf_include_path ${_abs_path} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${_abs_path})
			endif()
		endforeach()
	else()
		set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
	endif()
	
	foreach(DIR ${protobuf_generate_IMPORT_DIRS})
		get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
		list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
		if(${_contains_already} EQUAL -1)
			list(APPEND _protobuf_include_path -I ${ABS_PATH})
		endif()
	endforeach()
	
	set(_generated_srcs_all)
	message(STATUS "protobuf generate protos ${protobuf_generate_PROTOS}")
	
	if(PROTOC_EXE)
		set(protobuf_exe ${PROTOC_EXE})
	else()
		if(WIN32)
			set(protobuf_exe "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/protoc.exe")
		else()
			set(protobuf_exe "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/protoc")
		endif()
	endif()
	
	if(GRPC_PLUGIN_EXE)
		set(grpc_exe ${GRPC_PLUGIN_EXE})
	else()
		if(WIN32)
			set(grpc_exe "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/grpc_cpp_plugin.exe")
		else()
			set(grpc_exe "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/grpc_cpp_plugin")
		endif()
	endif()
	
	if(protobuf_generate_GRPC)
		set(protobuf_generate_GENERATE_EXTENSIONS ${protobuf_generate_GENERATE_EXTENSIONS} .grpc.pb.h .grpc.pb.cc)
		set(grpc_out --grpc_out=${_dll_export_decl}${protobuf_generate_PROTOC_OUT_DIR} --plugin=protoc-gen-grpc=${grpc_exe})
	endif()
	
	foreach(_proto ${protobuf_generate_PROTOS})	
		get_filename_component(_abs_file ${_proto} ABSOLUTE)
		get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
		get_filename_component(_basename ${_proto} NAME_WE)
		file(RELATIVE_PATH _rel_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_dir})
		
		list(FIND _protobuf_include_path ${_abs_dir} _contains_already)
		if(${_contains_already} EQUAL -1)
			list(APPEND _protobuf_include_path -I ${_abs_dir})
		endif()
		
		#message(STATUS "${_abs_file} ${_abs_dir} ${_rel_dir} ${_basename}")
		
		set(_possible_rel_dir)
		if (NOT protobuf_generate_APPEND_PATH)
			set(_possible_rel_dir ${_rel_dir}/)
		endif()
		
		set(_generated_srcs)
		foreach(_ext ${protobuf_generate_GENERATE_EXTENSIONS})
			list(APPEND _generated_srcs "${protobuf_generate_PROTOC_OUT_DIR}/${_basename}${_ext}")
		endforeach()
		
		if(protobuf_generate_DESCRIPTORS AND protobuf_generate_LANGUAGE STREQUAL cpp)
			set(_descriptor_file "${CMAKE_CURRENT_BINARY_DIR}/${_basename}.desc")
			set(_dll_desc_out "--descriptor_set_out=${_descriptor_file}")
			list(APPEND _generated_srcs ${_descriptor_file})
		endif()
		list(APPEND _generated_srcs_all ${_generated_srcs})
		
		set(language_out --${protobuf_generate_LANGUAGE}_out ${_dll_export_decl}${protobuf_generate_PROTOC_OUT_DIR})
		set(proto_args  ${language_out} ${grpc_out} ${_dll_desc_out} ${_protobuf_include_path} ${_abs_file})
		
		add_custom_command(
			OUTPUT ${_generated_srcs}
			COMMAND  ${protobuf_exe}
			ARGS ${proto_args}
			DEPENDS ${_abs_file} ${protobuf_exe}
			COMMENT "Running ${protobuf_generate_LANGUAGE} protocol buffer compiler on ${_proto}"
			VERBATIM )
		
		message(STATUS ${proto_args})
	endforeach()
	
	set_source_files_properties(${_generated_srcs_all} PROPERTIES GENERATED TRUE)
	if(protobuf_generate_OUT_VAR)
		set(${protobuf_generate_OUT_VAR} ${_generated_srcs_all} PARENT_SCOPE)
	endif()
	if(protobuf_generate_TARGET)
		target_sources(${protobuf_generate_TARGET} PRIVATE ${_generated_srcs_all})
	endif()
endfunction()

function(protobuf_generate_cpp SRCS HDRS)
	cmake_parse_arguments(generate_cpp "" "EXPORT_MACRO;DESCRIPTORS;GRPC" "" ${ARGN})
	
	set(_proto_files "${generate_cpp_UNPARSED_ARGUMENTS}")

	if(NOT _proto_files)
		message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
		return()
	endif()
	
	if(generate_cpp_APPEND_PATH)
		set(_append_arg APPEND_PATH)
	endif()
	
	if(generate_cpp_DESCRIPTORS)
		set(_descriptors DESCRIPTORS)
	endif()
	
	if(generate_cpp_GRPC)
		set(_append_arg ${_append_arg} GRPC)
	endif()
	
	if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
		set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
	endif()
	
	if(DEFINED Protobuf_IMPORT_DIRS)
		set(_import_arg ${Protobuf_IMPORT_DIRS})
	endif()
	
	set(_outvar)
	protobuf_generate(${_append_arg} ${_descriptors} LANGUAGE cpp 
					EXPORT_MACRO ${generate_cpp_EXPORT_MACRO}
					OUT_VAR _outvar
					IMPORT_DIRS ${_import_arg}
					PROTOS ${_proto_files})
	
	set(${SRCS})
	set(${HDRS})
	if(generate_cpp_DESCRIPTORS)
		set(${generate_cpp_DESCRIPTORS})
	endif()
	
	foreach(_file ${_outvar})
		if(_file MATCHES "cc$")
			list(APPEND ${SRCS} ${_file})
		elseif(_file MATCHES "desc$")
			list(APPEND ${generate_cpp_DESCRIPTORS} ${_file})
		else()
			list(APPEND ${HDRS} ${_file})
		endif()
	endforeach()
	set(${SRCS} ${${SRCS}} PARENT_SCOPE)
	set(${HDRS} ${${HDRS}} PARENT_SCOPE)
	message(STATUS "protobuf generate headers ${${HDRS}}")
	message(STATUS "protobuf generate sources ${${SRCS}}")
	
	if(generate_cpp_DESCRIPTORS)
		set(${generate_cpp_DESCRIPTORS} "${${generate_cpp_DESCRIPTORS}}" PARENT_SCOPE)
	endif()
endfunction()
