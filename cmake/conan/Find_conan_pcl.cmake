set(PCL_COMPONENTS pcl_common
				   pcl_features
				   pcl_filters
				   pcl_io
				   pcl_io_ply
				   pcl_kdtree
				   pcl_keypoints
				   pcl_ml
				   pcl_octree
				   pcl_outofcore
				   pcl_people
				   pcl_recognition
				   pcl_registration
				   pcl_sample_consensus
				   pcl_search
				   pcl_segmentation
				   pcl_stereo
				   pcl_surface
				   pcl_tracking
				   pcl_visualization
					  )
#set(PCL 3413)

if(CC_BC_WIN)
	foreach(_component ${PCL_COMPONENTS})
		__conan_import_one(pcl dll NAME ${_component} LIB ${_component} DLIB ${_component}d
														 DLL ${_component} DDLL ${_component}d)
	endforeach()
elseif(CC_BC_LINUX)
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil swresample swscale)
else()
	#__conan_import(ffmpeglib dll COMPONENT avcodec avdevice avfilter avformat avutil postproc swresample swscale)
endif()

set(pcl ${PCL_COMPONENTS})