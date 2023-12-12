macro(__set_target_folder target folder)
	set_target_properties(${target} PROPERTIES FOLDER ${folder})
endmacro()

macro(__set_folder_targets folder)
	cmake_parse_arguments(folder "" "" "TARGET" ${ARGN})
	if(folder_TARGET)
		foreach(target ${folder_TARGET})
			set_target_properties(${target} PROPERTIES FOLDER ${folder})	
		endforeach()
	endif()
endmacro()

macro(__append_global_property property value)
	get_property(TVALUES GLOBAL PROPERTY ${property})
	set_property(GLOBAL PROPERTY ${property} ${TVALUES} ${value}) 
endmacro()



#########################Global Property
# GLOBAL_NOT_INSTALL_IMPORT   不安装导入target
#

macro(__set_not_install_import)
	set_property(GLOBAL PROPERTY GLOBAL_NOT_INSTALL_IMPORT 1) 
endmacro()
