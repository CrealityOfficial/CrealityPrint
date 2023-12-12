# This sets the following variables:
# OCC target

set(OSG_LIBS OpenThreads
			 osg
			 osgDB
			 osgGA
			 osgText
			 osgUtil
			 osgViewer
			 )
			 
foreach(item ${OSG_LIBS})
	if(NOT TARGET ${item})
		__search_target_components(${item}
								INC osg/Array
								DLIB ${item}
								LIB ${item}
								PRE OpenSceneGraph
								)
								
		__test_import(${item} dll)
	endif()
endforeach()
