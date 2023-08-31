aux_source_directory(src DIR_MIAN_SRCS)
source_group(main FILES ${DIR_MIAN_SRCS})
file(GLOB QRC_SRC *.qrc)

qt5_add_resources(MAIN_QRC ${QRC_SRC})

if(WIN32)
	find_package(Qt5QuickCompiler)
	qtquick_compiler_add_resources(RESOURCES ${QRC_SRC})
endif()

__add_real_target(CrealityUI dll SOURCE ${DIR_MIAN_SRCS} ${MAIN_QRC}
								 LIB Qt5::Core Qt5::Quick Qt5::Qml
								 FOLDER plugin)

__build_qml_plugin(CrealityUI)
