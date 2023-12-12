# ovdbutil target
if(CC_BC_EMCC)
	__conan_import(ovdbutil lib COMPONENT ovdbutil openvdb)
else()
	__conan_find(trimesh2)
	__conan_find(tbb)
	__conan_import(ovdbutil dll ILIB trimesh2)
endif()
