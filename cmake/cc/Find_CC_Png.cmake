# This sets the following variables:
# png target

__search_target_components(png
						   INC png.h
						   DLIB png
						   LIB png
						   PRE png
						   )

__test_import(png lib)