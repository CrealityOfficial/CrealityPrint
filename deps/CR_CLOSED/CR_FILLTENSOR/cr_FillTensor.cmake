set(_srcdir "${CMAKE_CURRENT_LIST_DIR}/cr_FillTensor")

if (MSVC)
    set(_library_source "${_srcdir}/lib/win${DEPS_BITS}/cr_FillTensor_library.lib")
    set(_library_output "${DESTDIR}/lib/cr_FillTensor_library.lib")
elseif (APPLE)
    set(_cr_FillTensor_archs ${CMAKE_OSX_ARCHITECTURES})
    if (NOT _cr_FillTensor_archs)
        set(_cr_FillTensor_archs "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    list(LENGTH _cr_FillTensor_archs _cr_FillTensor_arch_count)
    if (_cr_FillTensor_arch_count GREATER 1)
        message(FATAL_ERROR
            "CR_FILLTENSOR does not provide a universal macOS archive. "
            "Build arm64 and x86_64 dependencies separately.")
    endif()

    list(GET _cr_FillTensor_archs 0 _cr_FillTensor_arch)
    if (_cr_FillTensor_arch MATCHES "^(arm64|aarch64)$")
        set(_cr_FillTensor_arch "arm64")
    elseif (_cr_FillTensor_arch MATCHES "^(x86_64|amd64|AMD64)$")
        set(_cr_FillTensor_arch "x86_64")
    else()
        message(FATAL_ERROR "Unsupported macOS architecture for CR_FILLTENSOR: ${_cr_FillTensor_arch}")
    endif()

    set(_library_source "${_srcdir}/lib/mac/${_cr_FillTensor_arch}/libcr_FillTensor_library.a")
    set(_library_output "${DESTDIR}/lib/libcr_FillTensor_library.a")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cr_FillTensor_arch)
    if (_cr_FillTensor_arch MATCHES "^(x86_64|amd64)$")
        set(_cr_FillTensor_arch "x86_64")
    else()
        message(FATAL_ERROR "Unsupported Linux architecture for CR_FILLTENSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    set(_library_source "${_srcdir}/lib/linux/${_cr_FillTensor_arch}/libcr_FillTensor_library.a")
    set(_library_output "${DESTDIR}/lib/libcr_FillTensor_library.a")
else()
    message(FATAL_ERROR "Unsupported platform for CR_FILLTENSOR: ${CMAKE_SYSTEM_NAME}")
endif()

if (NOT EXISTS "${_library_source}")
    message(FATAL_ERROR "Missing CR_FILLTENSOR library: ${_library_source}")
endif()

set(_output
    "${DESTDIR}/include/cr_FillTensor_library.h"
    "${_library_output}"
)

add_custom_command(
    OUTPUT ${_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTDIR}/include" "${DESTDIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_srcdir}/include/cr_FillTensor_library.h" "${DESTDIR}/include/cr_FillTensor_library.h"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_library_source}" "${_library_output}"
    DEPENDS "${_srcdir}/include/cr_FillTensor_library.h" "${_library_source}"
    VERBATIM
)

add_custom_target(dep_CR_FILLTENSOR SOURCES ${_output})
