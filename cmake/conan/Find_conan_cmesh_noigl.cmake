# cmesh_noigl target

__cc_find(trimesh2)
__conan_import(cmesh_noigl dll ILIB trimesh2
							   INTERFACE_DEF USE_CMESH_DLL
								)
