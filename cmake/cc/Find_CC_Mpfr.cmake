# This sets the following variables:
# mpfr target

if(NOT TARGET mpfr)
	if(NOT MPFR_INSTALL_ROOT)
		set(MPFR_INSTALL_ROOT $ENV{USR_INSTALL_ROOT})
	endif()
	
	set(mpfr_INCLUDE_DIRS ${MPFR_INSTALL_ROOT}/include/mpfr/)
	
    if(CC_BC_WIN)
		set(mpfr_LIBRARIES_DEBUG "${MPFR_INSTALL_ROOT}/lib/Debug/libmpfr-4.lib")
	    set(mpfr_LIBRARIES_RELEASE "${MPFR_INSTALL_ROOT}/lib/Release/libmpfr-4.lib")
	    set(mpfr_LOC_DEBUG "${MPFR_INSTALL_ROOT}/bin/Debug/libmpfr-4.dll")
	    set(mpfr_LOC_RELEASE "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr-4.dll")	
	elseif(CC_BC_LINUX)	
            set(mpfr_LIBRARIES_DEBUG "${MPFR_INSTALL_ROOT}/lib/Debug/libmpfr.so")
	    set(mpfr_LIBRARIES_RELEASE "${MPFR_INSTALL_ROOT}/lib/Release/libmpfr.so")
	    set(mpfr_LOC_DEBUG "${MPFR_INSTALL_ROOT}/bin/Debug/libmpfr.so")
	    set(mpfr_LOC_RELEASE "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr.so")	
       elseif(CC_BC_MAC)
            set(mpfr_LIBRARIES_DEBUG "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr.dylib")
            set(mpfr_LIBRARIES_RELEASE "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr.dylib")
            set(mpfr_LOC_DEBUG "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr.dylib")
            set(mpfr_LOC_RELEASE "${MPFR_INSTALL_ROOT}/bin/Release/libmpfr.dylib")

	endif()

	__test_import(mpfr dll)
endif()
