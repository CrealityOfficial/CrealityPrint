cmake_minimum_required(VERSION 3.15)
project(clipper)

add_library(clipper STATIC
# We are using ClipperLib compiled as part of the libslic3r project using Slic3r::Point as its base type.
#    clipper.cpp
#    clipper.hpp
    clipper_z.cpp
    clipper_z.hpp
)

# clipper.hpp now uses tbb::scalable_allocator, so the TBB headers must be visible.
target_link_libraries(clipper PUBLIC tbb_libs)

if(SLIC3R_PROFILE)
    target_link_libraries(clipper Shiny)
endif()
