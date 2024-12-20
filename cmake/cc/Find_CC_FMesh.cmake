# This sets the following variables:
# fmesh target

__search_target_components(fmesh
						   INC fmesh/generate/bottomgenerator.h
						   DLIB fmesh
						   LIB fmesh
						   PRE fmesh
						   )

__test_import(fmesh dll)