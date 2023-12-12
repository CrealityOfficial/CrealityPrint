# This sets the following variables:
# pixman target

__search_target_components(pixman
						   INC pixman.h
						   DLIB pixman
						   LIB pixman
						   PRE pixman
						   )

__test_import(pixman lib)