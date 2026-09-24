
set(DEP_CMAKE_OPTS "-DCMAKE_POSITION_INDEPENDENT_CODE=ON")

include("deps-unix-common.cmake")

# Linux builds must use the versions that are installed into DESTDIR.  The
# generic Unix setup intentionally probes the host for zlib, but allowing that
# result here makes the OpenCV dependency graph depend on the distribution
# (and causes libopencv_imgcodecs.so to pull in the host libpng at runtime).
# Keep the host package available for tools, while forcing the dependency
# project below to build and install its own zlib.
set(ZLIB_FOUND FALSE)
unset(ZLIB_LIBRARY CACHE)
unset(ZLIB_LIBRARY_RELEASE CACHE)
unset(ZLIB_LIBRARY_DEBUG CACHE)
unset(ZLIB_INCLUDE_DIR CACHE)

# Some Linuxes may have very old libpng, so it's best to bundle it instead of relying on the system version.
# find_package(PNG QUIET)
# if (NOT PNG_FOUND)
#     message(WARNING "No PNG dev package found in system, building static library. You should install the system package.")
# endif ()
