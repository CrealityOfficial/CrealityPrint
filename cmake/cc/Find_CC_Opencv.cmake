
# Find opencv
# This sets the following variables:
# OPENCV_INCLUDE_DIRS
# OpenCV target

macro(__opencv_add target dll)
  if(WIN32)
	find_library(${target}_LIBRARIES_DEBUG
				NAMES "${dll}d"
				PATHS "${OPENCV_INSTALL_ROOT}/install/x64/vc16/lib/")
	find_library(${target}_LIBRARIES_RELEASE
				NAMES ${dll}
				PATHS "${OPENCV_INSTALL_ROOT}/install/x64/vc16/lib/")
	set(${target}_LOC_DEBUG "${OPENCV_INSTALL_ROOT}/install/x64/vc16/bin/${dll}d.dll")
	set(${target}_LOC_RELEASE "${OPENCV_INSTALL_ROOT}/install/x64/vc16/bin/${dll}.dll")	
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_library(${target}_LIBRARIES_DEBUG
				NAMES "${dll}d"
				PATHS "${OPENCV_INSTALL_ROOT}/install/linux/lib/")
	find_library(${target}_LIBRARIES_RELEASE
				NAMES ${dll}
				PATHS "${OPENCV_INSTALL_ROOT}/install/linux/lib/")
  else()
	find_library(${target}_LIBRARIES_DEBUG
				NAMES "${dll}d"
				PATHS "${OPENCV_INSTALL_ROOT}/install/mac/lib/")
	find_library(${target}_LIBRARIES_RELEASE
				NAMES ${dll}
				PATHS "${OPENCV_INSTALL_ROOT}/install/mac/lib/")
  
  endif()
endmacro()

set(OPENCV_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_core_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_dnn_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_features2d_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_flann_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_highgui_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_imgcodecs_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_imgproc_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_ml_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_objdetect_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_photo_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_shape_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)
set(opencv_video_INCLUDE_DIRS ${OPENCV_INSTALL_ROOT}/install/include/)

if(WIN32)
	__opencv_add(opencv_core opencv_core3413)
	__opencv_add(opencv_calib3d opencv_calib3d3413)
	__opencv_add(opencv_dnn opencv_dnn3413)
	__opencv_add(opencv_features2d opencv_features2d3413)
	__opencv_add(opencv_flann opencv_flann3413)
	__opencv_add(opencv_highgui opencv_highgui3413)
	__opencv_add(opencv_imgcodecs opencv_imgcodecs3413)
	__opencv_add(opencv_imgproc opencv_imgproc3413)
	__opencv_add(opencv_ml opencv_ml3413)
	__opencv_add(opencv_objdetect opencv_objdetect3413)
	__opencv_add(opencv_photo opencv_photo3413)
	__opencv_add(opencv_shape opencv_shape3413)
	__opencv_add(opencv_video opencv_video3413)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	__opencv_add(opencv_core opencv_core)
	__opencv_add(opencv_calib3d opencv_calib3d)
	__opencv_add(opencv_dnn opencv_dnn)
	__opencv_add(opencv_features2d opencv_features2d)
	__opencv_add(opencv_flann opencv_flann)
	__opencv_add(opencv_highgui opencv_highgui)
	__opencv_add(opencv_imgcodecs opencv_imgcodecs)
	__opencv_add(opencv_imgproc opencv_imgproc)
	__opencv_add(opencv_ml opencv_ml)
	__opencv_add(opencv_objdetect opencv_objdetect)
	__opencv_add(opencv_photo opencv_photo)
	__opencv_add(opencv_shape opencv_shape)
	__opencv_add(opencv_video opencv_video)	
else()
	__opencv_add(opencv_core opencv_core.3.4.13)
	__opencv_add(opencv_calib3d opencv_calib3d.3.4.13)
	__opencv_add(opencv_dnn opencv_dnn.3.4.13)
	__opencv_add(opencv_features2d opencv_features2d.3.4.13)
	__opencv_add(opencv_flann opencv_flann.3.4.13)
	__opencv_add(opencv_highgui opencv_highgui.3.4.13)
	__opencv_add(opencv_imgcodecs opencv_imgcodecs.3.4.13)
	__opencv_add(opencv_imgproc opencv_imgproc.3.4.13)
	__opencv_add(opencv_ml opencv_ml.3.4.13)
	__opencv_add(opencv_objdetect opencv_objdetect.3.4.13)
	__opencv_add(opencv_photo opencv_photo.3.4.13)
	__opencv_add(opencv_shape opencv_shape.3.4.13)
	__opencv_add(opencv_video opencv_videoio.3.4.13)	

endif()
__test_import(opencv_core dll)
__test_import(opencv_calib3d dll)
__test_import(opencv_dnn dll)
__test_import(opencv_features2d dll)
__test_import(opencv_flann dll)
__test_import(opencv_highgui dll)
__test_import(opencv_imgcodecs dll)
__test_import(opencv_imgproc dll)
__test_import(opencv_ml dll)
__test_import(opencv_objdetect dll)
__test_import(opencv_photo dll)
__test_import(opencv_shape dll)
__test_import(opencv_video dll)

