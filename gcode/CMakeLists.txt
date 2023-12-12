__files_group(${CMAKE_CURRENT_SOURCE_DIR}/gcode/ SRCS)

__cc_find(trimesh2)
__assert_target(trimesh2)

set(ILIBS trimesh2)
		
__add_common_library(gcode)