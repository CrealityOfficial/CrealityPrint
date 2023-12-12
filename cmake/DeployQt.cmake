# The MIT License (MIT)
#
# Copyright (c) 2018 Nathan Osman
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

find_package(Qt5Core REQUIRED)

# Retrieve the absolute path to qmake and then use that path to find
# the windeployqt and macdeployqt binaries
get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
if(WIN32 AND NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt not found")
endif()

message(STATUS "win deploy tool -> [${WINDEPLOYQT_EXECUTABLE}]")

find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
message(STATUS "mac deploy tool -> [${MACDEPLOYQT_EXECUTABLE}]")
if(APPLE AND NOT MACDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "macdeployqt not found")
endif()

find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt HINTS "${_qt_bin_dir}")
message(STATUS "linux deploy tool -> [${LINUXDEPLOYQT_EXECUTABLE}]")
if(CC_BC_LINUX AND NOT LINUXDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "linuxdeployqt not found")
endif()
# Add commands that copy the required Qt files to the same directory as the
# target after being built as well as including them in final installation
function(windeployqt target)
    # Run windeployqt immediately after build
    add_custom_command(TARGET ${target} POST_BUILD
		#COMMAND ${CMAKE_COMMAND} -E remove_directory "${BIN_OUTPUT_DIR}/windeployqt"
        COMMAND "${CMAKE_COMMAND}" -E
            env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                --no-angle
                --no-opengl-sw
				--qmldir "${QML_ENTRY_DIR}"
				--dir "${BIN_OUTPUT_DIR}/windeployqt"
                \"$<TARGET_FILE:${target}>\"
        COMMENT "Deploying Qt..."
    )

    # windeployqt doesn't work correctly with the system runtime libraries,
    # so we fall back to one of CMake's own modules for copying them over

    # Doing this with MSVC 2015 requires CMake 3.6+
    if((MSVC_VERSION VERSION_EQUAL 1900 OR MSVC_VERSION VERSION_GREATER 1900)
            AND CMAKE_VERSION VERSION_LESS "3.6")
        message(WARNING "Deploying with MSVC 2015+ requires CMake 3.6+")
    endif()

    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    #include(InstallRequiredSystemLibraries)
    foreach(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        get_filename_component(filename "${lib}" NAME)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E
                copy_if_different "${lib}" \"$<TARGET_FILE_DIR:${target}>\"
            COMMENT "Copying ${filename}..."
        )
    endforeach()
endfunction()

macro(__windeployqt target)
	set(QMLDIR)
	if(QML_ENTRY_DIR)
		set(QMLDIR --qmldir ${QML_ENTRY_DIR})
	endif()
	set(QDIR --dir $<$<CONFIG:Release>:${BIN_OUTPUT_DIR}/Release/>$<$<CONFIG:Debug>:${BIN_OUTPUT_DIR}/Debug/>)
	
	if(CMAKE_BUILD_TYPE MATCHES "Release")
		add_custom_command(TARGET ${target} POST_BUILD
			#COMMAND ${CMAKE_COMMAND} -E remove_directory "${BIN_OUTPUT_DIR}/windeployqt"
			COMMAND "${CMAKE_COMMAND}" -E
				env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
					--verbose 0
					--no-compiler-runtime
					--angle
					${QMLDIR}
					--dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
					\"$<TARGET_FILE:${target}>\"
			COMMENT "Deploying Qt..."
		)
		INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/windeployqt/ DESTINATION .)
	endif()

    # windeployqt doesn't work correctly with the system runtime libraries,
    # so we fall back to one of CMake's own modules for copying them over

    # Doing this with MSVC 2015 requires CMake 3.6+
    if((MSVC_VERSION VERSION_EQUAL 1900 OR MSVC_VERSION VERSION_GREATER 1900)
            AND CMAKE_VERSION VERSION_LESS "3.6")
        message(WARNING "Deploying with MSVC 2015+ requires CMake 3.6+")
    endif()

    include(InstallRequiredSystemLibraries)
    foreach(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        get_filename_component(filename ${lib} NAME)
        INSTALL(FILES ${lib} DESTINATION .)
    endforeach()
endmacro()

# Add commands that copy the required Qt files to the application bundle
# represented by the target.
function(macdeployqt target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${MACDEPLOYQT_EXECUTABLE}"
            \"$<TARGET_FILE_DIR:${target}>/../..\"
            -always-overwrite -qmldir="${QML_ENTRY_DIR}"
        COMMENT "Deploying Qt...qml:${QML_ENTRY_DIR}"
    )
endfunction()

function(__macdeployqt target)
	set(QMLDIR)
	if(QML_ENTRY_DIR)
		set(QMLDIR -qmldir=${QML_ENTRY_DIR})
	endif()
	set(QDIR --dir $<$<CONFIG:Release>:${BIN_OUTPUT_DIR}/Release/>$<$<CONFIG:Debug>:${BIN_OUTPUT_DIR}/Debug/>)
	#message(STATUS "mac deploy bundle $<TARGET_FILE_DIR:${target}>/../..")
    #add_custom_command(TARGET ${target} POST_BUILD
    #    COMMAND "${MACDEPLOYQT_EXECUTABLE}"
	#	\"$<TARGET_FILE_DIR:${target}>/../..\"
    #        -always-overwrite
	#		${QMLDIR}
    #    COMMENT "Deploying Qt...qml:${QML_ENTRY_DIR}, bundle $<TARGET_FILE_DIR:${target}>/../..}"
    #)
	
	if(CMAKE_BUILD_TYPE MATCHES "Release")
		#add_custom_command(TARGET ${target} POST_BUILD
		#	COMMAND "${MACDEPLOYQT_EXECUTABLE}"
		#			-always-overwrite
		#			${QMLDIR}
		#			--dir "${CMAKE_CURRENT_BINARY_DIR}/macdeployqt"
		#			\"$<TARGET_FILE:${target}>\"
		#	COMMENT "Deploying Qt..."
		#)
		#INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/macdeployqt/ DESTINATION .)
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${MACDEPLOYQT_EXECUTABLE}"
				\"$<TARGET_FILE_DIR:${target}>/../..\"
				-always-overwrite -qmldir="${QML_ENTRY_DIR}"
			COMMENT "Deploying Qt...qml:${QML_ENTRY_DIR}"
		)
        add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${Qt5Core_DIR}/../../../plugins/renderers/" "$<TARGET_FILE_DIR:${target}>/../PlugIns/renderers/"
            COMMENT "Copy Render plugin $<TARGET_FILE_DIR:${target}>/renderers/"
        )
	endif()
endfunction()

function(__linuxdeployqt target)
	set(QMLDIR)
	if(QML_ENTRY_DIR)
		set(QMLDIR -qmldir=${QML_ENTRY_DIR})
	endif()
	set(QDIR --dir $<$<CONFIG:Release>:${BIN_OUTPUT_DIR}/Release/>$<$<CONFIG:Debug>:${BIN_OUTPUT_DIR}/Debug/>)
	message("QDIR =${QDIR}  TARGET = $<TARGET_FILE:${target}>")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${LINUXDEPLOYQT_EXECUTABLE}"
		\"$<TARGET_FILE:${target}>\"
            -always-overwrite
			-appimage
			-unsupported-allow-new-glibc
			${QMLDIR}
        COMMENT "Deploying Qt...qml:${QML_ENTRY_DIR}, bundle ${target}>}"
    )
endfunction()

mark_as_advanced(WINDEPLOYQT_EXECUTABLE MACDEPLOYQT_EXECUTABLE LINUXDEPLOYQT_EXECUTABLE)
