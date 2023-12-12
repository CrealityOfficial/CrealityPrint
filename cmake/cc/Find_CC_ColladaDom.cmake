# This sets the following variables:
# collada-dom target

if(NOT TARGET colladadom)
	__cc_find(Boost)
	__search_target_components(colladadom
							INC dom/domAny.h
							DLIB colladadom
							LIB colladadom
							PRE colladadom
							)
	
	__test_import(colladadom dll)
endif()