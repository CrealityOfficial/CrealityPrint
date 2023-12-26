__cc_find(RapidJson)
__files_group(${CMAKE_CURRENT_SOURCE_DIR}/cxgcode/ SRCS)

set(LIBS rapidjson)
set(ILIBS
	qtuser_core
	qtuser_3d
	crslice
	trimesh2
)

__add_common_library(cxgcode)
