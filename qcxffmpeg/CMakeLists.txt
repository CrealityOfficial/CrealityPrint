__cc_find(ffmpeglib)
__cc_find(metartc)
__assert_target(metartc6)
__assert_target(metartccore6)
__assert_target(yangwincodec6)
if(NOT TARGET Qt5::Core)
	__enable_qt5()
endif()
__cc_find(openssl)
set(SRCS interface/cxffmpeg/interface.h
		 interface/cxffmpeg/qmlplayer.h
		 interface/cxffmpeg/videoplayer.h
		 interface/cxffmpeg/frameprovider.h
		 interface/cxffmpeg/framesource.h
		 impl/qmlplayer.cpp
		 impl/VideoDecoder.h
		 impl/VideoDecoder.cpp
		 impl/WebRTCDecoder.h
		 impl/WebRTCDecoder.cpp
		 impl/YangPlayerHandleImpl.h
		 impl/YangPlayerHandleImpl.cpp
		 impl/YangRtcReceive.h
		 impl/YangRtcReceive.cpp
		 impl/YangPlayerBase.cpp
		 impl/YangPlayerDecoder.cpp
		 impl/YangPlayerPlay.cpp
		 )
		 
set(DEFS PLAYER_FFMPEG_DLL WIN32_LEAN_AND_MEAN)
list(APPEND DEFS _GLIBCXX_USE_NANOSLEEP)
set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/impl/ ${CMAKE_CURRENT_SOURCE_DIR}/interface)
set(LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Quick Qt5::Qml Qt5::Multimedia Qt5::Concurrent avutil avcodec avformat swscale swresample  metartc6 metartccore6  srtp2 usrsctp opus speexdsp ssl crypto)
if(CC_BC_WIN)
	list(APPEND LIBS Ws2_32 Avrt yangwincodec6)
endif()
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
			
