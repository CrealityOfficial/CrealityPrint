# This sets the following variables:
# libnest2d target

if(NOT TARGET libnest2d)
	__cc_find(Nlopt)
	__search_target_components(libnest2d
							INC libnest2d/nester.hpp
							DLIB libnest2d
							LIB libnest2d
							PRE libnest2d
							)
	
	__test_import(libnest2d lib)
endif()
