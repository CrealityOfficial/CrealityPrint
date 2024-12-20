# cxbin target

__conan_find(trimesh2)
__conan_import(boost dll COMPONENT boost_filesystem boost_nowide)

__conan_import(cxbin dll ILIB trimesh2 boost_filesystem boost_nowide)
