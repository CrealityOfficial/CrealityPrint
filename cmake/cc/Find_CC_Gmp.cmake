# This sets the following variables:
# gmp target

if(NOT TARGET gmp)
	if(NOT GMP_INSTALL_ROOT)
		set(GMP_INSTALL_ROOT $ENV{USR_INSTALL_ROOT})
	endif()
	
	set(gmp_INCLUDE_DIRS ${GMP_INSTALL_ROOT}/include/gmp/)
	
    if(CC_BC_WIN)
		set(gmp_LIBRARIES_DEBUG "${GMP_INSTALL_ROOT}/lib/Debug/libgmp-10.lib")
	    set(gmp_LIBRARIES_RELEASE "${GMP_INSTALL_ROOT}/lib/Release/libgmp-10.lib")
	    set(gmp_LOC_DEBUG "${GMP_INSTALL_ROOT}/bin/Debug/libgmp-10.dll")
	    set(gmp_LOC_RELEASE "${GMP_INSTALL_ROOT}/bin/Release/libgmp-10.dll")	
	elseif(CC_BC_LINUX)	
		set(gmp_LIBRARIES_DEBUG "${GMP_INSTALL_ROOT}/lib/Debug/libgmp.a")
	    set(gmp_LIBRARIES_RELEASE "${GMP_INSTALL_ROOT}/lib/Release/libgmp.a")
	    set(gmp_LOC_DEBUG "${GMP_INSTALL_ROOT}/bin/Debug/libgmp.a")
	    set(gmp_LOC_RELEASE "${GMP_INSTALL_ROOT}/bin/Release/libgmp.a")	
	elseif(CC_BC_MAC)
          set(gmp_LIBRARIES_DEBUG "${GMP_INSTALL_ROOT}/bin/Release/libgmp.dylib")
            set(gmp_LIBRARIES_RELEASE "${GMP_INSTALL_ROOT}/bin/Release/libgmp.dylib")
            set(gmp_LOC_DEBUG "${GMP_INSTALL_ROOT}/bin/Release/libgmp.dylib")
            set(gmp_LOC_RELEASE "${GMP_INSTALL_ROOT}/bin/Release/libgmp.dylib")

	endif()

	__test_import(gmp dll)
endif()
