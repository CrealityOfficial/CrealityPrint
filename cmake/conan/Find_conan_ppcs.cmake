
if(CC_BC_WIN)
	__conan_import_one(ppcs dll NAME ppcs LIB PPCS_API DLL PPCS_API)
elseif(CC_BC_LINUX)
	__conan_import_one(ppcs lib NAME ppcs LIB PPCS_API DLL PPCS_API)
elseif(CC_BC_MAC)
	__conan_import_one(ppcs lib NAME ppcs LIB PPCS_API DLL PPCS_API)
else()
endif()
