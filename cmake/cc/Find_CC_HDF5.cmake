# This sets the following variables:
# hdf5 target

if(NOT TARGET hdf5)
	__search_target_components(hdf5
							INC hdf5.h
							DLIB libhdf5_D
							LIB libhdf5
							PRE hdf5
							)
	
	__test_import(hdf5 lib)
endif()