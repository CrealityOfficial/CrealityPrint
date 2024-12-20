# This sets the following variables:
# splitslot target

__search_target_components(splitslot
						   INC splitslot/split.h
						   DLIB splitslot
						   LIB splitslot
						   PRE splitslot
						   )

__test_import(splitslot dll)
