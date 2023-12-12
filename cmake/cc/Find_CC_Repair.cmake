# This sets the following variables:
# repair target

__search_target_components(repair
						   INC cmesh/mesh/meshrepair.h
						   DLIB repair
						   LIB repair
						   PRE repair
						   )

__test_import(repair dll)