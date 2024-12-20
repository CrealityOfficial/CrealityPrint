# This sets the following variables:
# FMPEG_INCLUDE_DIRS
# FMPEG_INCLUDE_FOUND

if(NOT FMPEG_INSTALL_ROOT)
	set(FMPEG_INSTALL_ROOT $ENV{USR_INSTALL_ROOT}/ffmpeglib/)
endif()

if(FMPEG_INSTALL_ROOT)
	message(STATUS "FMpeg Specified FMPEG_INSTALL_ROOT : ${FMPEG_INSTALL_ROOT}")
	set(FMPEG_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	
	set(avcodec_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	set(avformat_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	set(avutil_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	set(swscale_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	
	set(swresample_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	
	set(avdevice_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	
	set(avfilter_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
	set(x264_INCLUDE_DIRS ${FMPEG_INSTALL_ROOT}/include/)
							   
else()
	message(FATAL_ERROR "Not Set The FMPEG_INSTALL_ROOT ")
endif()
	
macro(__fmpeg_add target dll)
	#message("${target}_LIBRARIES_DEBUG  ${${target}_LIBRARIES_DEBUG}")
	#message("${target}_LIBRARIES_RELEASE  ${${target}_LIBRARIES_RELEASE}")
    if(CC_BC_WIN)
		find_library(${target}_LIBRARIES_DEBUG
					NAMES ${target}
					PATHS "${FMPEG_INSTALL_ROOT}/lib/")		
		find_library(${target}_LIBRARIES_RELEASE
					NAMES ${target}
					PATHS "${FMPEG_INSTALL_ROOT}/lib/")
	    set(${target}_LOC_DEBUG "${FMPEG_INSTALL_ROOT}/bin/${dll}.dll")
	    set(${target}_LOC_RELEASE "${FMPEG_INSTALL_ROOT}/bin/${dll}.dll")
	elseif(CC_BC_LINUX)
		set(${target}_LIBRARIES_DEBUG "${FMPEG_INSTALL_ROOT}/lib/${dll}")
	    set(${target}_LIBRARIES_RELEASE "${FMPEG_INSTALL_ROOT}/lib/${dll}")
	    set(${target}_LOC_DEBUG ${${target}_LIBRARIES_DEBUG})
	    set(${target}_LOC_RELEASE ${${target}_LIBRARIES_RELEASE})		
	endif()
endmacro()

if(CC_BC_WIN)
	__fmpeg_add(avcodec avcodec-58)
	__fmpeg_add(avformat avformat-58)
	__fmpeg_add(avutil avutil-56)
	__fmpeg_add(swscale swscale-5)
	__fmpeg_add(swresample swresample-3)
	__fmpeg_add(avdevice avdevice-58)
	__fmpeg_add(avfilter avfilter-7)
	__fmpeg_add(x264 x264-58)
elseif(CC_BC_LINUX)
    __fmpeg_add(avcodec libavcodec.so.58)
	__fmpeg_add(avformat libavformat.so.58)
	__fmpeg_add(avutil libavutil.so.56)
	__fmpeg_add(swscale libswscale.so.5)
	__fmpeg_add(swresample libswresample.so.3)
	__fmpeg_add(avdevice libavdevice.so.58)
	__fmpeg_add(avfilter libavfilter.so.7)
elseif(CC_BC_MAC)
	message("find cc fmpeg macOS")
	find_library(avcodec_LIBRARIES_DEBUG
				NAMES avcodec.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(avcodec_LIBRARIES_RELEASE
				NAMES avcodec.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("avcodec_LIBRARIES_DEBUG  ${avcodec_LIBRARIES_DEBUG}")
	message("avcodec_LIBRARIES_RELEASE  ${avcodec_LIBRARIES_RELEASE}")
				
	find_library(avformat_LIBRARIES_DEBUG
				NAMES avformat.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(avformat_LIBRARIES_RELEASE
				NAMES avformat.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("avformat_LIBRARIES_DEBUG  ${avformat_LIBRARIES_DEBUG}")
	message("avformat_LIBRARIES_RELEASE  ${avformat_LIBRARIES_RELEASE}")
				
	find_library(avutil_LIBRARIES_DEBUG
				NAMES avutil.56
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(avutil_LIBRARIES_RELEASE
				NAMES avutil.56
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("avutil_LIBRARIES_DEBUG  ${avutil_LIBRARIES_DEBUG}")
	message("avutil_LIBRARIES_RELEASE  ${avutil_LIBRARIES_RELEASE}")
				
	find_library(swscale_LIBRARIES_DEBUG
				NAMES swscale.5
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(swscale_LIBRARIES_RELEASE
				NAMES swscale.5
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("swscale_LIBRARIES_DEBUG  ${swscale_LIBRARIES_DEBUG}")
	message("swscale_LIBRARIES_RELEASE  ${swscale_LIBRARIES_RELEASE}")
				
	find_library(swresample_LIBRARIES_DEBUG
				NAMES swresample.3
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(swresample_LIBRARIES_RELEASE
				NAMES swresample.3
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("swresample_LIBRARIES_DEBUG  ${swresample_LIBRARIES_DEBUG}")
	message("swresample_LIBRARIES_RELEASE  ${swresample_LIBRARIES_RELEASE}")
				
	find_library(avdevice_LIBRARIES_DEBUG
				NAMES avdevice.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(avdevice_LIBRARIES_RELEASE
				NAMES avdevice.58
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("avdevice_LIBRARIES_DEBUG  ${avdevice_LIBRARIES_DEBUG}")
	message("avdevice_LIBRARIES_RELEASE  ${avdevice_LIBRARIES_RELEASE}")
				
	find_library(x264_LIBRARIES_DEBUG
				NAMES x264
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")		
	find_library(x264_LIBRARIES_RELEASE
				NAMES x264
				PATHS "${FMPEG_INSTALL_ROOT}/bin_osX/")
	message("x264_LIBRARIES_DEBUG  ${x264_LIBRARIES_DEBUG}")
	message("x264_LIBRARIES_RELEASE  ${x264_LIBRARIES_RELEASE}")
endif()						   
						   

__test_import(avcodec dll)
__test_import(avformat dll)
__test_import(swscale dll)
__test_import(swresample dll)
__test_import(x264 dll)
__test_import(avutil dll)
__test_import(avdevice dll)
__test_import(avfilter dll)

if(FMPEG_INCLUDE_DIRS)
	set(FMPEG_INCLUDE_FOUND 1)
	set(Fmpeg_INCLUDE_DIRS ${FMPEG_INCLUDE_DIRS})
	message(STATUS "FMPEG_INCLUDE_DIRS : ${FMPEG_INCLUDE_DIRS}")
else()
	message(STATUS "Find FMPEG include Failed")
endif()
