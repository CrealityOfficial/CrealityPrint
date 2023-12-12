# This sets the following variables:
# lib3MF target

__search_target_components(lib3MF
						   INC lib3mf_abi.hpp
						   DLIB lib3MF
						   LIB lib3MF
						   PRE 3mf
						   )

__test_import(lib3MF lib)