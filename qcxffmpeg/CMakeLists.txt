__cc_find(ffmpeglib)
if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()

set(SRCS interface/cxffmpeg/interface.h
		 interface/cxffmpeg/qmlplayer.h
		 impl/qmlplayer.cpp
		 impl/VideoDecoder.h
		 impl/VideoDecoder.cpp
		 )
		 
set(DEFS PLAYER_FFMPEG_DLL)
list(APPEND DEFS _GLIBCXX_USE_NANOSLEEP)
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/impl/ ${CMAKE_CURRENT_SOURCE_DIR}/interface)
set(LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Quick Qt5::Multimedia avutil avcodec avformat swscale swresample)

find_package(Qt5 COMPONENTS Multimedia REQUIRED)
__add_real_target(qcxffmpeg dll SOURCE ${SRCS}
								DEF ${DEFS}
								INC ${INCS}
								LIB ${LIBS}
								INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interface
								)
								
if(CC_INSTALL_QCXFFMPEG)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/interface/ qcxffmpeg)
endif()
			
