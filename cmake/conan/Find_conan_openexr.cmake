
set(EXR_COMPONENTS Iex IexMath IlmThread Imath OpenEXR OpenEXRUtil)

if(CC_BC_WIN)
	__conan_import(openexr dll COMPONENT ${EXR_COMPONENTS})
	__add_target_interface(Imath DEF IMATH_DLL)
else()
	__conan_import(openexr lib COMPONENT ${EXR_COMPONENTS})
endif()