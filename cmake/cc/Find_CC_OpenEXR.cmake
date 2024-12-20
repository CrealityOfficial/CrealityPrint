# This sets the following variables:
# Imath

if(OPENEXR_INSTALL_ROOT)
	message(STATUS "Specified OPENEXR_INSTALL_ROOT : ${OPENEXR_INSTALL_ROOT}")
	
	set(Imath_INCLUDE_ROOT ${OPENEXR_INSTALL_ROOT}/include/Imath/)
	set(Imath_LIB_ROOT ${OPENEXR_INSTALL_ROOT}/lib/)
	__search_target_components(Imath
							   INC ImathConfig.h
							   DLIB Imath
							   LIB Imath
							   )
	
	set(Iex_INCLUDE_ROOT ${OPENEXR_INSTALL_ROOT}/include/Iex/)
	set(Iex_LIB_ROOT ${OPENEXR_INSTALL_ROOT}/lib/)
	__search_target_components(Iex
							   INC Iex.h
							   DLIB Iex
							   LIB Iex
							   )

	set(IlmThread_INCLUDE_ROOT ${OPENEXR_INSTALL_ROOT}/include/IlmThread/)
	set(IlmThread_LIB_ROOT ${OPENEXR_INSTALL_ROOT}/lib/)
	__search_target_components(IlmThread
							   INC IlmThread.h
							   DLIB IlmThread
							   LIB IlmThread
							   )

	set(OpenEXR_INCLUDE_ROOT ${OPENEXR_INSTALL_ROOT}/include/)
	set(OpenEXR_LIB_ROOT ${OPENEXR_INSTALL_ROOT}/lib/)
	__search_target_components(OpenEXR
							   INC OpenEXR/OpenEXRConfig.h
							   DLIB OpenEXR
							   LIB OpenEXR
							   )
							   
	#set(Imath_INCLUDE_ROOT ${OPENEXR_INSTALL_ROOT}/include/Imath/)
	#set(Imath_LIB_ROOT ${OPENEXR_INSTALL_ROOT}/lib/)
	#__search_target_components(Imath
	#						   INC ImathConfig.h
	#						   DLIB Imath
	#						   LIB Imath
	#						   )							   
else()
endif()

add_definitions(-DIMATH_DLL)
__test_import(Imath dll)
__test_import(Iex dll)
__test_import(IlmThread dll)
__test_import(OpenEXR dll)