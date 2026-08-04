set(_srcdir ${CMAKE_CURRENT_LIST_DIR}/mpfr)

# Default patch command; enable on LoongArch to fix __int128__ token
set(patch_command "")
set(_mpfr_cppflags "")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch")
    # Apply patch if possible; tolerate failure and rely on CPPFLAGS workaround
    set(patch_command git init && ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/0001-loongarch64-build.patch || true)
    set(_mpfr_cppflags "-D__int128__=__int128")
endif()

if (MSVC)
    set(_output  ${DESTDIR}/include/mpfr.h
                 ${DESTDIR}/include/mpf2mpfr.h
                 ${DESTDIR}/lib/libmpfr-4.lib 
                 ${DESTDIR}/bin/libmpfr-4.dll)

    add_custom_command(
        OUTPUT  ${_output}
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/include/mpfr.h ${DESTDIR}/include/
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/include/mpf2mpfr.h ${DESTDIR}/include/
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/lib/win${DEPS_BITS}/libmpfr-4.lib ${DESTDIR}/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/lib/win${DEPS_BITS}/libmpfr-4.dll ${DESTDIR}/bin/
    )

    add_custom_target(dep_MPFR SOURCES ${_output})

else ()

    set(_cross_compile_arg "")
    if (CMAKE_CROSSCOMPILING)
        # TOOLCHAIN_PREFIX should be defined in the toolchain file
        set(_cross_compile_arg --host=${TOOLCHAIN_PREFIX})
    endif ()

    ExternalProject_Add(dep_MPFR
        URL https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.bz2
            https://www.mpfr.org/mpfr-4.2.2/mpfr-4.2.2.tar.bz2
        URL_HASH SHA256=9ad62c7dc910303cd384ff8f1f4767a655124980bb6d8650fe62c815a231bb7b
        DOWNLOAD_DIR ${DEP_DOWNLOAD_DIR}/MPFR
        PATCH_COMMAND ${patch_command}
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND autoreconf -f -i && 
                          env "CPPFLAGS=${_mpfr_cppflags}" "CFLAGS=${_gmp_ccflags}" "CXXFLAGS=${_gmp_ccflags}" ./configure ${_cross_compile_arg} --prefix=${DESTDIR} --enable-shared=no --enable-static=yes --with-gmp=${DESTDIR} ${_gmp_build_tgt}
        BUILD_COMMAND make -j
        INSTALL_COMMAND make install
        DEPENDS dep_GMP
    )
endif ()

