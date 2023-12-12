# This sets the following variables:
# medC target

if(NOT TARGET medC)
	__search_target_components(medC
							INC med.h
							DLIB medC
							LIB medC
							PRE med
							)
	
	__test_import(medC dll)
endif()