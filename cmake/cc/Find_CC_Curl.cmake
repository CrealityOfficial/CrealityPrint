# This sets the following variables:
# curl target

__search_target_components(curl
						   INC curl/curl.h
						   DLIB curl
						   LIB curl
						   PRE curl
						   )

__test_import(curl dll)