# This sets the following variables:
# thumbnail target

__search_target_components(thumbnail
						   INC thumbnail/stl_thumbnail.h
						   DLIB thumbnail
						   LIB thumbnail
						   PRE thumbnail
						   )

__test_import(thumbnail dll)