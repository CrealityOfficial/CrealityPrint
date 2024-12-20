macro(__build_qml_plugin target)
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qmldir)
		message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/qmldir must exist in a qml plugin")
	endif()
	set_target_properties(${target} PROPERTIES QML_PLUGIN_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR}/qmldir)
	
	__configure_qml_plugin_target(${target})
endmacro()