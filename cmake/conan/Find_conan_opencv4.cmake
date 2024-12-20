
set(OPENCV_VERSION 470)


if(CC_BC_WIN)

	__conan_import_one(opencv dll NAME opencv_world470 LIB opencv_world470 DLIB opencv_world470d
		                                               DLL opencv_world470 DDLL opencv_world470d)

elseif(CC_BC_LINUX)
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil swresample swscale)
else()
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil postproc swresample swscale)
endif()

