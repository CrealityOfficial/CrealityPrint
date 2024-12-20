set(OPENCV_COMPONENTS opencv_calib3d
					  opencv_core
					  opencv_dnn
					  opencv_features2d
					  opencv_flann
					  opencv_highgui
					  opencv_imgcodecs
					  opencv_imgproc
					  opencv_ml
					  opencv_objdetect
					  opencv_photo
					  opencv_shape
					  opencv_video
					  )
set(OPENCV_VERSION 3413)

if(CC_BC_WIN)
	foreach(_component ${OPENCV_COMPONENTS})
		__conan_import_one(opencv dll NAME ${_component} LIB ${_component}${OPENCV_VERSION} DLIB ${_component}${OPENCV_VERSION}d
														 DLL ${_component}${OPENCV_VERSION} DDLL ${_component}${OPENCV_VERSION}d)
	endforeach()
elseif(CC_BC_LINUX)
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil swresample swscale)
else()
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil postproc swresample swscale)
endif()

set(opencv ${OPENCV_COMPONENTS})