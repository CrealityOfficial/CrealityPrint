# This sets the following variables:
# CGAL_INCLUDE_DIRS

#__cc_find(Boost)
#__cc_find(Eigen)

if(NOT BOOST_INCLUDE_DIRS)
	message(FATAL_ERROR "Please Specified BOOST_INCLUDE_DIRS")
endif()

if(NOT EIGEN_INCLUDE_DIRS)
	message(FATAL_ERROR "Please Specified EIGEN_INCLUDE_DIRS")
endif()

include_directories(${BOOST_INCLUDE_DIRS})
include_directories(${EIGEN_INCLUDE_DIRS})

if(NOT USE_IGL)
	add_definitions(-DCGAL_NO_GMP=1)
	add_definitions(-DCGAL_NO_MPFR=1)
endif()
add_definitions(-DCGAL_HEADER_ONLY=1)

macro(__cc_cgal_include package)
	if(NOT CGAL_INCLUDE_DIRS)
		message(FATAL_ERROR "Please Specified CGAL_INCLUDE_DIRS--> ${package}")
	endif()
	include_directories(${CGAL_INCLUDE_DIRS}/${package})
	include_directories(${CGAL_INCLUDE_DIRS}/${package}/include/)
	include_directories(${CGAL_INCLUDE_DIRS}/${package}/poly/)
endmacro()

if(CGAL_INSTALL_ROOT)
	message(STATUS "CGAL Specified CGAL_INSTALL_ROOT : ${CGAL_INSTALL_ROOT}")
	set(CGAL_INCLUDE_DIRS ${CGAL_INSTALL_ROOT}/include/)
elseif(CXCGAL_INSTALL_ROOT)
	message(STATUS "CGAL Specified CXCGAL_INSTALL_ROOT : ${CXCGAL_INSTALL_ROOT}")
	find_path(CGAL_INCLUDE_DIRS CGAL/Kernel_23/internal/Projection_traits_3.h
		HINTS  "${CXCGAL_INSTALL_ROOT}/include/"
		PATHS "/usr/include/cgal/include")
else()
	find_path(CGAL_INCLUDE_DIRS CGAL/Kernel_23/internal/Projection_traits_3.h
		HINTS  "${CXCGAL_INSTALL_ROOT}/include/"
		PATHS "$ENV{USR_INSTALL_ROOT}/include/CGAL" "$ENV{USR_INSTALL_ROOT}/include/"
		NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
		)
	if(NOT CGAL_INCLUDE_DIRS)
		find_path(CGAL_INCLUDE_DIRS CGAL/Kernel_23/internal/Projection_traits_3.h
			HINTS  "${CXCGAL_INSTALL_ROOT}/include/"
			PATHS "$ENV{USR_INSTALL_ROOT}/include/CGAL/" "$ENV{USR_INSTALL_ROOT}/include/"
			NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			)
	endif()
	message(STATUS "CGAL CGAL_INCLUDE_DIRS: ${CGAL_INCLUDE_DIRS}")
endif()

__cc_cgal_include(Installation)
__cc_cgal_include(STL_Extension)
__cc_cgal_include(Stream_support)
__cc_cgal_include(Number_types)
__cc_cgal_include(Solver_interface)
__cc_cgal_include(Polygon_mesh_processing)

__cc_cgal_include(BGL)
__cc_cgal_include(Circulator)
__cc_cgal_include(Profiling_tools)
__cc_cgal_include(Sqrt_extension)
__cc_cgal_include(Algebraic_foundations)
__cc_cgal_include(Interval_support)
__cc_cgal_include(Modular_arithmetic)
__cc_cgal_include(Property_map)
__cc_cgal_include(Random_numbers)
__cc_cgal_include(Cartesian_kernel)
__cc_cgal_include(Box_intersection_d)
__cc_cgal_include(Intersections_3)
__cc_cgal_include(Intersections_2)
__cc_cgal_include(Distance_3)
__cc_cgal_include(Distance_2)
__cc_cgal_include(Hash_map)
__cc_cgal_include(Filtered_kernel)
__cc_cgal_include(Homogeneous_kernel)
__cc_cgal_include(Arithmetic_kernel)

__cc_cgal_include(Kernel_23)
__cc_cgal_include(Kernel_d)

__cc_cgal_include(Straight_skeleton_2)
__cc_cgal_include(Polygon)
__cc_cgal_include(Polyhedron)
__cc_cgal_include(Polyhedron_IO)
__cc_cgal_include(AABB_tree)
__cc_cgal_include(Spatial_searching)
__cc_cgal_include(Spatial_sorting)
__cc_cgal_include(Generator)
__cc_cgal_include(Union_find)
__cc_cgal_include(Modifier)
__cc_cgal_include(Triangulation_2)
__cc_cgal_include(TDS_2)
__cc_cgal_include(Surface_mesh)
__cc_cgal_include(Surface_mesh_simplification)
__cc_cgal_include(HalfedgeDS)
__cc_cgal_include(Surface_mesh_parameterization)
__cc_cgal_include(Surface_mesh_segmentation)
__cc_cgal_include(Partition_2)

#enable cluster point 
__cc_cgal_include(Point_set_processing_3)
__cc_cgal_include(Point_set_3)
__cc_cgal_include(Solver_interface)
__cc_cgal_include(Triangulation_3)
__cc_cgal_include(TDS_3)
__cc_cgal_include(Subdivision_method_3)
__cc_cgal_include(Convex_decomposition_3)
__cc_cgal_include(Convex_hull_2)
__cc_cgal_include(Convex_hull_3)
__cc_cgal_include(Convex_hull_d)
__cc_cgal_include(Bounding_volumes)
__cc_cgal_include(Optimisation_basic)
__cc_cgal_include(Principal_component_analysis_LGPL)
__cc_cgal_include(CGAL_Core)

#Root 
__cc_cgal_include(CGAL/Root)