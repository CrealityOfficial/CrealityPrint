# mpfr target
if(CC_BC_WIN)
	__conan_import_one(mpfr dll NAME mpfr LIB libmpfr-4 DLIB libmpfr-4
									  DLL libmpfr-4 DDLL libmpfr-4
									  )
else()
	__conan_import(mpfr lib)
endif()
