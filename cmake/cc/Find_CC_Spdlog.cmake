# This sets the following variables:
# spdlog target
__search_target_components(spdlog
						   INC spdlog/cxlog.h
						   DLIB spdlog
						   LIB spdlog
						   PRE spdlog
						   )

__test_import(spdlog dll)
