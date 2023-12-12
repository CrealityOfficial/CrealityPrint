



set(freeglut_inc "/freeglut/include/")
set(freeglut_lib debug freeglutd.lib optimized freeglut.lib)
set(glew_inc "/glew/include/")
set(glew_lib debug glew.lib optimized glew.lib)
set(qtuser_inc "/qtuser/3d/" "/qtuser/core/" "/qtuser/qml/")
set(qtuser_lib debug qtuser_qml.lib qtuser_3d.lib qtuser_core.lib optimized qtuser_qml.lib qtuser_3d.lib qtuser_core.lib)
set(trimesh2_inc "/trimesh2/include/")
set(trimesh2_lib debug trimesh2.lib optimized trimesh2.lib)
set(zlib_inc "/zlib/zlib/")
set(zlib_lib debug zlibd.lib optimized zlib.lib)
set(mpir_inc "/mpir/")
set(mpir_lib debug mpir.lib optimized mpir.lib)

function(__add_target_dependency target)
	cmake_parse_arguments(target_dependency "" "" "DEP" ${ARGN})
	if(target_dependency_DEP)
		foreach(dep ${target_dependency_DEP})
			foreach(inc ${${dep}_inc})
				target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/../${inc}")
				target_include_directories(${target} PRIVATE "${CMAKE_BINARY_DIR}/../${inc}")
			endforeach()
			
			cmake_parse_arguments(libs "" "" "debug;optimized" ${${dep}_lib})
			foreach(lib ${libs_debug})
				target_link_libraries(${target} PRIVATE debug "${LIB_DEBUG_PATH}/${lib}")
			endforeach()
			foreach(lib ${libs_optimized})
				target_link_libraries(${target} PRIVATE optimized "${LIB_RELEASE_PATH}/${lib}")
			endforeach()
		endforeach()
	endif()
endfunction()


#########################Global Parameter
