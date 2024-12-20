aux_source_directory(src DIR_MIAN_SRCS)
source_group(main FILES ${DIR_MIAN_SRCS})
file(GLOB QRC_SRC qml.qrc)

#qt5_add_resources(MAIN_QRC ${QRC_SRC})

#if(WIN32)
	find_package(Qt5QuickCompiler)
	qtquick_compiler_add_resources(MAIN_QRC ${QRC_SRC})

#endif()

__add_real_target(CrealityUI dll SOURCE ${DIR_MIAN_SRCS} ${MAIN_QRC}
								 LIB Qt5::Core Qt5::Quick Qt5::Qml
								 FOLDER plugin)

qt5_add_binary_resources(CrealityUIRes image.qrc font.qrc OPTIONS -no-compress)
add_dependencies(CrealityUI CrealityUIRes)
add_custom_command(TARGET CrealityUI POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_BINARY_DIR}/CrealityUIRes.rcc
"$<TARGET_FILE_DIR:CrealityUI>/"
)
__build_qml_plugin(CrealityUI)
