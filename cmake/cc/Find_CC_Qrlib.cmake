# This sets the following variables:
# qrlib target

__search_target_components(qrlib
						   INC qrlib/urlqrcode.h
						   DLIB qrlib
						   LIB qrlib
						   PRE qrlib
						   )

__test_import(qrlib dll)