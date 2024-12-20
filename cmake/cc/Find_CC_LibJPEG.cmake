# This sets the following variables:
# LibJPEG target

__search_target_components(LibJPEG
						   INC LibJPEG/jpeglib.h
						   DLIB LibJPEG
						   LIB LibJPEG
						   PRE LibJPEG
						   )

__test_import(LibJPEG lib)