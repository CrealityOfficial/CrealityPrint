# This sets the following variables:
# fltk target

__search_target_components(fltk
						   INC FL/abi-version.h
						   DLIB fltk
						   LIB fltk
						   PRE fltk
						   )

__test_import(fltk lib)