set(CMAKE_CXX_STANDARD 14)

list(APPEND SRCS
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/libnest2d.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nester.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/geometry_traits.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/geometry_traits_nfp.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/common.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/optimizer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/utils/metaloop.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/utils/rotfinder.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/utils/rotcalipers.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/placers/placer_boilerplate.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/placers/bottomleftplacer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/placers/nfpplacer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/selections/selection_boilerplate.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/selections/filler.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/selections/firstfit.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/selections/djd_heuristic.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/backends/clipper/geometries.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/backends/clipper/clipper_polygon.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/optimizers/nlopt/nlopt_boilerplate.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/optimizers/nlopt/simplex.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/optimizers/nlopt/subplex.hpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/optimizers/nlopt/genetic.hpp
    #${CMAKE_CURRENT_LIST_DIR}/src/libnest2d.cpp
    ${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/libnfporb.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/history.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/geometry.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/svg.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/translation_vector.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/wkt.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/find_feasible.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/search_start.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/select_next.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/slide.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/touching_point.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/libnest2d/nfp/algo/trim_vector.hpp
	
	${CMAKE_CURRENT_LIST_DIR}/include/polygonLib/polygonLib.h
	${CMAKE_CURRENT_LIST_DIR}/include/polygonLib/delaunator.h
	${CMAKE_CURRENT_LIST_DIR}/include/polygonLib/svghelper.h
	
	${CMAKE_CURRENT_LIST_DIR}/src/polygonLib.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/svg.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/libnfporb.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/geometry.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/svghelper.cpp
	
	${CMAKE_CURRENT_LIST_DIR}/tools/benchmark.h
	${CMAKE_CURRENT_LIST_DIR}/tools/svgtools.hpp
    )

list(APPEND DEFS LIBNEST2D_OPTIMIZER_nlopt LIBNEST2D_GEOMETRIES_clipper)
list(APPEND INCS ${CMAKE_CURRENT_LIST_DIR}/include/)

if(TARGET tbb)
	list(APPEND LIBS tbb)
	list(APPEND DEFS USE_TBB)
endif()