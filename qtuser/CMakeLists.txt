set(CMAKE_CXX_STANDARD 14)

if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/qtusercore/ SRCS)

set(ILIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Qml Qt5::Quick)

__cc_find(quazip_static)
__assert_target(quazip_static)

set(LIBS quazip_static)
__add_common_library(qtuser_core 
					QTQRC qtuserqml.qrc
					FOLDER qtuser
					)

if(CC_INSTALL_QTUSERCORE)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/qtusercore qtuser_core)
endif()