# This sets the following variables:
# trimesh2 target

__check_target_return(trimesh2)

if(NOT TARGET trimesh2)
	__search_target_components(trimesh2
							INC trimesh2/TriMesh.h
							DLIB trimesh2
							LIB trimesh2
							PRE trimesh2
							)
	
	__test_import(trimesh2 lib)
endif()
