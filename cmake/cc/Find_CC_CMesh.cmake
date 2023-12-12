# This sets the following variables:
# cmesh target

if(NOT TARGET cmesh)
	__cc_find(Gmp)
	__cc_find(Mpfr)

	__search_target_components(cmesh
							INC cmesh/poly/roof.h
							DLIB cmesh
							LIB cmesh
							PRE cmesh
							)
	
	__test_import(cmesh dll)
endif()
