
set(GTEST_COMPONENTS gtest
                    gtest_main
                    gmock
                    gmock_main
					  )
if(CC_BC_WIN)
	foreach(_component ${GTEST_COMPONENTS})
		__conan_import_one(gtest lib NAME ${_component} LIB ${_component}${OPENCV_VERSION} DLIB ${_component}${OPENCV_VERSION}d)
	endforeach()
elseif(CC_BC_LINUX)
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil swresample swscale)
else()
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil postproc swresample swscale)
endif()

set(gtest ${GTEST_COMPONENTS})