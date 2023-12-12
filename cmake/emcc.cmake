message(STATUS "Build System -> [Emscripten Web]")

set(EMCC_WEB 1)
add_definitions(-D__WEB__)

find_package(Python3 REQUIRED)
if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "Python3_EXECUTABLE Must be Set.")
endif()

set(PYTHON ${Python3_EXECUTABLE} CACHE STRING "Python path")
set(EMSCRIPTEN_ROOT $ENV{EMSDK}/upstream/emscripten CACHE STRING "Emscripten path")
set(CMAKE_TOOLCHAIN_FILE ${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake)
set(WEBIDL_BINDER_SCRIPT ${EMSCRIPTEN_ROOT}/tools/webidl_binder.py)
message(STATUS "PYTHON: ${PYTHON}")
message(STATUS "EMSCRIPTEN_ROOT: ${EMSCRIPTEN_ROOT}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "WEBIDL_BINDER_SCRIPT: ${WEBIDL_BINDER_SCRIPT}")

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Type")

macro(__emcc_idl IDL_FILE GLUE_NAME GLUE_ARG IDL_OBJ IDL_JS)
	add_custom_command(
		OUTPUT ${GLUE_NAME}.cpp ${GLUE_NAME}.js
		BYPRODUCTS parser.out WebIDLGrammar.pkl
		COMMAND ${PYTHON} ${WEBIDL_BINDER_SCRIPT} ${IDL_FILE} ${GLUE_NAME}
		COMMENT "Generating WebIDL bindings"
		VERBATIM)
		
	add_custom_command(
		OUTPUT ${GLUE_NAME}.o
		COMMAND emcc ${GLUE_NAME}.cpp ${${GLUE_ARG}} -o ${GLUE_NAME}.o
		DEPENDS ${GLUE_NAME}.cpp
		COMMENT "Building bindings"
		VERBATIM)

	set(${IDL_OBJ} ${GLUE_NAME}.o)
	set(${IDL_JS} ${GLUE_NAME}.js)
endmacro()

macro(__emcc_wasm WSAM_NAME SOURCE WSAM_ARGS)

	message(STATUS "__emcc_wasm ${${SOURCE}}")
	message(STATUS "__emcc_wasm ${${WSAM_ARGS}}")
	
	add_custom_command(
		OUTPUT ${WSAM_NAME}.js ${WSAM_NAME}.wasm
		COMMAND emcc ${${SOURCE}} ${${WSAM_ARGS}} -o ${WSAM_NAME}.js
		DEPENDS trimesh2
		COMMENT "Building WASM"
		VERBATIM)
	add_custom_target(${WSAM_NAME} 
		ALL 
		DEPENDS ${WSAM_NAME}.js ${WSAM_NAME}.wasm)
endmacro()