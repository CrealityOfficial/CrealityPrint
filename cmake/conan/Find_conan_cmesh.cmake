# cmesh target

__cc_find(boost)
__cc_find(tbb)
__cc_find(gmp)
__cc_find(mpfr)

__conan_import_one(cmesh dll NAME cmesh LIB cmesh DLL cmesh INTERFACE_DEF USE_CMESH_DLL)
