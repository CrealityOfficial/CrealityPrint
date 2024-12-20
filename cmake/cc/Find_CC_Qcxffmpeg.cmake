# This sets the following variables:
# qcxffmpeg target
__cc_find(Fmpeg)
__search_target_components(qcxffmpeg
						   INC cxffmpeg/qmlplayer.h
						   DLIB qcxffmpeg
						   LIB qcxffmpeg
						   PRE qcxffmpeg
						   )

__test_import(qcxffmpeg dll)