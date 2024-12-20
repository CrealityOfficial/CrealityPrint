include(BuildInfoUtil)

macro(__source_recurse dir src)
	file(GLOB_RECURSE _tmp_list ${dir}/*.h ${dir}/*.hpp ${dir}/*.cpp ${dir}/*.c ${dir}/*.inl)
	set(${src} ${_tmp_list})
	#message("${${src}}")
endmacro()

macro(__files_group dir src)   #support 2 level
	file(GLOB _src ${dir}/*.h ${dir}/*.cpp ${dir}/*.cc ${dir}/*.c ${dir}/*.proto)
	file(GLOB children RELATIVE ${dir} ${dir}/*)
	foreach(child ${children})
		set(sub_dir ${dir}/${child})
		if(IS_DIRECTORY ${sub_dir})
			file(GLOB sub_src ${sub_dir}/*.h ${sub_dir}/*.cpp ${sub_dir}/*.cc ${sub_dir}/*.c)
			source_group(${child} FILES ${sub_src})
			set(_src ${_src} ${sub_src})
		endif()
	endforeach()
	set(${src} ${_src})
endmacro()

macro(__files_group_2 dir folder src)   #support 2 level
	file(GLOB _src ${dir}/*.h ${dir}/*.cpp)
	file(GLOB children RELATIVE ${dir} ${dir}/*)
	foreach(child ${children})
		set(sub_dir ${dir}/${child})
		if(IS_DIRECTORY ${sub_dir})
			file(GLOB sub_src ${sub_dir}/*.h ${sub_dir}/*.cpp)
			source_group(${folder}/${child} FILES ${sub_src})
			set(_src ${_src} ${sub_src})
		endif()
	endforeach()
	set(${src} ${_src})
endmacro()

macro(__files_group_c dir src)   #support 2 level
	file(GLOB _src ${dir}/*.c)
	file(GLOB children RELATIVE ${dir} ${dir}/*)
	foreach(child ${children})
		set(sub_dir ${dir}/${child})
		if(IS_DIRECTORY ${sub_dir})
			file(GLOB sub_src ${sub_dir}/*.c)
			source_group(${child} FILES ${sub_src})
			set(_src ${_src} ${sub_src})
		endif()
	endforeach()
	set(${src} ${_src})
endmacro()

function(__recursive_add_subdirectory dir)
	file(GLOB children RELATIVE ${dir} ${dir}/*)
	foreach(child ${children})
		set(sub_dir ${dir}/${child})
		if(IS_DIRECTORY ${sub_dir} AND EXISTS ${sub_dir}/CMakeLists.txt)
			add_subdirectory(${sub_dir})
		endif()
	endforeach()
endfunction()

macro(__get_file_name path name)
	STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" ${name} ${path})
endmacro()

macro(__add_all_directory)
	__recursive_add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR})
endmacro()

macro(__build_engine_info_header)
	string(TIMESTAMP BUILD_TIME "%y_%m_%d_%H_%M")
	set(BUILD_ENGINE_INFO_HEAD "${PROJECT_NAME}_cxss_${BUILD_TIME}")
	
	set(SUB "cxss")
	__get_submodule_git_hash(${SUB} CXSS_GIT_HASH)
	__get_branch_name(MAGE_VERSION_GIT_HEAD_BRANCH)
	__get_last_commit_time(MAGE_VERSION_LAST_COMMIT_TIME)
	
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cxss/engineinfo.h.prebuild
               ${CMAKE_CURRENT_BINARY_DIR}/buildengineinfo.h)
endmacro()

macro(__build_crslice_info_header)
	string(TIMESTAMP BUILD_TIME "%y_%m_%d_%H_%M")
	set(BUILD_ENGINE_INFO_HEAD "${PROJECT_NAME}_crslice_${BUILD_TIME}")
	
	set(SUB "crslice")
	__get_submodule_git_hash(${SUB} CRSLICE_GIT_HASH)

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/crslice.h.prebuild
               ${CMAKE_CURRENT_BINARY_DIR}/crsliceinfo.h)
endmacro()

function(__scope_add tlist item)
	list(APPEND ${tlist} ${item})
	list(REMOVE_DUPLICATES ${tlist})
	#set(${tlist} ${${tlist}} CACHE STRING INTERNAL FORCE)
endfunction()

macro(__duplicates_add tlist item)
	list(APPEND ${tlist} ${item})
	list(REMOVE_DUPLICATES ${tlist})
endmacro()

macro(__assign_source_group dir)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${dir}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
		
		#message(STATUS ${_source_path})
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endmacro()

macro(__gather_source dir DEST_SOURCE)
	file(GLOB_RECURSE ${DEST_SOURCE} RELATIVE "${dir}" "*.cpp" "*.h" "*.c" "*.cc" "*.hpp" "*.CPP" "*.hxx" "*.cxx")
endmacro()

macro(__tree_add_source dir src)
	file(GLOB_RECURSE _SRCS RELATIVE "${dir}" "*.cpp" "*.h" "*.c" "*.cc" "*.hpp" "*.CPP" "*.hxx" "*.cxx")
	__assign_source_group(${dir} ${_SRCS})
	
	#message(STATUS ${dir})
	#message(STATUS ${_SRCS})
	#message(STATUS "${${src}}")
	foreach(item ${_SRCS})
		list(APPEND ${src} "${dir}/${item}")
		#message(STATUS "${${src}}")
	endforeach()
	
	#foreach(item ${${src}})
	#	message(STATUS "${item}")
	#endforeach()
		
	#message(STATUS ${${src}})
	if(QT5_ENABLED)
		file(GLOB_RECURSE QRCS RELATIVE "${dir}" "*.qrc")
		set(QRCFILES)
		foreach(item ${QRCS})
			list(APPEND QRCFILES "${dir}/${item}")
		endforeach()
		
		list(LENGTH QRCFILES ARGC)
		#message(STATUS ${ARGC} ${QRCFILES})
		if(${ARGC} GREATER 0)
			qt5_add_resources(QT_QRC ${QRCFILES})
			#message(STATUS ${QRCFILES})
		endif()
		list(APPEND ${src} ${QT_QRC})
	endif()
endmacro()

macro(__tree_add_current src)
	__tree_add_source(${CMAKE_CURRENT_SOURCE_DIR} ${src})
endmacro()

macro(__copy_ppcs_dlls dlls)
	if(WIN32)
		add_custom_target(__copy_ppcs ALL COMMENT "copy ppcs dll!")
		__set_target_folder(__copy_ppcs CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			set(_debug_dll "${CMAKE_CURRENT_SOURCE_DIR}/ppcs/Lib/Windows/x64/${dll}")
			set(_release_dll "${CMAKE_CURRENT_SOURCE_DIR}/ppcs/Lib/Windows/x64/${dll}")
			add_custom_command(TARGET __copy_ppcs PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different  
					"$<$<CONFIG:Release>:${_release_dll}>"  
					"$<$<CONFIG:Debug>:${_debug_dll}>" 
					"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
			)
		endforeach()
	else()
		add_custom_target(__copy_ppcs ALL COMMENT "copy ppcs dll!")
		__set_target_folder(__copy_ppcs CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			set(_debug_dll "${CMAKE_CURRENT_SOURCE_DIR}/ppcs/Lib/osX/x64/${dll}")
			set(_release_dll "${CMAKE_CURRENT_SOURCE_DIR}/ppcs/Lib/osX/x64/${dll}")
			add_custom_command(TARGET __copy_ppcs PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${LIB_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different  
					"$<$<CONFIG:Release>:${_release_dll}>"  
					"$<$<CONFIG:Debug>:${_debug_dll}>" 
					"${LIB_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
			)
		endforeach()
	endif()
endmacro()

macro(__copy_ffmpeg_dlls dlls)
	if(WIN32)
		add_custom_target(__copy_ffmpeg ALL COMMENT "copy ffmpeg dll!")
		__set_target_folder(__copy_ffmpeg CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			set(_debug_dll "${FMPEG_INSTALL_ROOT}/bin/${dll}")
			set(_release_dll "${FMPEG_INSTALL_ROOT}/bin/${dll}")
			add_custom_command(TARGET __copy_ffmpeg PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different  
					"$<$<CONFIG:Release>:${_release_dll}>"  
					"$<$<CONFIG:Debug>:${_debug_dll}>" 
					"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
			)
		endforeach()
	else()
		add_custom_target(__copy_ffmpeg ALL COMMENT "copy ffmpeg dll!")
		__set_target_folder(__copy_ffmpeg CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			set(_debug_dll "${FMPEG_INSTALL_ROOT}/bin_osX/${dll}")
			set(_release_dll "${FMPEG_INSTALL_ROOT}/bin_osX/${dll}")
			add_custom_command(TARGET __copy_ffmpeg PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${LIB_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different  
					"$<$<CONFIG:Release>:${_release_dll}>"  
					"$<$<CONFIG:Debug>:${_debug_dll}>" 
					"${LIB_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
			)
		endforeach()
	endif()
endmacro()

macro(__copy_third_party_dlls dlls)
	if(WIN32)
		add_custom_target(__copy_thirdparty ALL COMMENT "copy third party dll!")
		__set_target_folder(__copy_thirdparty CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			if(DEFINED ENV{USR_INSTALL_ROOT})
			    message("############copy third party usr dll ${dll}#################")
				set(_debug_dll "$ENV{USR_INSTALL_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{USR_INSTALL_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_thirdparty PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			elseif(DEFINED ENV{CX_THIRDPARTY_ROOT})
			message("------------copy third party dll  ${dll}-------------")
				set(_debug_dll "$ENV{CX_THIRDPARTY_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{CX_THIRDPARTY_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_thirdparty PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			endif()
		endforeach()
	endif()
endmacro()

macro(__copy_boost_dlls dlls)
	if(WIN32)
		add_custom_target(__copy_boostdll ALL COMMENT "copy boost dll!")
		__set_target_folder(__copy_boostdll CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			if(DEFINED ENV{USR_INSTALL_ROOT})
				set(_debug_dll "$ENV{USR_INSTALL_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{USR_INSTALL_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_boostdll PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			elseif(DEFINED ENV{CX_BOOST_ROOT})
				set(_debug_dll "$ENV{CX_BOOST_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{CX_BOOST_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_boostdll PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			endif()
		endforeach()
	endif()
endmacro()

macro(__copy_occ_dlls dlls)
	if(WIN32)
		add_custom_target(__copy_occ ALL COMMENT "copy occ dll!")
		__set_target_folder(__copy_occ CMakePredefinedTargets)

		foreach(dll ${${dlls}})
			if(DEFINED ENV{USR_INSTALL_ROOT})
				set(_debug_dll "$ENV{USR_INSTALL_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{USR_INSTALL_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_thirdparty PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			elseif(DEFINED ENV{CX_OCC_ROOT})
				set(_debug_dll "$ENV{CX_OCC_ROOT}/bin/Debug/${dll}")
				set(_release_dll "$ENV{CX_OCC_ROOT}/bin/Release/${dll}")
				add_custom_command(TARGET __copy_occ PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${_release_dll}>"  
						"$<$<CONFIG:Debug>:${_debug_dll}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			endif()
		endforeach()
	endif()
endmacro()

macro(__copy_files copy_target files)
	add_custom_target(${copy_target} ALL COMMENT "copy third party dll!")
	__set_target_folder(${copy_target} CMakePredefinedTargets)

	add_custom_command(TARGET ${copy_target} PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
		)
	foreach(file ${${files}})
		add_custom_command(TARGET ${copy_target} PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different  
				"$<$<CONFIG:Release>:${file}>"  
				"$<$<CONFIG:Debug>:${file}>" 
				"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
			)	
	endforeach()
endmacro()

macro(__add_cdirectory_test)
	#message(STATUS "__add_cdirectory_test ${ARGN}")
	file(GLOB files *.cpp *.c *.cc)
	foreach(var ${files})
		string(REGEX REPLACE ".+/(.+)\\..*" "\\1" name ${var})
		__add_real_target(${name} exe SOURCE ${var} ${ARGN})
	endforeach()
endmacro()

macro(__add_cdirectory_prefix_test prefix)
	#message(STATUS "__add_cdirectory_prefix_test ${ARGN}")
	file(GLOB files *.cpp *.c *.cc)
	foreach(var ${files})
		string(REGEX REPLACE ".+/(.+)\\..*" "\\1" name ${var})
		__add_real_target(${prefix}_${name} exe SOURCE ${var} ${ARGN})
	endforeach()
endmacro()

macro(__copy_find_targets targets)	
	set(RTARGETS ${targets})
	if(${${targets}})
		set(RTARGETS ${${targets}})
	endif()
	#message(STATUS "__copy_find_targets ${RTARGETS}")
	foreach(target ${RTARGETS})
		add_custom_target(__auto_copy_${target} ALL COMMENT "copy installed library.")
		__set_target_folder(__auto_copy_${target} CMakePredefinedTargets)
	
		get_target_property(IMPORT_LOC_DEBUG ${target} IMPORTED_LOCATION_DEBUG)
		get_target_property(IMPORT_LOC_RELEASE ${target} IMPORTED_LOCATION_RELEASE)

		if(NOT EXISTS ${IMPORT_LOC_DEBUG})
			set(IMPORT_LOC_DEBUG ${IMPORT_LOC_RELEASE})
		endif()
		if(CC_BC_WIN OR CC_BC_MAC)
			if(IMPORT_LOC_DEBUG AND IMPORT_LOC_RELEASE 
					AND EXISTS ${IMPORT_LOC_RELEASE} AND EXISTS ${IMPORT_LOC_DEBUG})
				message(STATUS "copy imported target ${target}")
				add_custom_command(TARGET __auto_copy_${target} PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"$<$<CONFIG:Release>:${IMPORT_LOC_RELEASE}>"  
						"$<$<CONFIG:Debug>:${IMPORT_LOC_DEBUG}>" 
						"${BIN_OUTPUT_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
					)
			endif()
		elseif(CC_BC_LINUX)
			if(IMPORT_LOC_RELEASE  AND EXISTS ${IMPORT_LOC_RELEASE})
				message(STATUS "copy imported target ${target}")
				add_custom_command(TARGET __auto_copy_${target} PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_OUTPUT_DIR}/Release/lib/"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different  
						"${IMPORT_LOC_RELEASE}"  
						"${BIN_OUTPUT_DIR}/Release/lib/"
					COMMENT "auto copy ${target} [${IMPORT_LOC_RELEASE}] -> [${BIN_OUTPUT_DIR}/Release/lib/]")
			endif()
		endif()
		
		if(CMAKE_BUILD_TYPE MATCHES "Release")
			if(IMPORT_LOC_RELEASE AND EXISTS ${IMPORT_LOC_RELEASE})
				if(CC_BC_WIN)
					INSTALL(FILES ${IMPORT_LOC_RELEASE} DESTINATION .)
				elseif(CC_BC_MAC)
					INSTALL(FILES ${IMPORT_LOC_RELEASE} DESTINATION "${MACOS_INSTALL_LIB_DIR}")
				elseif(CC_BC_LINUX)
					MESSAGE(STATUS \"=====linux install import target ${target}.\")
					MESSAGE(STATUS \"import loc release ${IMPORT_LOC_RELEASE} \")
					MESSAGE(STATUS \"target path ${CMAKE_INSTALL_PREFIX}/lib/ \")
					FILE(COPY ${IMPORT_LOC_RELEASE} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ FOLLOW_SYMLINK_CHAIN)
				endif()
			endif()
		endif()
	endforeach()
endmacro()