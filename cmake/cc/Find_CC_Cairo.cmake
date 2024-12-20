# This sets the following variables:
# cairo target

__search_target_components(cairo
						   INC cairo.h
						   DLIB cairo
						   LIB cairo
						   PRE cairo
						   )

__test_import(cairo dll)