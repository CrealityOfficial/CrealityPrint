set(_srcdir "${CMAKE_CURRENT_LIST_DIR}/cr_km_recipe")

if (MSVC)
    set(_library_source "${_srcdir}/lib/win${DEPS_BITS}/cr_km_recipe.lib")
    set(_library_output "${DESTDIR}/lib/cr_km_recipe.lib")
elseif (APPLE)
    set(_cr_km_recipe_archs ${CMAKE_OSX_ARCHITECTURES})
    if (NOT _cr_km_recipe_archs)
        set(_cr_km_recipe_archs "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    list(LENGTH _cr_km_recipe_archs _cr_km_recipe_arch_count)
    if (_cr_km_recipe_arch_count GREATER 1)
        message(FATAL_ERROR
            "CR_KM_RECIPE does not provide a universal macOS archive. "
            "Build arm64 and x86_64 dependencies separately.")
    endif()

    list(GET _cr_km_recipe_archs 0 _cr_km_recipe_arch)
    if (_cr_km_recipe_arch MATCHES "^(arm64|aarch64)$")
        set(_cr_km_recipe_arch "arm64")
    elseif (_cr_km_recipe_arch MATCHES "^(x86_64|amd64|AMD64)$")
        set(_cr_km_recipe_arch "x86_64")
    else()
        message(FATAL_ERROR "Unsupported macOS architecture for CR_KM_RECIPE: ${_cr_km_recipe_arch}")
    endif()

    set(_library_source "${_srcdir}/lib/mac/${_cr_km_recipe_arch}/libcr_km_recipe.a")
    set(_library_output "${DESTDIR}/lib/libcr_km_recipe.a")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cr_km_recipe_arch)
    if (_cr_km_recipe_arch MATCHES "^(x86_64|amd64)$")
        set(_cr_km_recipe_arch "x86_64")
    else()
        message(FATAL_ERROR "Unsupported Linux architecture for CR_KM_RECIPE: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    set(_library_source "${_srcdir}/lib/linux/${_cr_km_recipe_arch}/libcr_km_recipe.a")
    set(_library_output "${DESTDIR}/lib/libcr_km_recipe.a")
else()
    message(FATAL_ERROR "Unsupported platform for CR_KM_RECIPE: ${CMAKE_SYSTEM_NAME}")
endif()

if (NOT EXISTS "${_library_source}")
    message(FATAL_ERROR "Missing CR_KM_RECIPE library: ${_library_source}")
endif()

set(_output
    "${DESTDIR}/include/cr_km_recipe.h"
    "${_library_output}"
)

add_custom_command(
    OUTPUT ${_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTDIR}/include" "${DESTDIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_srcdir}/include/cr_km_recipe.h" "${DESTDIR}/include/cr_km_recipe.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_library_source}" "${_library_output}"
    DEPENDS "${_srcdir}/include/cr_km_recipe.h" "${_library_source}"
    VERBATIM
)

add_custom_target(dep_CR_KM_RECIPE SOURCES ${_output})
