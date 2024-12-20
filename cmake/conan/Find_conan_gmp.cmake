# gmp target
if(CC_BC_WIN)
	__conan_import_one(gmp dll NAME gmp LIB libgmp-10 DLIB libgmp-10
									DLL libgmp-10 DDLL libgmp-10
									)
else()
	__conan_import(gmp lib)
endif()