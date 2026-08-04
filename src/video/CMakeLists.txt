cmake_minimum_required(VERSION 3.13)
project(webrtc_video)

include(PrecompiledHeader)
find_package(FFmpeg)
if(FFMPEG_VERSION)
    set(VIDEO_SOURCES 
        RTSPDecoder.h
        RTSPDecoder.cpp
    )
    if(ENABLE_WEBRTC_VIDEO)
        list(APPEND VIDEO_SOURCES
            WebRTCDecoder.h
            WebRTCDecoder.cpp
            YangPlayerBase.cpp
            YangPlayerDecoder.cpp
            YangPlayerHandleImpl.cpp
            YangPlayerHandleImpl.h
            YangPlayerPlay.cpp
            YangRtcReceive.cpp
            YangRtcReceive.h
        )
    endif(ENABLE_WEBRTC_VIDEO)
    set(VIDEO_INCLUDE_DIRS
        ${CMAKE_PREFIX_PATH}/include/metartc/
        ${CMAKE_PREFIX_PATH}/include/
    )
    add_library(webrtc_video STATIC ${VIDEO_SOURCES})
    target_include_directories(webrtc_video
        PUBLIC
            ${VIDEO_INCLUDE_DIRS}
    )


        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
        set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

        #find_package(OpenSSL REQUIRED)
        #set(X264_LIB_PATH "/opt/homebrew/lib/libx264.a")
        set(VIDEO_LIBS
            FFmpeg::avformat
            FFmpeg::avcodec
            FFmpeg::avutil
            FFmpeg::swresample
            FFmpeg::swscale
            FFmpeg::avfilter
        )
        if(NOT WIN32)
            find_library(SLIC3R_X264_LIBRARY
                NAMES x264
                PATHS ${CMAKE_PREFIX_PATH}
                PATH_SUFFIXES lib
                NO_DEFAULT_PATH
            )
            if(SLIC3R_X264_LIBRARY)
                list(APPEND VIDEO_LIBS ${SLIC3R_X264_LIBRARY})
            else()
                list(APPEND VIDEO_LIBS x264)
            endif()
        endif()
        if(ENABLE_WEBRTC_VIDEO)
            find_library(LIBMETARTC metartc6)
            find_library(LIBMETARTCCORE metartccore6)
            list(APPEND VIDEO_LIBS
                ${LIBMETARTC}
                ${LIBMETARTCCORE}
                usrsctp
                srtp2
                asound
                opus
                speexdsp
                lzma
            )
        endif(ENABLE_WEBRTC_VIDEO)
        target_link_libraries(webrtc_video INTERFACE boost_libs ${VIDEO_LIBS} ${OPENSSL_LIBRARIES})
    endif()
